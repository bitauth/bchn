// Copyright (c) 2025 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cstdint>

namespace Consensus {
namespace UpgradeTimes {

// Median-time-past (UTC 12:00:00) for tentative upgrades.
static constexpr int64_t MAY_2025 = 1747310400; // May 15, 2025 12:00:00 UTC
static constexpr int64_t NOV_2025 = 1763208000; // Nov 15, 2025 12:00:00 UTC
static constexpr int64_t MAY_2026 = 1778846400; // May 15, 2026 12:00:00 UTC
static constexpr int64_t NOV_2026 = 1794744000; // Nov 15, 2026 12:00:00 UTC
static constexpr int64_t MAY_2027 = 1810382400; // May 15, 2027 12:00:00 UTC
static constexpr int64_t NOV_2027 = 1826280000; // Nov 15, 2027 12:00:00 UTC
static constexpr int64_t MAY_2028 = 1842004800; // May 15, 2028 12:00:00 UTC
static constexpr int64_t NOV_2028 = 1857902400; // Nov 15, 2028 12:00:00 UTC
static constexpr int64_t MAY_2029 = 1873540800; // May 15, 2029 12:00:00 UTC
static constexpr int64_t NOV_2029 = 1889438400; // Nov 15, 2029 12:00:00 UTC

} // namespace UpgradeTimes
} // namespace Consensus
