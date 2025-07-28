// Copyright (c) 2025 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <chain.h>

#include <array>
#include <cassert>
#include <memory>
#include <vector>

template <bool noCache>
static void DoBench(benchmark::State &state) {
    using CBlockIndexPtr = std::unique_ptr<CBlockIndex>;
    std::vector<CBlockIndexPtr> chain;
    constexpr size_t chainSize = 10'000;
    constexpr int64_t t0 = 1'700'000'000;
    chain.reserve(chainSize);
    CBlockIndex *pindexPrev{};
    std::array<int64_t, 13> deltas = {0, 573, 10, 100, 601, -9, 635, -8, 700, 0, 832, -1, 333};
    size_t deltasIndex = 0;
    while (chain.size() < chainSize) {
        CBlockIndex *pindex = chain.emplace_back(std::make_unique<CBlockIndex>()).get();
        pindex->nTime = (pindexPrev ? pindexPrev->nTime : t0) + deltas[deltasIndex++ % deltas.size()];
        pindex->pprev = pindexPrev;
        pindexPrev = pindex;
        if (pindex->pprev) {
            assert(pindex->GetBlockTime() > pindex->pprev->GetMedianTimePast());
        }
    }

    BENCHMARK_LOOP {
        for (const auto &pindex : chain) {
            if constexpr (noCache) {
                pindex->ClearCachedMTPValue();
            }
            benchmark::NoOptimize(pindex->GetMedianTimePast());
        }
    }
}

static void GetMedianTimePast(benchmark::State &state) { DoBench<false>(state); }
static void GetMedianTimePast_Nocache(benchmark::State &state) { DoBench<true>(state); }

BENCHMARK(GetMedianTimePast, 3750);
BENCHMARK(GetMedianTimePast_Nocache, 3750);
