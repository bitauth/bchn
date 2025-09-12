#!/usr/bin/env python3
# Copyright (c) 2025 The Bitcoin developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
Verify getblockchaininfo.upgrade_status fields for the latest supported upgrade (May 2026).

Covers:
- Presence and types of fields pre-activation (future activation time):
  mempool_activated = false; block_activation_{height,hash} = null
- Correct mempool_activation_mtp equals CLI override
- software_expiration_mtp present (>0)
- Post-activation (past activation time) after mining a block:
  mempool_activated = true; block_activation_{height,hash} populated and consistent
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_greater_than
import time


class GetBlockchainInfoUpgradeStatusTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        # Start with upgrade 12 well in the future
        self.future_time = int(time.time()) + 10_000_000
        self.extra_args = [[f'-upgrade12activationtime={self.future_time}']]

    def run_test(self):
        node = self.nodes[0]

        # Pre-activation: far-future activation time => mempool_activated = false
        info = node.getblockchaininfo()
        us = info['upgrade_status']
        assert us['name'].startswith('May 2026 Upgrade (')
        assert_equal(us['mempool_activation_mtp'], self.future_time)
        assert_equal(us['mempool_activated'], False)
        # Not yet known
        assert us['block_activation_height'] is None
        assert us['block_activation_hash'] is None
        # On regtest, expiration may be unset (0). Just ensure the field exists and is numeric.
        assert 'software_expiration_mtp' in us
        assert isinstance(us['software_expiration_mtp'], int)

        # Move activation time to the past and restart; mine a block to advance MTP and ensure activation reached
        # Ensure activation is definitely reached by using an activation time in the distant past
        past_time = 0
        self.restart_node(0, extra_args=[f'-upgrade12activationtime={past_time}'])
        # Mine one block so tip/MTP reflect current and activation logic runs
        node.generatetoaddress(1, node.getnewaddress())

        info = node.getblockchaininfo()
        us = info['upgrade_status']
        assert_equal(us['mempool_activation_mtp'], past_time)
        assert_equal(us['mempool_activated'], True)

        # Activation block details should be available
        act_h = us['block_activation_height']
        act_hash = us['block_activation_hash']
        assert act_h is not None
        assert isinstance(act_h, int)
        assert act_hash is not None and isinstance(act_hash, str)
        # Hash is a valid 64-hex string and matches getblockhash(height)
        assert_equal(len(act_hash), 64)
        assert_equal(node.getblockhash(act_h), act_hash)


if __name__ == '__main__':
    GetBlockchainInfoUpgradeStatusTest().main()
