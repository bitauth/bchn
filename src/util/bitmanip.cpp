// Copyright (c) 2025 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/bitmanip.h>

#include <algorithm>
#include <cmath>
#include <climits>
#include <cstring>
#include <limits>
#include <stdexcept>

// left-shift is positive nbits, right-shift is negative nbits
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
        // no work to do!
        return;
    }
    int64_t const spanBitSize = span.size() * CHAR_BIT;
    bool const rshift = nbits < 0;
    nbits = std::min(std::abs(nbits), spanBitSize); // normalize nbits to be positive and <= span bit size
    if (nbits >= spanBitSize) {
        // short-circuit return, fill with 0's
        std::memset(span.data(), 0, span.size());
        return;
    }

    size_t const fullyMovedBytes = nbits / CHAR_BIT;
    size_t const perByteShift = nbits % CHAR_BIT;

    if (rshift) {
        // right-shift

        // 1. move all fully-shifted bytes forward
        if (fullyMovedBytes) {
            std::memmove(span.data() + fullyMovedBytes, span.data(), span.size() - fullyMovedBytes);
            // zero-fill front
            std::memset(span.data(), 0, fullyMovedBytes);
        }

        // 2. right-shift all individual bytes (if needed)
        if (perByteShift) {
            std::byte leftOverBits{0u};
            for (size_t i = fullyMovedBytes; i < span.size(); ++i) {
                std::byte &b = span[i];
                // calculate new value which includes any leftover bits as top-most bits
                std::byte const newval = leftOverBits | (b >> perByteShift);
                // save bottom bits as "left over" topmost bits for next iteration (will be discarded on last iteration)
                leftOverBits = b << (CHAR_BIT - perByteShift);
                // finally, overwrite this byte
                b = newval;
            }
        }
    } else {
        // left-shift

        // 1. move all fully-shifted bytes backward
        if (fullyMovedBytes) {
            std::memmove(span.data(), span.data() + fullyMovedBytes, span.size() - fullyMovedBytes);
            // zero-fill back
            std::memset(span.data() + span.size() - fullyMovedBytes, 0, fullyMovedBytes);
        }

        // 2. left-shift all individual bytes (if needed)
        if (perByteShift && fullyMovedBytes < span.size()) {
            std::byte leftOverBits{0u};
            // iterate backward, left-shifting each byte
            size_t pos = span.size() - fullyMovedBytes;
            while (pos > 0u) {
                std::byte &b = span[--pos];
                // calculate new value which include any leftovers as bottom-most bits
                std::byte const newval = (b << perByteShift) | leftOverBits;
                // save top bits as "left over" bottom-most bits for next iteration (will be discarded on last iteration)
                leftOverBits = b >> (CHAR_BIT - perByteShift);
                // finally, overwrite this byte
                b = newval;
            }
        }
    }
}
