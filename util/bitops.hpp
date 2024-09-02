/* Copyright (c) 2017-2023 Hans-Kristian Arntzen
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace Util
{
#ifdef __GNUC__
#define leading_zeroes_(x) ((x) == 0 ? 32 : __builtin_clz(x))
#define trailing_zeroes_(x) ((x) == 0 ? 32 : __builtin_ctz(x))
#define trailing_ones_(x) __builtin_ctz(~uint32_t(x))
#define leading_zeroes64_(x) ((x) == 0 ? 64 : __builtin_clzll(x))
#define trailing_zeroes64_(x) ((x) == 0 ? 64 : __builtin_ctzll(x))
#define trailing_ones64_(x) __builtin_ctzll(~uint64_t(x))
#define popcount32_(x) __builtin_popcount(x)

static inline uint32_t leading_zeroes(uint32_t x) { return leading_zeroes_(x); }
static inline uint32_t trailing_zeroes(uint32_t x) { return trailing_zeroes_(x); }
static inline uint32_t trailing_ones(uint32_t x) { return trailing_ones_(x); }
static inline uint32_t leading_zeroes64(uint64_t x) { return leading_zeroes64_(x); }
static inline uint32_t trailing_zeroes64(uint64_t x) { return trailing_zeroes64_(x); }
static inline uint32_t trailing_ones64(uint64_t x) { return trailing_ones64_(x); }
static inline uint32_t popcount32(uint32_t x) { return popcount32_(x); }

#elif defined(_MSC_VER)
namespace Internal
{
static inline uint32_t popcount32(uint32_t x)
{
	return __popcnt(x);
}

static inline uint32_t clz(uint32_t x)
{
	unsigned long result;
	if (_BitScanReverse(&result, x))
		return 31 - result;
	else
		return 32;
}

static inline uint32_t ctz(uint32_t x)
{
	unsigned long result;
	if (_BitScanForward(&result, x))
		return result;
	else
		return 32;
}

static inline uint32_t clz64(uint64_t x)
{
	unsigned long result;
	if (_BitScanReverse64(&result, x))
		return 63 - result;
	else
		return 64;
}

static inline uint32_t ctz64(uint64_t x)
{
	unsigned long result;
	if (_BitScanForward64(&result, x))
		return result;
	else
		return 64;
}
}

static inline uint32_t leading_zeroes(uint32_t x) { return Internal::clz(x); }
static inline uint32_t trailing_zeroes(uint32_t x) { return Internal::ctz(x); }
static inline uint32_t trailing_ones(uint32_t x) { return Internal::ctz(~x); }
static inline uint32_t leading_zeroes64(uint64_t x) { return Internal::clz64(x); }
static inline uint32_t trailing_zeroes64(uint64_t x) { return Internal::ctz64(x); }
static inline uint32_t trailing_ones64(uint64_t x) { return Internal::ctz64(~x); }
static inline uint32_t popcount32(uint32_t x) { return Internal::popcount32(x); }
#else
#error "Implement me."
#endif

template <typename T>
inline void for_each_bit64(uint64_t value, const T &func)
{
	while (value)
	{
		uint32_t bit = trailing_zeroes64(value);
		func(bit);
		value &= ~(1ull << bit);
	}
}

template <typename T>
inline void for_each_bit(uint32_t value, const T &func)
{
	while (value)
	{
		uint32_t bit = trailing_zeroes(value);
		func(bit);
		value &= ~(1u << bit);
	}
}

template <typename T>
inline void for_each_bit_range(uint32_t value, const T &func)
{
	if (value == ~0u)
	{
		func(0, 32);
		return;
	}

	uint32_t bit_offset = 0;
	while (value)
	{
		uint32_t bit = trailing_zeroes(value);
		bit_offset += bit;
		value >>= bit;
		uint32_t range = trailing_ones(value);
		func(bit_offset, range);
		value &= ~((1u << range) - 1);
	}
}

template <typename T>
inline bool is_pow2(T value)
{
	return (value & (value - T(1))) == T(0);
}

inline uint32_t next_pow2(uint32_t v)
{
	v--;
	v |= v >> 16;
	v |= v >> 8;
	v |= v >> 4;
	v |= v >> 2;
	v |= v >> 1;
	return v + 1;
}

inline uint32_t prev_pow2(uint32_t v)
{
	return next_pow2(v + 1) >> 1;
}

inline uint32_t floor_log2(uint32_t v)
{
	return 31 - leading_zeroes(v);
}
}
