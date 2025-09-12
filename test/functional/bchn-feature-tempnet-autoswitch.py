#!/usr/bin/env python3
# Copyright (c) 2025 The Bitcoin Cash Node developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Verify tempnet sunset behavior:

- After the Nov 15 upgrade time, -tempnet is interpreted as chipnet on startup.
- Tempnet-only configuration is ignored when remapped (e.g. -upgrade12activationtx).

Note: The functional test framework always runs nodes on regtest by default.
This test exercises the remap and config-ignoring behavior by ensuring that
passing -tempnet together with a tempnet-only option after the switch time
does not cause an init error and the node starts on regtest.
"""

from test_framework.test_framework import BitcoinTestFramework
import time


class TempnetAutoSwitchTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        # Restart node with -tempnet and tempnet-only flag after the switch time.
        self.stop_node(0)

        # Use a recent time in the past for switch time to ensure remap is active.
        switch_time = int(time.time()) - 1

        # Minimal hex string is fine; the content is ignored in this test.
        hex_tx = "00"
        self.start_node(0, extra_args=[
            f"-upgrade12activationtime={switch_time}",
            "-tempnet",
            f"-upgrade12activationtx={hex_tx}",
        ])

        info = self.nodes[0].getblockchaininfo()
        assert info["chain"] == "regtest"

        # On remap, a warning is emitted to stderr about ignoring the tempnet-only arg.
        self.stop_node(0, expected_stderr="Warning: Ignoring -upgrade12activationtx: tempnet has retired and is remapped to chipnet.")


if __name__ == '__main__':
    TempnetAutoSwitchTest().main()
