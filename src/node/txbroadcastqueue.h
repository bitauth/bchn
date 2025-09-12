// Copyright (c) 2025 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include <primitives/transaction.h>
#include <uint256.h>

class CBlockIndex;
class Config;

// Lightweight queue for scheduling transaction broadcasts by height or MTP.
// Node-level facility (non-consensus), in-memory only.

bool EnqueueTxForBroadcast(const CTransactionRef &tx,
                           std::optional<int> height,
                           std::optional<int64_t> mtp);

void ProcessTxBroadcastQueue(const Config &config, const CBlockIndex *pindexNew);

std::vector<std::tuple<std::string, std::optional<int>, std::optional<int64_t>, unsigned int>>
GetTxBroadcastQueue();

bool CancelQueuedTxBroadcast(const uint256 &txid);
