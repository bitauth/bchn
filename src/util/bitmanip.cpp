// Copyright (c) 2025 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/bitmanip.h>

#include <compat/endian.h>

#include <algorithm>
#include <cmath>
#include <climits>
#include <cstring>
#include <limits>
#include <stdexcept>

// Positive nbits: left-shift, negative nbits: right-shift
void bitShiftBlob(const Span<std::byte> &span, int64_t nbits) {
    if (nbits == std::numeric_limits<int64_t>::min()) {
        // Refuse to operate on INT64_MIN bits since negating this is undefined
        throw std::out_of_range("Cannot operate on INT64_MIN bits!");
    }
    if (span.size() > static_cast<uint64_t>(std::numeric_limits<int64_t>::max() / CHAR_BIT)) {
        // Refuse to operate on a span of > INT64_MAX bits
        throw std::out_of_range("Input byte span is too large (exceeds INT64_MAX bits)!");
    }
    if (span.empty() || nbits == 0) {
        // No work to do!
        return;
    }
    int64_t const spanBitSize = span.size() * CHAR_BIT;
    bool const rshift = nbits < 0;
    nbits = std::min(std::abs(nbits), spanBitSize); // normalize nbits to be positive and <= span bit size
    if (nbits >= spanBitSize) {
        // Short-circuit return, fill with 0's
        std::memset(span.data(), 0, span.size());
        return;
    }

    // For performance, we shift the data in 64-bit chunks, which should be faster than byte-wise shifting.

    constexpr size_t wordBytes = sizeof(uint64_t);
    constexpr size_t wordBits = wordBytes * CHAR_BIT;
    static_assert(wordBits == 64u && wordBytes == 8u);

    size_t const fullyMovedBytes = nbits / CHAR_BIT;
    size_t const perWordShift = (nbits - fullyMovedBytes * CHAR_BIT) % wordBits;

    if (rshift) {
        // Right-shift

        // 1. Move all fully-shifted bytes forward in the buffer
        if (fullyMovedBytes) {
            std::memmove(span.data() + fullyMovedBytes, span.data(), span.size() - fullyMovedBytes);
            // zero-fill front
            std::memset(span.data(), 0, fullyMovedBytes);
        }

        // 2. Right-shift the non-zeroed portion of the buffer as individual 64-bit words (if needed)
        if (perWordShift) {
            uint64_t leftOverBits{};
            size_t i = fullyMovedBytes;
            while (i < span.size()) {
                uint64_t val{};
                size_t const nb = std::min(span.size() - i, wordBytes);
                std::memcpy(&val, &span[i], nb);
                // Treat in-memory bytes as "big endian"
                val = be64toh(val);
                // Calculate new value which includes any leftover bits, pasted in as the top-most bits
                uint64_t newval = leftOverBits | (val >> perWordShift);
                // Save bottom "carry" bits that have been shifted out to the top-most bit positions of leftOverBits,
                // for the next iteration (will be discarded on the last iteration, which is what we want).
                leftOverBits = val << (wordBits - perWordShift);
                // Save back to memory as big endian
                newval = htobe64(newval);
                std::memcpy(&span[i], &newval, nb);

                i += nb;
            }
        }
    } else {
        // Left-shift

        // 1. Move all fully-shifted bytes backward to the beginning of the buffer
        if (fullyMovedBytes) {
            std::memmove(span.data(), span.data() + fullyMovedBytes, span.size() - fullyMovedBytes);
            // zero-fill back
            std::memset(span.data() + span.size() - fullyMovedBytes, 0, fullyMovedBytes);
        }

        // 2. Left-shift the non-zeroed portion of the buffer as individual 64-bit words (if needed)
        if (perWordShift) {
            uint64_t leftOverBits{};
            // Iterate backward, left-shifting each word
            size_t lastPos = span.size() - fullyMovedBytes;
            size_t pos = lastPos > wordBytes ? lastPos - wordBytes : 0u;
            while (lastPos) {
                uint64_t val{};
                size_t const nb = lastPos - pos;
                std::memcpy(&val, &span[pos], nb);
                // Treat in-memory bytes as "big endian"
                val = be64toh(val);
                // Fudge factor for the last iteration (at the beginning of the buffer, which may be unaligned)
                size_t const fudgeBits = nb < wordBytes ? (wordBytes - nb) * CHAR_BIT : 0u;
                val >>= fudgeBits;
                // Calculate new value which includes any leftover bits as bottom-most bits
                uint64_t newval = (val << perWordShift) | leftOverBits;
                // Save top "carry" bits that have been shifted out to the bottom-most bit positions of leftOverBits,
                // for the next iteration (will be discarded on the last iteration, which is what we want).
                leftOverBits = val >> (wordBits - perWordShift);
                // Fudge factor for the last iteration
                newval <<= fudgeBits;
                // Save back to memory as big endian
                newval = htobe64(newval);
                std::memcpy(&span[pos], &newval, nb);

                lastPos = pos;
                pos -= std::min(pos, wordBytes);
            }
        }
    }
}
