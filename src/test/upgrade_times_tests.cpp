// Copyright (c) 2025 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/upgrade_times.h>
#include <util/time.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(upgrade_times_tests)

BOOST_AUTO_TEST_CASE(constants_match_expected_unix_times) {
    using namespace Consensus::UpgradeTimes;
    auto ts = [](const char *iso) { return ParseISO8601DateTime(iso); };
    // Validate constants by parsing UTC ISO8601 dates at 12:00:00.
    BOOST_CHECK_EQUAL(MAY_2025, ts("2025-05-15T12:00:00Z"));
    BOOST_CHECK_EQUAL(NOV_2025, ts("2025-11-15T12:00:00Z"));
    BOOST_CHECK_EQUAL(MAY_2026, ts("2026-05-15T12:00:00Z"));
    BOOST_CHECK_EQUAL(NOV_2026, ts("2026-11-15T12:00:00Z"));
    BOOST_CHECK_EQUAL(MAY_2027, ts("2027-05-15T12:00:00Z"));
    BOOST_CHECK_EQUAL(NOV_2027, ts("2027-11-15T12:00:00Z"));
    BOOST_CHECK_EQUAL(MAY_2028, ts("2028-05-15T12:00:00Z"));
    BOOST_CHECK_EQUAL(NOV_2028, ts("2028-11-15T12:00:00Z"));
    BOOST_CHECK_EQUAL(MAY_2029, ts("2029-05-15T12:00:00Z"));
    BOOST_CHECK_EQUAL(NOV_2029, ts("2029-11-15T12:00:00Z"));
}

BOOST_AUTO_TEST_SUITE_END()
