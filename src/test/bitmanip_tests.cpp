// Copyright (c) 2019-2025 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/setup_common.h>

#include <compat/endian.h>
#include <random.h>
#include <util/bitmanip.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <limits>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(bitmanip_tests, BasicTestingSetup)

static void CheckBitCount(uint32_t value, uint32_t expected_count) {
    // Count are rotation invariant.
    for (int i = 0; i < 32; i++) {
        BOOST_CHECK_EQUAL(countBits(value), expected_count);
        value = (value << 1) | (value >> 31);
    }
}

static uint32_t countBitsNaive(uint32_t value) {
    uint32_t ret = 0;
    while (value != 0) {
        ret += (value & 0x01);
        value >>= 1;
    }

    return ret;
}

#define COUNT 4096

BOOST_AUTO_TEST_CASE(bit_count) {
    // Check various known values.
    CheckBitCount(0, 0);
    CheckBitCount(1, 1);
    CheckBitCount(0xffffffff, 32);
    CheckBitCount(0x01234567, 12);
    CheckBitCount(0x12345678, 13);
    CheckBitCount(0xfedcba98, 20);
    CheckBitCount(0x5a55aaa5, 16);
    CheckBitCount(0xdeadbeef, 24);

    for (uint32_t i = 2; i != 0; i <<= 1) {
        // Check two bit set for all combinations.
        CheckBitCount(i | 0x01, 2);
    }

    // Check many small values against a naive implementation.
    for (uint32_t v = 0; v <= 0xfff; v++) {
        CheckBitCount(v, countBitsNaive(v));
    }

    // Check random values against a naive implementation.
    for (int i = 0; i < COUNT; i++) {
        uint32_t v = InsecureRand32();
        CheckBitCount(v, countBitsNaive(v));
    }
}

BOOST_AUTO_TEST_CASE(bitShiftBlob_exceptions) {
    std::byte *nullbytearray = nullptr;
    uint8_t *nulluchararray = nullptr;
    const size_t size_t_max = std::numeric_limits<size_t>::max();

    // Check that a byte blob that has too many bits throws
    BOOST_CHECK_THROW(bitShiftBlob({nullbytearray, size_t_max / 8u + 1u}, 1, true), std::out_of_range);
    BOOST_CHECK_THROW(bitShiftBlob({nullbytearray, size_t_max / 8u + 1u}, 1, false), std::out_of_range);
    BOOST_CHECK_THROW(bitShiftBlob({nulluchararray, size_t_max / 8u + 1u}, 1, true), std::out_of_range);
    BOOST_CHECK_THROW(bitShiftBlob({nulluchararray, size_t_max / 8u + 1u}, 1, false), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(bitShiftBlob_small_vals) {

    auto Spanify = [](auto & i) -> Span<std::byte> {
        return {reinterpret_cast<std::byte *>(&i), sizeof(i)};
    };

    for (size_t i = 0; i < 20'000; ++i) {
        const uint64_t datum = GetRand64();
        const size_t shiftAmt = GetRandInt(64), shiftAmt32 = GetRandInt(32), shiftAmt16 = GetRandInt(16), shiftAmt8 = GetRandInt(8);

        for (const bool rshift : {false, true}) {
            // 64-bit
            {
                // binary shifting of bitShiftBlob behaves "as if" it's operating on big endian data
                uint64_t datum_be = htobe64(datum);
                bitShiftBlob(Spanify(datum_be), shiftAmt, rshift);
                const uint64_t expected = rshift ? datum >> shiftAmt : datum << shiftAmt;
                BOOST_CHECK_EQUAL(be64toh(datum_be), expected);
            }

            // 32-bit
            {
                // binary shifting of bitShiftBlob behaves "as if" it's operating on big endian data
                const uint32_t datum32 = datum;
                uint32_t datum_be = htobe32(datum32);
                bitShiftBlob(Spanify(datum_be), shiftAmt32, rshift);
                const uint32_t expected = rshift ? datum32 >> shiftAmt32 : datum32 << shiftAmt32;
                BOOST_CHECK_EQUAL(be32toh(datum_be), expected);
            }

            // 16-bit
            {
                // binary shifting of bitShiftBlob behaves "as if" it's operating on big endian data
                const uint16_t datum16 = datum;
                uint16_t datum_be = htobe16(datum16);
                bitShiftBlob(Spanify(datum_be), shiftAmt16, rshift);
                const uint16_t expected = rshift ? datum16 >> shiftAmt16 : datum16 << shiftAmt16;
                BOOST_CHECK_EQUAL(be16toh(datum_be), expected);
            }

            // 8-bit
            {
                // binary shifting of bitShiftBlob behaves "as if" it's operating on big endian data
                const uint8_t datum8 = datum;
                uint8_t datum_mutable = datum8;
                bitShiftBlob(Spanify(datum_mutable), shiftAmt8, rshift);
                const uint8_t expected = rshift ? datum8 >> shiftAmt8 : datum8 << shiftAmt8;
                BOOST_CHECK_EQUAL(datum_mutable, expected);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(bitShiftBlob_arbitrary_data) {
    // Convert a vector of bytes into a vector of bools, with each bit unpacked into a bool
    auto ToBoolVec = [](const std::vector<uint8_t> &vec) -> std::vector<bool> {
        std::vector<bool> ret;
        ret.reserve(vec.size() * 8);
        for (const auto &item : vec) {
            constexpr size_t isz = 8;
            // MSB first
            for (size_t i = isz; i != 0; --i) {
                const size_t shift = i - 1;
                ret.push_back(item >> shift & 0x1u);
            }
        }
        return ret;
    };
    // Convert a bool vector into a byte vector with the bits packed into each byte
    auto FromBoolVec = [](const std::vector<bool> &bv) -> std::vector<uint8_t> {
        std::vector<uint8_t> ret;
        ret.reserve((bv.size() + 1) / 8);
        size_t bitnum = 7;
        uint8_t byte = 0;
        for (const bool b : bv) {
            // MSB first
            if (b) byte |= 0x1u << bitnum;
            if (!bitnum) {
                ret.push_back(byte);
                bitnum = 7;
                byte = 0;
            } else {
                --bitnum;
            }
        }
        // add trailing bools to last byte
        if (bitnum != 7) {
            ret.push_back(byte);
        }
        return ret;
    };
    // Verifying function, shifts bits using slow vector<bool>
    auto ShiftBoolVec = [](const std::vector<bool> &bv, int amt) -> std::vector<bool> {
        std::vector<bool> ret;
        ret.reserve(bv.size());

        const bool rshift = amt < 0;
        amt = std::min<int>(std::abs(amt), int(bv.size()));

        std::vector<bool>::const_iterator it, end;
        if (rshift) {
            // pad beginning
            ret.assign(static_cast<size_t>(amt), false);
            it = bv.begin();
            end = bv.end() - amt;
        } else {
            it = bv.begin() + amt;
            end = bv.end();
        }

        while (it != end) {
            ret.push_back(*it++);
        }

        if (!rshift) {
            // pad ending
            ret.resize(ret.size() + amt, false);
        }

        return ret;
    };

    // check sanity of FromBoolVec/ToBoolVec/ShiftBoolVec
    {
        BOOST_REQUIRE(FromBoolVec(ToBoolVec(ParseHex("f33db33ff00d1234"))) == ParseHex("f33db33ff00d1234"));
        BOOST_REQUIRE(FromBoolVec(ShiftBoolVec(ToBoolVec(ParseHex("deadbeef")),  8)) == ParseHex("adbeef00"));
        BOOST_REQUIRE(FromBoolVec(ShiftBoolVec(ToBoolVec(ParseHex("deadbeef")), -8)) == ParseHex("00deadbe"));
        BOOST_REQUIRE(FromBoolVec(ShiftBoolVec(ToBoolVec(ParseHex("deadbeef")),  4)) == ParseHex("eadbeef0"));
        BOOST_REQUIRE(FromBoolVec(ShiftBoolVec(ToBoolVec(ParseHex("deadbeef")), -4)) == ParseHex("0deadbee"));
        BOOST_REQUIRE(FromBoolVec(ShiftBoolVec(ToBoolVec(ParseHex("deadbeef")),  3)) == ParseHex("f56df778"));
        BOOST_REQUIRE(FromBoolVec(ShiftBoolVec(ToBoolVec(ParseHex("deadbeef")), -3)) == ParseHex("1bd5b7dd"));
        BOOST_REQUIRE(FromBoolVec(ShiftBoolVec(ToBoolVec(ParseHex("deadbeef")),  1)) == ParseHex("bd5b7dde"));
        BOOST_REQUIRE(FromBoolVec(ShiftBoolVec(ToBoolVec(ParseHex("deadbeef")), -1)) == ParseHex("6f56df77"));
        BOOST_REQUIRE(FromBoolVec(ShiftBoolVec(ToBoolVec(ParseHex("deadbeef")),  0)) == ParseHex("deadbeef"));
        BOOST_REQUIRE(FromBoolVec(ShiftBoolVec(ToBoolVec(ParseHex("deadbeef")),  32)) == ParseHex("00000000"));
        BOOST_REQUIRE(FromBoolVec(ShiftBoolVec(ToBoolVec(ParseHex("deadbeef")), -32)) == ParseHex("00000000"));
        BOOST_REQUIRE(FromBoolVec(ShiftBoolVec(ToBoolVec(ParseHex("deadbeef")),  31)) == ParseHex("80000000"));
        BOOST_REQUIRE(FromBoolVec(ShiftBoolVec(ToBoolVec(ParseHex("deadbeef")), -31)) == ParseHex("00000001"));
        BOOST_REQUIRE(FromBoolVec(ShiftBoolVec(ToBoolVec(ParseHex("deadbeef")), -30)) == ParseHex("00000003"));
        BOOST_REQUIRE(FromBoolVec(ShiftBoolVec(ToBoolVec(ParseHex("deadbeef")), -29)) == ParseHex("00000006"));
        BOOST_REQUIRE(FromBoolVec(ShiftBoolVec(ToBoolVec(ParseHex("deadbeef")),  33)) == ParseHex("00000000"));
        BOOST_REQUIRE(FromBoolVec(ShiftBoolVec(ToBoolVec(ParseHex("deadbeef")), -33)) == ParseHex("00000000"));
        BOOST_REQUIRE(FromBoolVec(ShiftBoolVec(ToBoolVec(ParseHex("deadbeef")),  67)) == ParseHex("00000000"));
        BOOST_REQUIRE(FromBoolVec(ShiftBoolVec(ToBoolVec(ParseHex("deadbeef")), -67)) == ParseHex("00000000"));
    }

    // simple check of left and right shifting for bitShiftBlob
    {
        std::vector<uint8_t> data, shifted;
        shifted = data = ParseHex("beeff00d");
        bitShiftBlob(shifted, 4, false);
        BOOST_CHECK_EQUAL(HexStr(shifted), "eeff00d0");
        BOOST_CHECK(FromBoolVec(ShiftBoolVec(ToBoolVec(data), 4)) == shifted); // check vs our verify code
        // right shift
        shifted = data;
        bitShiftBlob(shifted, 4, true);
        BOOST_CHECK_EQUAL(HexStr(shifted), "0beeff00");
        BOOST_CHECK(FromBoolVec(ShiftBoolVec(ToBoolVec(data), -4)) == shifted); // check vs our verify code
        // left shift by 3 bits
        shifted = data;
        bitShiftBlob(shifted, 3, false);
        BOOST_CHECK_EQUAL(HexStr(shifted), "f77f8068");
        BOOST_CHECK(FromBoolVec(ShiftBoolVec(ToBoolVec(data), 3)) == shifted); // check vs our verify code
        // right shift by 3 bits
        shifted = data;
        bitShiftBlob(shifted, 3, true);
        BOOST_CHECK_EQUAL(HexStr(shifted), "17ddfe01");
        BOOST_CHECK(FromBoolVec(ShiftBoolVec(ToBoolVec(data), -3)) == shifted); // check vs our verify code
        // left shift by 17 bits
        shifted = data;
        bitShiftBlob(shifted, 17, false);
        BOOST_CHECK_EQUAL(HexStr(shifted), "e01a0000");
        BOOST_CHECK(FromBoolVec(ShiftBoolVec(ToBoolVec(data), 17)) == shifted); // check vs our verify code
        // right shift by 17 bits
        shifted = data;
        bitShiftBlob(shifted, 17, true);
        BOOST_CHECK_EQUAL(HexStr(shifted), "00005f77");
        BOOST_CHECK(FromBoolVec(ShiftBoolVec(ToBoolVec(data), -17)) == shifted); // check vs our verify code
    }

    // Now, generate random pieces of data of random lengths and randomly shift the bits, verifying vs our
    // verify function
    FastRandomContext ctx;
    for (size_t i = 0; i < 16; ++i) {
        // random piece of data up to 16KB in size
        const auto datablob = ctx.randbytes(ctx.randrange(16'000));
        const auto ndatabits = datablob.size() * 8u;
        BOOST_REQUIRE(FromBoolVec(ToBoolVec(datablob)) == datablob); // sanity check

        for (size_t j = 0; j < 32; ++j) {
            // randomize shift amount
            const int shiftamt_big = ctx.randrange(ndatabits),
                      shiftamt_small = ctx.randrange(std::min<size_t>(ndatabits, 80));

            // shift both in left and right shift directions, and test vs verify function
            for (const auto amt : {shiftamt_big, shiftamt_small}) {
                for (const bool rshift : {false, true}) {
                    auto shifted = datablob;
                    bitShiftBlob(shifted, amt, rshift);

                    auto expected = FromBoolVec(ShiftBoolVec(ToBoolVec(datablob), rshift ? -amt : amt));
                    BOOST_CHECK(expected == shifted);
                }
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
