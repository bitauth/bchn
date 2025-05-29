// Copyright (c) 2019-2025 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <config/bitcoin-config.h>

#include <span.h>

#include <cstddef>
#include <cstdint>

inline uint32_t countBits(uint32_t v) {
#if HAVE_DECL___BUILTIN_POPCOUNT
    return __builtin_popcount(v);
#else
    /**
     * Computes the number of bits set in each group of 8bits then uses a
     * multiplication to sum all of them in the 8 most significant bits and
     * return these.
     * More detailed explanation can be found at
     * https://www.playingwithpointers.com/blog/swar.html
     */
    v = v - ((v >> 1) & 0x55555555);
    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
    return (((v + (v >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;
#endif
}


/**
 * @brief bitShiftBlob - Bit-shift a binary byte blob left or right, as if it were 1 large (unsigned) machine word.
 * @param span - The byte blob to bit-shift in-place
 * @param nbits - The number of bits to shift left/right. Positive values are for left-shift, negative for right-shift.
 * @pre `span` should be <= INT64_MAX / 8 in size. `nbits` must not be equal to INT64_MIN.
 * @post `span` is bit-shifted in-place as if it were a giant span.size() * 8 bit machine word, with 0 bits shifted into
 * the right-most/left-most end. If `std::abs(nbits) >= span.size() * 8`, then the span is completely cleared with 0's.
 * @exception std::out_of_range - If either `span.size() > INT64_MAX / 8`, or if `nbits == INT64_MIN`.
 */
void bitShiftBlob(const Span<std::byte> &span, int64_t nbits);

// helpers for above
inline void leftShiftBlob (const Span<std::byte> &span, uint32_t const nbits) { bitShiftBlob(span,  static_cast<int64_t>(nbits)); }
inline void rightShiftBlob(const Span<std::byte> &span, uint32_t const nbits) { bitShiftBlob(span, -static_cast<int64_t>(nbits)); }

// overloads using uint8_t instead of std::byte
inline void bitShiftBlob(const Span<uint8_t> &span, int64_t const nbits) {
    bitShiftBlob(Span<std::byte>{reinterpret_cast<std::byte *>(span.data()), span.size()},  nbits);
}
inline void leftShiftBlob (const Span<uint8_t> &span, uint32_t const nbits) { bitShiftBlob(span,  static_cast<int64_t>(nbits)); }
inline void rightShiftBlob(const Span<uint8_t> &span, uint32_t const nbits) { bitShiftBlob(span, -static_cast<int64_t>(nbits)); }
