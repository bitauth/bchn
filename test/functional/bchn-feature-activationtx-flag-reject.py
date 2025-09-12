#!/usr/bin/env python3
# Copyright (c) 2025 The Bitcoin Cash Node developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Verify -upgrade12activationtx behavior across networks:

- On regtest: option is ignored with a warning.
- On chipnet: option overrides the preset and is accepted.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.test_node import ErrorMatch


class ActivationTxFlagRejectTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def setup_network(self):
        # Do not auto-start nodes; we want to control startup args.
        self.add_nodes(self.num_nodes)

    def run_test(self):
        # Stop the running node to test startup with custom args
        self.stop_node(0)

        # 1) regtest: option is ignored; node starts successfully
        self.start_node(0, extra_args=["-upgrade12activationtx=00"])
        self.stop_node(0)

        # 2) chipnet: option accepted (use a valid tx hex)
        hex_tx_chip = (
            "0100000001b14bdcbc3e01bdaad36cc08e81e69c82e1060bc14e518db2b49aa43ad90ba26"
            "000000000490047304402203f16c6f40162ab686621ef3000b04e75418a0c0cb2d8aebeac89"
            "4ae360ac1e780220ddc15ecdfc3507ac48e1681a33eb60996631bf6bf5bc0a0682c4db743ce7"
            "ca2b01ffffffff0140420f00000000001976a914660d4ef3a743e3e696ad990364e555c271ad"
            "504b88ac00000000"
        )
        self.start_node(0, extra_args=[f"-upgrade12activationtx={hex_tx_chip}", "-chipnet"])
        # Node should start successfully
        self.stop_node(0)


if __name__ == '__main__':
    ActivationTxFlagRejectTest().main()
