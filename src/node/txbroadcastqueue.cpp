// Copyright (c) 2025 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/txbroadcastqueue.h>

#include <chainparams.h>
#include <chain.h>
#include <consensus/params.h>
#include <logging.h>
#include <primitives/txid.h>
#include <rpc/protocol.h>
#include <streams.h>
#include <sync.h>
#include <tinyformat.h>
#include <validationinterface.h>

#include <algorithm>

namespace {
struct QueuedTx {
    CTransactionRef tx;
    std::optional<int> height;
    std::optional<int64_t> mtp;
};

static RecursiveMutex cs_tx_broadcast_queue;
static std::vector<QueuedTx> g_tx_broadcast_queue GUARDED_BY(cs_tx_broadcast_queue);
} // namespace

TxId BroadcastTransaction(const Config &config, CTransactionRef tx,
                          bool allowhighfees = false,
                          bool wait_for_wallet = true);

bool EnqueueTxForBroadcast(const CTransactionRef &tx,
                           std::optional<int> height,
                           std::optional<int64_t> mtp) {
    const auto txid = tx->GetId();
    if (!height && !mtp) {
        LogPrint(BCLog::MEMPOOL,
                 "Ignoring request to queue tx %s with no height/mtp trigger\n",
                 txid.ToString());
        return false;
    }
    if (height && mtp) {
        LogPrint(BCLog::MEMPOOL,
                 "Ignoring request to queue tx %s with both height and mtp triggers\n",
                 txid.ToString());
        return false;
    }
    if (height && *height < 0) {
        LogPrint(BCLog::MEMPOOL,
                 "Ignoring request to queue tx %s with negative height %d\n",
                 txid.ToString(), *height);
        return false;
    }
    if (mtp && *mtp < 0) {
        LogPrint(BCLog::MEMPOOL,
                 "Ignoring request to queue tx %s with negative mtp %d\n",
                 txid.ToString(), *mtp);
        return false;
    }
    LOCK(cs_tx_broadcast_queue);
    for (const auto &e : g_tx_broadcast_queue) {
        if (e.tx->GetId() == txid) {
            LogPrint(BCLog::MEMPOOL,
                     "Ignoring duplicate request to queue tx %s\n",
                     txid.ToString());
            return false;
        }
    }
    g_tx_broadcast_queue.push_back({tx, height, mtp});
    if (height) {
        LogPrint(BCLog::MEMPOOL,
                 "Queued tx %s for broadcast at height %d\n",
                 txid.ToString(), *height);
    } else if (mtp) {
        LogPrint(BCLog::MEMPOOL,
                 "Queued tx %s for broadcast at MTP %d\n",
                 txid.ToString(), *mtp);
    }
    return true;
}

void ProcessTxBroadcastQueue(const Config &config, const CBlockIndex *pindexNew) {
    std::vector<QueuedTx> toBroadcast;
    {
        LOCK(cs_tx_broadcast_queue);
        auto it = g_tx_broadcast_queue.begin();
        while (it != g_tx_broadcast_queue.end()) {
            bool ready = false;
            if (it->height && pindexNew->nHeight >= *it->height) ready = true;
            if (it->mtp && pindexNew->GetMedianTimePast() >= *it->mtp) ready = true;
            if (ready) {
                toBroadcast.push_back(*it);
                it = g_tx_broadcast_queue.erase(it);
            } else {
                ++it;
            }
        }
    }
    for (const auto &entry : toBroadcast) {
        const CTransactionRef ptx = entry.tx;
        const auto txid = ptx->GetId();
        const std::string trigger = entry.height ? strprintf("height>=%d", *entry.height)
                                                 : strprintf("mtp>=%d", *entry.mtp);
        LogPrint(BCLog::MEMPOOL,
                 "Broadcasting queued tx %s (trigger: %s)\n",
                 txid.ToString(), trigger);
        CallFunctionInValidationInterfaceQueue([&config, ptx, txid, trigger]() {
            try {
                BroadcastTransaction(config, ptx, true /*allowhighfees*/, false /*wait_for_wallet*/);
                LogPrintf("Queued-broadcast success: %s (trigger: %s)\n", txid.ToString(), trigger);
            } catch (const JSONRPCError &e) {
                LogPrintf("Queued-broadcast failed: %s (trigger: %s) err=%s\n",
                          txid.ToString(), trigger, e.message);
            } catch (const std::exception &e) {
                LogPrintf("Queued-broadcast failed: %s (trigger: %s) err=%s\n",
                          txid.ToString(), trigger, e.what());
            } catch (...) {
                LogPrintf("Queued-broadcast failed: %s (trigger: %s) err=%s\n",
                          txid.ToString(), trigger, "unknown");
            }
        });
    }
}

std::vector<std::tuple<std::string, std::optional<int>, std::optional<int64_t>, unsigned int>>
GetTxBroadcastQueue() {
    std::vector<std::tuple<std::string, std::optional<int>, std::optional<int64_t>, unsigned int>> out;
    LOCK(cs_tx_broadcast_queue);
    out.reserve(g_tx_broadcast_queue.size());
    for (const auto &e : g_tx_broadcast_queue) {
        out.emplace_back(e.tx->GetId().ToString(), e.height, e.mtp, e.tx->GetTotalSize());
    }
    return out;
}

bool CancelQueuedTxBroadcast(const uint256 &txid) {
    LOCK(cs_tx_broadcast_queue);
    bool removed = false;
    auto it = g_tx_broadcast_queue.begin();
    while (it != g_tx_broadcast_queue.end()) {
        if (it->tx->GetId() == txid) {
            it = g_tx_broadcast_queue.erase(it);
            removed = true;
        } else {
            ++it;
        }
    }
    if (removed) {
        LogPrint(BCLog::MEMPOOL,
                 "Cancelled queued broadcast for tx %s\n",
                 txid.ToString());
    }
    return removed;
}
