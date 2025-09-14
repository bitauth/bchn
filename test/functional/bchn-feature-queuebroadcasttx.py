#!/usr/bin/env python3
# Copyright (c) 2025 The Bitcoin Cash Node developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Exercise the transaction broadcast queue:
 - queuebroadcasttx by height and by MTP
 - gettxbroadcastqueue returns queued items
 - error handling for invalid parameter combinations
 - broadcasting occurs at the specified trigger
"""

from decimal import Decimal
import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_raises_rpc_error, wait_until


class QueueBroadcastTxTest(BitcoinTestFramework):
    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def set_test_params(self):
        self.num_nodes = 1
        # Reuse cached pre-mined chain to have plenty of mature UTXOs
        self.setup_clean_chain = False
        # Use default node args, rely on runner's portseed; cached chain ensures speed
        self.extra_args = [[]]
        self.rpc_timeout = 120

    def make_signed_tx(self, node, amount=Decimal('0.001')):
        # Build a simple one-input, one-output tx from an existing UTXO to avoid
        # coin selection and speed up the test. We subtract a small fee.
        # Use a distinct UTXO each time to avoid double-spend conflicts.
        if not hasattr(self, '_utxos'):
            self._utxos = node.listunspent(1)
            self._utxo_idx = 0
        assert self._utxo_idx < len(self._utxos), "No UTXOs to spend"
        utxo = self._utxos[self._utxo_idx]
        self._utxo_idx += 1
        txin = {"txid": utxo["txid"], "vout": utxo["vout"]}
        fee = Decimal('0.0001')
        dest_addr = node.getnewaddress()
        send_amt = utxo["amount"] - fee
        assert send_amt > 0, "UTXO too small for fee"
        raw = node.createrawtransaction([txin], {dest_addr: send_amt})
        signed = node.signrawtransactionwithwallet(raw)["hex"]
        txid = node.decoderawtransaction(signed)["txid"]
        return txid, signed

    def run_test(self):
        node = self.nodes[0]
        # Pre-cache UTXOs so we can spend distinct coins for each tx
        self._utxos = node.listunspent(1)
        self._utxo_idx = 0

        # Height-triggered test (locktime-style: <500000000)
        txid_h, raw_h = self.make_signed_tx(node)
        h_now = node.getblockcount()
        target_h = h_now + 1
        qtxid = node.queuebroadcasttx(raw_h, target_h)
        assert qtxid == txid_h

        # Confirm it appears in the queue and not in mempool yet
        q = node.gettxbroadcastqueue()
        assert any(e["txid"] == txid_h and e["height"] == target_h for e in q)
        assert all("size" in e and isinstance(e["size"], int) and e["size"] > 0 for e in q)
        assert txid_h not in node.getrawmempool()

        # Mine one block to reach target height and trigger broadcast
        self.generate(node, 1)
        # Wait for mempool acceptance
        wait_until(lambda: txid_h in node.getrawmempool(), timeout=60)

        # Queue should be empty now
        q = node.gettxbroadcastqueue()
        assert all(e["txid"] != txid_h for e in q)

        # MTP-triggered test via locktime-style: use current MTP so next tip update triggers broadcast
        txid_m, raw_m = self.make_signed_tx(node)
        mtp_now = node.getblockheader(node.getbestblockhash())["mediantime"]
        target_mtp = mtp_now
        qtxid2 = node.queuebroadcasttx(raw_m, target_mtp)
        assert qtxid2 == txid_m
        q = node.gettxbroadcastqueue()
        assert any(e["txid"] == txid_m and e.get("mtp") == target_mtp for e in q)
        assert all("size" in e and isinstance(e["size"], int) and e["size"] > 0 for e in q)
        assert txid_m not in node.getrawmempool()
        self.generate(node, 1)
        wait_until(lambda: txid_m in node.getrawmempool(), timeout=60)

        # Error cases
        bad_tx_hex = "00"
        assert_raises_rpc_error(-22, "TX decode failed", node.queuebroadcasttx, bad_tx_hex, 1)
        # None specified (reuse an existing tx)
        q_before = node.gettxbroadcastqueue()
        assert_raises_rpc_error(-8, "Must specify height_or_mtp", node.queuebroadcasttx, raw_h)
        # Ensure the queue did not change on error
        assert node.gettxbroadcastqueue() == q_before

        # Duplicate queueing should not add another entry
        txid_dup, raw_dup = self.make_signed_tx(node)
        h_now = node.getblockcount()
        target_dup = h_now + 5
        node.queuebroadcasttx(raw_dup, target_dup)
        q0 = node.gettxbroadcastqueue()
        assert all("size" in e and isinstance(e["size"], int) and e["size"] > 0 for e in q0)
        # Queue same tx again with same trigger
        node.queuebroadcasttx(raw_dup, target_dup)
        q1 = node.gettxbroadcastqueue()
        assert q0 == q1

        # Cancel queued item and ensure it won't broadcast
        txid_c, raw_c = self.make_signed_tx(node)
        h_now = node.getblockcount()
        target_c = h_now + 3
        node.queuebroadcasttx(raw_c, target_c)
        assert any(e["txid"] == txid_c for e in node.gettxbroadcastqueue())
        assert node.cancelbroadcasttx(txid_c) is True
        assert all(e["txid"] != txid_c for e in node.gettxbroadcastqueue())
        # Mine one block; canceled item should not broadcast
        self.generate(node, 1)
        # It should not have been broadcast
        assert txid_c not in node.getrawmempool()
        # Cancel non-existent returns false
        assert node.cancelbroadcasttx(txid_c) is False

        # Failed broadcast should be logged and not requeued
        # Create a tx that spends a fake outpoint
        bogus_prev = [{"txid": "00" * 32, "vout": 0}]
        addr = node.getnewaddress()
        raw_fail = node.createrawtransaction(bogus_prev, {addr: 0.001})
        # Sign won't add inputs (no UTXO), but raw is still validly encoded
        h_now = node.getblockcount()
        target_fail = h_now + 2
        # Queue raw transaction directly via RPC (no signing)
        txid_fail = node.decoderawtransaction(raw_fail)["txid"]
        node.queuebroadcasttx(raw_fail, target_fail)
        self.generate(node, 2)
        # Should not appear in mempool and not be requeued
        assert txid_fail not in node.getrawmempool()
        assert all(e["txid"] != txid_fail for e in node.gettxbroadcastqueue())


if __name__ == '__main__':
    QueueBroadcastTxTest().main()
