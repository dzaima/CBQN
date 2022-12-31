// Copyright 2019 Ulf Adams
//
// The contents of this file may be used under the terms of the Apache License,
// Version 2.0.
//
//    (See accompanying file LICENSE-Apache or copy at
//     http://www.apache.org/licenses/LICENSE-2.0)
//
// Alternatively, the contents of this file may be used under the terms of
// the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE-Boost or copy at
//     https://www.boost.org/LICENSE_1_0.txt)
//
// Unless required by applicable law or agreed to in writing, this software
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.


// original file: digit_table.h
static const char DIGIT_TABLE[200] = {
  '0','0','0','1','0','2','0','3','0','4','0','5','0','6','0','7','0','8','0','9',
  '1','0','1','1','1','2','1','3','1','4','1','5','1','6','1','7','1','8','1','9',
  '2','0','2','1','2','2','2','3','2','4','2','5','2','6','2','7','2','8','2','9',
  '3','0','3','1','3','2','3','3','3','4','3','5','3','6','3','7','3','8','3','9',
  '4','0','4','1','4','2','4','3','4','4','4','5','4','6','4','7','4','8','4','9',
  '5','0','5','1','5','2','5','3','5','4','5','5','5','6','5','7','5','8','5','9',
  '6','0','6','1','6','2','6','3','6','4','6','5','6','6','6','7','6','8','6','9',
  '7','0','7','1','7','2','7','3','7','4','7','5','7','6','7','7','7','8','7','9',
  '8','0','8','1','8','2','8','3','8','4','8','5','8','6','8','7','8','8','8','9',
  '9','0','9','1','9','2','9','3','9','4','9','5','9','6','9','7','9','8','9','9'
};


// original file: common.h
#if defined(_M_IX86) || defined(_M_ARM)
#define RYU_32_BIT_PLATFORM
#endif

// Returns e == 0 ? 1 : [log_2(5^e)]; requires 0 <= e <= 3528.
static inline int32_t log2pow5(const int32_t e) {
  // This approximation works up to the point that the multiplication overflows at e = 3529.
  // If the multiplication were done in 64 bits, it would fail at 5^4004 which is just greater
  // than 2^9297.
  assert(e >= 0);
  assert(e <= 3528);
  return (int32_t) ((((uint32_t) e) * 1217359) >> 19);
}

// Returns e == 0 ? 1 : ceil(log_2(5^e)); requires 0 <= e <= 3528.
static inline int32_t pow5bits(const int32_t e) {
  // This approximation works up to the point that the multiplication overflows at e = 3529.
  // If the multiplication were done in 64 bits, it would fail at 5^4004 which is just greater
  // than 2^9297.
  assert(e >= 0);
  assert(e <= 3528);
  return (int32_t) (((((uint32_t) e) * 1217359) >> 19) + 1);
}

// Returns e == 0 ? 1 : ceil(log_2(5^e)); requires 0 <= e <= 3528.
static inline int32_t ceil_log2pow5(const int32_t e) {
  return log2pow5(e) + 1;
}

// Returns floor(log_10(2^e)); requires 0 <= e <= 1650.
static inline uint32_t log10Pow2(const int32_t e) {
  // The first value this approximation fails for is 2^1651 which is just greater than 10^297.
  assert(e >= 0);
  assert(e <= 1650);
  return (((uint32_t) e) * 78913) >> 18;
}

// Returns floor(log_10(5^e)); requires 0 <= e <= 2620.
static inline uint32_t log10Pow5(const int32_t e) {
  // The first value this approximation fails for is 5^2621 which is just greater than 10^1832.
  assert(e >= 0);
  assert(e <= 2620);
  return (((uint32_t) e) * 732923) >> 20;
}

static inline uint64_t double_to_bits(const double d) {
  uint64_t bits = 0;
  memcpy(&bits, &d, sizeof(double));
  return bits;
}



// original file: d2s_intrinsics.h
#if defined(__SIZEOF_INT128__) && !defined(_MSC_VER) && !defined(RYU_ONLY_64_BIT_OPS)
#define HAS_UINT128
#elif defined(_MSC_VER) && !defined(RYU_ONLY_64_BIT_OPS) && defined(_M_X64)
#define HAS_64_BIT_INTRINSICS
#endif

#if defined(HAS_UINT128)
typedef __uint128_t uint128_t;
#endif

#if defined(HAS_64_BIT_INTRINSICS)
  #include <intrin.h>
  static inline uint64_t umul128(const uint64_t a, const uint64_t b, uint64_t* const productHi) {
    return _umul128(a, b, productHi);
  }
  // Returns the lower 64 bits of (hi*2^64 + lo) >> dist, with 0 < dist < 64.
  static inline uint64_t shiftright128(const uint64_t lo, const uint64_t hi, const uint32_t dist) {
    assert(dist < 64);
    return __shiftright128(lo, hi, (unsigned char) dist);
  }
#else // defined(HAS_64_BIT_INTRINSICS)
  static inline uint64_t umul128(const uint64_t a, const uint64_t b, uint64_t* const productHi) {
    // The casts here help MSVC to avoid calls to the __allmul library function.
    const uint32_t aLo = (uint32_t)a; const uint32_t aHi = (uint32_t)(a >> 32);
    const uint32_t bLo = (uint32_t)b; const uint32_t bHi = (uint32_t)(b >> 32);
    const uint64_t b00 = (uint64_t)aLo * bLo; const uint64_t b01 = (uint64_t)aLo * bHi;
    const uint64_t b10 = (uint64_t)aHi * bLo; const uint64_t b11 = (uint64_t)aHi * bHi;
    const uint32_t b00Lo = (uint32_t)b00;
    const uint32_t b00Hi = (uint32_t)(b00 >> 32);
    
    const uint64_t mid1 = b10 + b00Hi;
    const uint32_t mid1Lo = (uint32_t)(mid1);
    const uint32_t mid1Hi = (uint32_t)(mid1 >> 32);
    
    const uint64_t mid2 = b01 + mid1Lo;
    const uint32_t mid2Lo = (uint32_t)(mid2);
    const uint32_t mid2Hi = (uint32_t)(mid2 >> 32);
    
    const uint64_t pHi = b11 + mid1Hi + mid2Hi;
    const uint64_t pLo = ((uint64_t)mid2Lo << 32) | b00Lo;
    
    *productHi = pHi;
    return pLo;
  }

  static inline uint64_t shiftright128(const uint64_t lo, const uint64_t hi, const uint32_t dist) {
    // We don't need to handle the case dist >= 64 here (see above).
    assert(dist < 64);
    assert(dist > 0);
    return (hi << (64 - dist)) | (lo >> dist);
  }
#endif // defined(HAS_64_BIT_INTRINSICS)

#if defined(RYU_32_BIT_PLATFORM)
  // Returns the high 64 bits of the 128-bit product of a and b.
  static inline uint64_t umulh(const uint64_t a, const uint64_t b) {
    // Reuse the umul128 implementation.
    // Optimizers will likely eliminate the instructions used to compute the
    // low part of the product.
    uint64_t hi;
    umul128(a, b, &hi);
    return hi;
  }
  static inline uint64_t div5(const uint64_t x) { return umulh(x, 0xCCCCCCCCCCCCCCCDu) >> 2; }
  static inline uint64_t div10(const uint64_t x) { return umulh(x, 0xCCCCCCCCCCCCCCCDu) >> 3; }
  static inline uint64_t div100(const uint64_t x) { return umulh(x >> 2, 0x28F5C28F5C28F5C3u) >> 2; }
  static inline uint64_t div1e8(const uint64_t x) { return umulh(x, 0xABCC77118461CEFDu) >> 26; }
  static inline uint64_t div1e9(const uint64_t x) { return umulh(x >> 9, 0x44B82FA09B5A53u) >> 11; }
  static inline uint32_t mod1e9(const uint64_t x) { return ((uint32_t) x) - 1000000000 * ((uint32_t) div1e9(x)); }
#else // defined(RYU_32_BIT_PLATFORM)
  static inline uint64_t div5(const uint64_t x) { return x / 5; }
  static inline uint64_t div10(const uint64_t x) { return x / 10; }
  static inline uint64_t div100(const uint64_t x) { return x / 100; }
  static inline uint64_t div1e8(const uint64_t x) { return x / 100000000; }
  static inline uint64_t div1e9(const uint64_t x) { return x / 1000000000; }
  static inline uint32_t mod1e9(const uint64_t x) { return (uint32_t) (x - 1000000000 * div1e9(x)); }
#endif // defined(RYU_32_BIT_PLATFORM)

static inline uint32_t pow5Factor(uint64_t value) {
  const uint64_t m_inv_5 = 14757395258967641293u; // 5 * m_inv_5 = 1 (mod 2^64)
  const uint64_t n_div_5 = 3689348814741910323u;  // #{ n | n = 0 (mod 2^64) } = 2^64 / 5
  uint32_t count = 0;
  for (;;) {
    assert(value != 0);
    value *= m_inv_5;
    if (value > n_div_5)
      break;
    ++count;
  }
  return count;
}

// Returns true if value is divisible by 5^p.
static inline bool multipleOfPowerOf5(const uint64_t value, const uint32_t p) {
  // I tried a case distinction on p, but there was no performance difference.
  return pow5Factor(value) >= p;
}

// Returns true if value is divisible by 2^p.
static inline bool multipleOfPowerOf2(const uint64_t value, const uint32_t p) {
  assert(value != 0);
  assert(p < 64);
  // __builtin_ctzll doesn't appear to be faster here.
  return (value & ((1ull << p) - 1)) == 0;
}

#if defined(HAS_UINT128)

// Best case: use 128-bit type.
static inline uint64_t mulShift64(const uint64_t m, const uint64_t* const mul, const int32_t j) {
  const uint128_t b0 = ((uint128_t) m) * mul[0];
  const uint128_t b2 = ((uint128_t) m) * mul[1];
  return (uint64_t) (((b0 >> 64) + b2) >> (j - 64));
}

static inline uint64_t mulShiftAll64(const uint64_t m, const uint64_t* const mul, const int32_t j, uint64_t* const vp, uint64_t* const vm, const uint32_t mmShift) {
  *vp = mulShift64(4 * m + 2, mul, j);
  *vm = mulShift64(4 * m - 1 - mmShift, mul, j);
  return mulShift64(4 * m, mul, j);
}

#elif defined(HAS_64_BIT_INTRINSICS)

static inline uint64_t mulShift64(const uint64_t m, const uint64_t* const mul, const int32_t j) {
  // m is maximum 55 bits
  uint64_t high1;                                   // 128
  const uint64_t low1 = umul128(m, mul[1], &high1); // 64
  uint64_t high0;                                   // 64
  umul128(m, mul[0], &high0);                       // 0
  const uint64_t sum = high0 + low1;
  if (sum < high0) {
    ++high1; // overflow into high1
  }
  return shiftright128(sum, high1, j - 64);
}

static inline uint64_t mulShiftAll64(const uint64_t m, const uint64_t* const mul, const int32_t j,
  uint64_t* const vp, uint64_t* const vm, const uint32_t mmShift) {
  *vp = mulShift64(4 * m + 2, mul, j);
  *vm = mulShift64(4 * m - 1 - mmShift, mul, j);
  return mulShift64(4 * m, mul, j);
}

#else // !defined(HAS_UINT128) && !defined(HAS_64_BIT_INTRINSICS)

static inline uint64_t mulShift64(const uint64_t m, const uint64_t* const mul, const int32_t j) {
  // m is maximum 55 bits
  uint64_t high1;                                   // 128
  const uint64_t low1 = umul128(m, mul[1], &high1); // 64
  uint64_t high0;                                   // 64
  umul128(m, mul[0], &high0);                       // 0
  const uint64_t sum = high0 + low1;
  if (sum < high0) {
    ++high1; // overflow into high1
  }
  return shiftright128(sum, high1, j - 64);
}

// This is faster if we don't have a 64x64->128-bit multiplication.
static inline uint64_t mulShiftAll64(uint64_t m, const uint64_t* const mul, const int32_t j,
  uint64_t* const vp, uint64_t* const vm, const uint32_t mmShift) {
  m <<= 1;
  // m is maximum 55 bits
  uint64_t tmp;
  const uint64_t lo = umul128(m, mul[0], &tmp);
  uint64_t hi;
  const uint64_t mid = tmp + umul128(m, mul[1], &hi);
  hi += mid < tmp; // overflow into hi

  const uint64_t lo2 = lo + mul[0];
  const uint64_t mid2 = mid + mul[1] + (lo2 < lo);
  const uint64_t hi2 = hi + (mid2 < mid);
  *vp = shiftright128(mid2, hi2, (uint32_t) (j - 64 - 1));

  if (mmShift == 1) {
    const uint64_t lo3 = lo - mul[0];
    const uint64_t mid3 = mid - mul[1] - (lo3 > lo);
    const uint64_t hi3 = hi - (mid3 > mid);
    *vm = shiftright128(mid3, hi3, (uint32_t) (j - 64 - 1));
  } else {
    const uint64_t lo3 = lo + lo;
    const uint64_t mid3 = mid + mid + (lo3 < lo);
    const uint64_t hi3 = hi + hi + (mid3 < mid);
    const uint64_t lo4 = lo3 - mul[0];
    const uint64_t mid4 = mid3 - mul[1] - (lo4 > lo3);
    const uint64_t hi4 = hi3 - (mid4 > mid3);
    *vm = shiftright128(mid4, hi4, (uint32_t) (j - 64));
  }

  return shiftright128(mid, hi, (uint32_t) (j - 64 - 1));
}

#endif // HAS_64_BIT_INTRINSICS
