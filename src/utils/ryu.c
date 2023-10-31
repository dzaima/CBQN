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

#define INT_SCIENTIFIC_START 1e15 // integer scientific representation start; only applies to integers with 2⋆53>|𝕩
#define POS_SCIENTIFIC_START   15 // positive scientific representation start; as-is can't be greater than 15 because only the small_int case provides trailing zeroes
#define NEG_SCIENTIFIC_START    5 // negative scientific representation start

#if NO_RYU
void ryu_init(void) { }
#else

// original file: d2s.h
#include "../core.h"
#include "ryu/ryu_common.h"

// Include either the small or the full lookup tables depending on the mode.
#if RYU_OPTIMIZE_SIZE
  #include "ryu/d2s_small_table.h"
#else
  #include "ryu/d2s_full_table.h"
#endif

#define DOUBLE_MANTISSA_BITS 52
#define DOUBLE_EXPONENT_BITS 11
#define DOUBLE_BIAS 1023

// A floating decimal representing m * 10^e.
typedef struct floating_decimal_64 {
  uint64_t mantissa;
  // Decimal exponent's range is -324 to 308
  // inclusive, and can fit in a short if needed.
  int32_t exponent;
} floating_decimal_64;

static inline floating_decimal_64 d2d(const uint64_t mantissa, const uint32_t exponent) {
  int32_t e2;
  uint64_t m2;
  if (exponent == 0) {
    // We subtract 2 so that the bounds computation has 2 additional bits.
    e2 = 1 - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS - 2;
    m2 = mantissa;
  } else {
    e2 = (int32_t) exponent - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS - 2;
    m2 = (1ull << DOUBLE_MANTISSA_BITS) | mantissa;
  }
  const bool even = (m2 & 1) == 0;
  const bool acceptBounds = even;

#ifdef RYU_DEBUG
  printf("-> %" PRIu64 " * 2^%d\n", m2, e2 + 2);
#endif

  // Step 2: Determine the interval of valid decimal representations.
  const uint64_t mv = 4 * m2;
  const uint32_t mmShift = mantissa!=0 || exponent<=1; // Implicit bool -> int conversion. True is 1, false is 0.
  // We would compute mp and mm like this:
  // uint64_t mp = 4 * m2 + 2;
  // uint64_t mm = mv - 1 - mmShift;

  // Step 3: Convert to a decimal power base using 128-bit arithmetic.
  uint64_t vr, vp, vm;
  int32_t e10;
  bool vmIsTrailingZeros = false;
  bool vrIsTrailingZeros = false;
  if (e2 >= 0) {
    // I tried special-casing q == 0, but there was no effect on performance.
    // This expression is slightly faster than max(0, log10Pow2(e2) - 1).
    const uint32_t q = log10Pow2(e2) - (e2 > 3);
    e10 = (int32_t) q;
    const int32_t k = DOUBLE_POW5_INV_BITCOUNT + pow5bits((int32_t) q) - 1;
    const int32_t i = -e2 + (int32_t) q + k;
#if RYU_OPTIMIZE_SIZE
    uint64_t pow5[2];
    double_computeInvPow5(q, pow5);
    vr = mulShiftAll64(m2, pow5, i, &vp, &vm, mmShift);
#else
    vr = mulShiftAll64(m2, DOUBLE_POW5_INV_SPLIT[q], i, &vp, &vm, mmShift);
#endif
#ifdef RYU_DEBUG
    printf("%" PRIu64 " * 2^%d / 10^%u\n", mv, e2, q);
    printf("V+=%" PRIu64 "\nV =%" PRIu64 "\nV-=%" PRIu64 "\n", vp, vr, vm);
#endif
    if (q <= 21) {
      // This should use q <= 22, but I think 21 is also safe. Smaller values
      // may still be safe, but it's more difficult to reason about them.
      // Only one of mp, mv, and mm can be a multiple of 5, if any.
      const uint32_t mvMod5 = ((uint32_t) mv) - 5 * ((uint32_t) div5(mv));
      if (mvMod5 == 0) {
        vrIsTrailingZeros = multipleOfPowerOf5(mv, q);
      } else if (acceptBounds) {
        // Same as min(e2 + (~mm & 1), pow5Factor(mm)) >= q
        // <=> e2 + (~mm & 1) >= q && pow5Factor(mm) >= q
        // <=> true && pow5Factor(mm) >= q, since e2 >= q.
        vmIsTrailingZeros = multipleOfPowerOf5(mv - 1 - mmShift, q);
      } else {
        // Same as min(e2 + 1, pow5Factor(mp)) >= q.
        vp -= multipleOfPowerOf5(mv + 2, q);
      }
    }
  } else {
    // This expression is slightly faster than max(0, log10Pow5(-e2) - 1).
    const uint32_t q = log10Pow5(-e2) - (-e2 > 1);
    e10 = (int32_t) q + e2;
    const int32_t i = -e2 - (int32_t) q;
    const int32_t k = pow5bits(i) - DOUBLE_POW5_BITCOUNT;
    const int32_t j = (int32_t) q - k;
#if RYU_OPTIMIZE_SIZE
    uint64_t pow5[2];
    double_computePow5(i, pow5);
    vr = mulShiftAll64(m2, pow5, j, &vp, &vm, mmShift);
#else
    vr = mulShiftAll64(m2, DOUBLE_POW5_SPLIT[i], j, &vp, &vm, mmShift);
#endif
#ifdef RYU_DEBUG
    printf("%" PRIu64 " * 5^%d / 10^%u\n", mv, -e2, q);
    printf("%u %d %d %d\n", q, i, k, j);
    printf("V+=%" PRIu64 "\nV =%" PRIu64 "\nV-=%" PRIu64 "\n", vp, vr, vm);
#endif
    if (q <= 1) {
      // {vr,vp,vm} is trailing zeros if {mv,mp,mm} has at least q trailing 0 bits.
      // mv = 4 * m2, so it always has at least two trailing 0 bits.
      vrIsTrailingZeros = true;
      if (acceptBounds) {
        // mm = mv - 1 - mmShift, so it has 1 trailing 0 bit iff mmShift == 1.
        vmIsTrailingZeros = mmShift == 1;
      } else {
        // mp = mv + 2, so it always has at least one trailing 0 bit.
        --vp;
      }
    } else if (q < 63) { // TODO(ulfjack): Use a tighter bound here.
      // We want to know if the full product has at least q trailing zeros.
      // We need to compute min(p2(mv), p5(mv) - e2) >= q
      // <=> p2(mv) >= q && p5(mv) - e2 >= q
      // <=> p2(mv) >= q (because -e2 >= q)
      vrIsTrailingZeros = multipleOfPowerOf2(mv, q);
#ifdef RYU_DEBUG
      printf("vr is trailing zeros=%s\n", vrIsTrailingZeros ? "true" : "false");
#endif
    }
  }
#ifdef RYU_DEBUG
  printf("e10=%d\n", e10);
  printf("V+=%" PRIu64 "\nV =%" PRIu64 "\nV-=%" PRIu64 "\n", vp, vr, vm);
  printf("vm is trailing zeros=%s\n", vmIsTrailingZeros ? "true" : "false");
  printf("vr is trailing zeros=%s\n", vrIsTrailingZeros ? "true" : "false");
#endif

  // Step 4: Find the shortest decimal representation in the interval of valid representations.
  int32_t removed = 0;
  uint8_t lastRemovedDigit = 0;
  uint64_t output;
  // On average, we remove ~2 digits.
  if (vmIsTrailingZeros || vrIsTrailingZeros) {
    // General case, which happens rarely (~0.7%).
    for (;;) {
      const uint64_t vpDiv10 = div10(vp);
      const uint64_t vmDiv10 = div10(vm);
      if (vpDiv10 <= vmDiv10) {
        break;
      }
      const uint32_t vmMod10 = ((uint32_t) vm) - 10 * ((uint32_t) vmDiv10);
      const uint64_t vrDiv10 = div10(vr);
      const uint32_t vrMod10 = ((uint32_t) vr) - 10 * ((uint32_t) vrDiv10);
      vmIsTrailingZeros &= vmMod10 == 0;
      vrIsTrailingZeros &= lastRemovedDigit == 0;
      lastRemovedDigit = (uint8_t) vrMod10;
      vr = vrDiv10;
      vp = vpDiv10;
      vm = vmDiv10;
      ++removed;
    }
#ifdef RYU_DEBUG
    printf("V+=%" PRIu64 "\nV =%" PRIu64 "\nV-=%" PRIu64 "\n", vp, vr, vm);
    printf("d-10=%s\n", vmIsTrailingZeros ? "true" : "false");
#endif
    if (vmIsTrailingZeros) {
      for (;;) {
        const uint64_t vmDiv10 = div10(vm);
        const uint32_t vmMod10 = ((uint32_t) vm) - 10 * ((uint32_t) vmDiv10);
        if (vmMod10 != 0) {
          break;
        }
        const uint64_t vpDiv10 = div10(vp);
        const uint64_t vrDiv10 = div10(vr);
        const uint32_t vrMod10 = ((uint32_t) vr) - 10 * ((uint32_t) vrDiv10);
        vrIsTrailingZeros &= lastRemovedDigit == 0;
        lastRemovedDigit = (uint8_t) vrMod10;
        vr = vrDiv10;
        vp = vpDiv10;
        vm = vmDiv10;
        ++removed;
      }
    }
#ifdef RYU_DEBUG
    printf("%" PRIu64 " %d\n", vr, lastRemovedDigit);
    printf("vr is trailing zeros=%s\n", vrIsTrailingZeros ? "true" : "false");
#endif
    if (vrIsTrailingZeros && lastRemovedDigit == 5 && vr % 2 == 0) {
      // Round even if the exact number is .....50..0.
      lastRemovedDigit = 4;
    }
    // We need to take vr + 1 if vr is outside bounds or we need to round up.
    output = vr + ((vr == vm && (!acceptBounds || !vmIsTrailingZeros)) || lastRemovedDigit >= 5);
  } else {
    // Specialized for the common case (~99.3%). Percentages below are relative to this.
    bool roundUp = false;
    const uint64_t vpDiv100 = div100(vp);
    const uint64_t vmDiv100 = div100(vm);
    if (vpDiv100 > vmDiv100) { // Optimization: remove two digits at a time (~86.2%).
      const uint64_t vrDiv100 = div100(vr);
      const uint32_t vrMod100 = ((uint32_t) vr) - 100 * ((uint32_t) vrDiv100);
      roundUp = vrMod100 >= 50;
      vr = vrDiv100;
      vp = vpDiv100;
      vm = vmDiv100;
      removed += 2;
    }
    // Loop iterations below (approximately), without optimization above:
    // 0: 0.03%, 1: 13.8%, 2: 70.6%, 3: 14.0%, 4: 1.40%, 5: 0.14%, 6+: 0.02%
    // Loop iterations below (approximately), with optimization above:
    // 0: 70.6%, 1: 27.8%, 2: 1.40%, 3: 0.14%, 4+: 0.02%
    for (;;) {
      const uint64_t vpDiv10 = div10(vp);
      const uint64_t vmDiv10 = div10(vm);
      if (vpDiv10 <= vmDiv10) {
        break;
      }
      const uint64_t vrDiv10 = div10(vr);
      const uint32_t vrMod10 = ((uint32_t) vr) - 10 * ((uint32_t) vrDiv10);
      roundUp = vrMod10 >= 5;
      vr = vrDiv10;
      vp = vpDiv10;
      vm = vmDiv10;
      ++removed;
    }
#ifdef RYU_DEBUG
    printf("%" PRIu64 " roundUp=%s\n", vr, roundUp ? "true" : "false");
    printf("vr is trailing zeros=%s\n", vrIsTrailingZeros ? "true" : "false");
#endif
    // We need to take vr + 1 if vr is outside bounds or we need to round up.
    output = vr + (vr == vm || roundUp);
  }
  const int32_t exp = e10 + removed;

#ifdef RYU_DEBUG
  printf("V+=%" PRIu64 "\nV =%" PRIu64 "\nV-=%" PRIu64 "\n", vp, vr, vm);
  printf("O=%" PRIu64 "\n", output);
  printf("EXP=%d\n", exp);
#endif

  floating_decimal_64 fd;
  fd.exponent = exp;
  fd.mantissa = output;
  return fd;
}





#define W2D(RES, NUM) memcpy((RES), DIGIT_TABLE + (NUM)*2, 2)
#define W4D(RES, NUM) ({ char* r4_ = (RES); AUTO n4_ = (NUM); W2D(r4_, n4_/100  ); W2D(r4_+2, n4_%100  ); })

#ifdef __clang__ // https://bugs.llvm.org/show_bug.cgi?id=38217
#define mod1e4(X) ((X) - 10000 * ((X)/10000))
#else
#define mod1e4(X) ((X)%10000)
#endif

static inline bool d2d_small_int(const uint64_t mantissa, const uint32_t exponent, floating_decimal_64* const v) {
  const uint64_t m2 = (1ull << DOUBLE_MANTISSA_BITS) | mantissa;
  const int32_t e2 = (int32_t) exponent - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;
  
  if (e2 > 0) return false; // f = m2 * 2^e2 >= 2^53 is an integer. Ignore this case for now.
  if (e2 < -52) return false; // f < 1.
  
  // Since 2^52 <= m2 < 2^53 and 0 <= -e2 <= 52: 1 <= f = m2 / 2^-e2 < 2^53.
  // Test if the lower -e2 bits of the significand are 0, i.e. whether the fraction is 0.
  const uint64_t mask = (1ull << -e2) - 1;
  const uint64_t fraction = m2 & mask;
  if (fraction != 0) return false;
  
  // f is an integer in the range [1, 2^53).
  v->mantissa = m2 >> -e2; // Note: mantissa contains trailing (decimal) 0's for numbers with such.
  v->exponent = 0;
  return true;
}

static inline B to_chars(const floating_decimal_64 v, const bool sign, const bool forcePositional) {
  char buf[25];
  char* dec = buf+25;
  
  // decode all digits to temporary buffer
  uint64_t o64 = v.mantissa;
  if ((o64 >> 32) != 0) {
    uint64_t q = div1e8(o64);
    uint32_t o32 = ((uint32_t) o64) - 100000000 * ((uint32_t) q);
    o64 = q;
    
    uint32_t c = o32%10000; o32/= 10000;
    uint32_t d = o32%10000;
    W4D(dec-4, c);
    W4D(dec-8, d);
    dec-= 8;
  }
  uint32_t o32 = (uint32_t) o64;
  while (o32 >= 10000) { uint32_t c = mod1e4(o32); o32/= 10000; dec-= 4; W4D(dec, c); }
  if    (o32 >= 100  ) { uint32_t c = o32%100;     o32/= 100;   dec-= 2; W2D(dec, c); }
  if    (o32 >= 10   ) { uint32_t c = o32;                      dec-= 2; W2D(dec, c); }
  else *(--dec) = '0'+o32;
  
  int dlen = buf+25 - dec; // number of digits
  int32_t expRaw = v.exponent; // exponent relative to decimal mantissa
  int exp = expRaw+dlen-1; // proper decimal exponent
#if RYU_DEBUG
  printf("mantissa=" N64u " exp=%d expRaw=%d dlen=%d\n", v.mantissa, exp, expRaw, dlen);
#endif
  
  bool positional = forcePositional;
  positional|= exp>-NEG_SCIENTIFIC_START && exp<POS_SCIENTIFIC_START;
  
  int rlen = dlen + sign;
  if (positional) {
    rlen+= exp<0? 1-exp : expRaw!=0; // exp<0? 0.00000001234 : expRaw==0? 1234 : 12.34
    // 123400000 is handled by caller by already having trailing zeroes in mantissa and expRaw==0
  } else {
    int expAbs = exp;
    rlen+= dlen!=1; // 1e99 vs 1.2e99
    if (expAbs<0) { expAbs = -expAbs; rlen++; } // 1e9 vs 1e¯9
    rlen+= expAbs>=100? 4 : expAbs>=10? 3 : 2; // 1e9 vs 1e99 vs 1e299
  }
  
  u8* rp;
  B r = m_c8arrv(&rp, rlen);
  if (sign) rp++[0] = U'¯';
  
  if (positional) {
    if (exp<0) {
      rp[0] = '0';
      rp[1] = '.';
      PLAINLOOP for (int i = 0; i < -exp-1; i++) rp[i+2] = '0';
      memcpy(rp-exp+1, dec, dlen);
    } else {
      memcpy(rp, dec, exp+1);
      if (expRaw!=0) {
        rp[exp+1] = '.';
        memcpy(rp+exp+2, dec+exp+1, dlen-exp-1);
      }
    }
  } else {
    rp[0] = dec[0];
    int epos;
    if (dlen!=1) {
      rp[1] = '.';
      memcpy(rp+2, dec+1, dlen-1);
      epos = dlen+1;
    } else epos = dlen;
    rp[epos] = 'e';
    
    if (exp<0) { exp = -exp; rp[++epos] = U'¯'; }
    
    if     (exp>=100) { W2D(rp+epos+1, exp/10); rp[epos+3] = '0' + exp%10; }
    else if (exp>=10) { W2D(rp+epos+1, exp); }
    else rp[epos+1] = '0' + exp;
  }
  
  return r;
}

static B fmt_nan, fmt_zero;
static B fmt_inf[2];
B ryu_d2s(double f) {
  const uint64_t bits = double_to_bits(f); // decode the floating-point number, and unify normalized and subnormal cases.
  
#ifdef RYU_DEBUG
  printf("IN=");
  for (int32_t bit = 64; bit-->0; ) printf("%d", (int) ((bits >> bit) & 1));
  printf("\n");
#endif
  
  // Decode bits into sign, mantissa, and exponent.
  const bool sign = ((bits >> (DOUBLE_MANTISSA_BITS + DOUBLE_EXPONENT_BITS)) & 1) != 0;
  const uint64_t mantissa = bits & ((1ull << DOUBLE_MANTISSA_BITS) - 1);
  const uint32_t exponent = (uint32_t) ((bits >> DOUBLE_MANTISSA_BITS) & ((1u << DOUBLE_EXPONENT_BITS) - 1));
  // Case distinction; exit early for the easy cases.
  if (exponent == ((1u << DOUBLE_EXPONENT_BITS) - 1u) || (exponent == 0 && mantissa == 0)) {
    B r;
    if (mantissa)      r = fmt_nan;
    else if (exponent) r = fmt_inf[sign];
    else               r = fmt_zero;
    return incG(r);
  }
  
  floating_decimal_64 v;
  
  // For small numbers, we want non-scientific.
  // isSmallInt handles the case of trailing zeroes, everything else is just moving "." & "e" around
  bool isSmallInt = d2d_small_int(mantissa, exponent, &v); // "small" ≡ <2⋆53
  bool forcePositional;
  if (isSmallInt) {
    forcePositional = v.mantissa < (uint64_t)(INT_SCIENTIFIC_START);
    if (!forcePositional) {
      for (;;) { // remove trailing zeroes, as to_chars doesn't handle those unless forcePositional
        uint64_t q = div10(v.mantissa);
        uint32_t r = (uint32_t)v.mantissa - 10 * (uint32_t)q;
        if (r!=0) break;
        v.mantissa = q;
        ++v.exponent;
      }
    }
  } else {
    forcePositional = false;
    v = d2d(mantissa, exponent);
  }
  
  return to_chars(v, sign, forcePositional);
}






// original file: s2d.h
#if defined(_MSC_VER)
  #include <intrin.h>
  static inline uint32_t floor_log2(const uint64_t value) {
    long index;
    return _BitScanReverse64(&index, value) ? index : 64;
  }
#else
  static inline uint32_t floor_log2(const uint64_t value) { return 63 - CLZ(value); }
#endif

// The max function is already defined on Windows.
static inline int32_t max32(int32_t a, int32_t b) { return a < b ? b : a; }

static inline double int64Bits2Double(uint64_t bits) {
  double f;
  memcpy(&f, &bits, sizeof(double));
  return f;
}

bool ryu_s2d_n(u8* buffer, int len, f64* result) {
  assert(len>0 && len<(1<<20)); // max length so that '0.0000[a billion zeroes]0001e1000000000' doesn't have to be handled
  int m10digits = 0;
  int dotIndex = len;
  int eIndex = len;
  uint64_t m10 = 0;
  int32_t e10 = 0;
  int32_t offset = 0;
  bool signedM = false;
  bool signedE = false;
  
  int i = 0;
  if (buffer[i] == '-') {
    signedM = true;
    i++;
  }
  int i1 = i;
  for (; i < len; i++) {
    char c = buffer[i];
    if (c == '.') {
      if (dotIndex!=len) return false; // "12.34.56"
      dotIndex = i;
      continue;
    }
    if ((c<'0') || (c>'9')) break;
    if (m10digits < 18) {
      m10 = 10*m10 + (c-'0');
      if (m10!=0) m10digits++;
    } else offset++;
  }
  if (i-i1<=1) {
    if (i==i1) return false; // "-", "e2", "-e2"
    if (i1==dotIndex) return false; // ".", ".e2"
  }
  if (i<len && ((buffer[i]=='e') || (buffer[i]=='E'))) {
    eIndex = i;
    i++;
    if (i<len && ((buffer[i]=='-') || (buffer[i]=='+'))) {
      signedE = buffer[i] == '-';
      i++;
    }
    if (i >= len) return false; // "123e", "123e+"
    for (; i < len; i++) {
      char c = buffer[i];
      if ((c<'0') || (c>'9')) return false;
      
      // 1<<25 >> 2⋆20, 10×2⋆20 < 2⋆32; some leeway because why not, and also to make sure we're really out of the range where the exponent could in any way compensate a long input
      // else, just leave e10 as-is; continue loop to finish checking input validity, but an infinity or ±0
      // from e.g. ±"0e100000000000000" or "123e-100000000000000" will get output regardless of the precise e10 value
      if (e10 < (1<<25)) {
        e10 = 10*e10 + (c-'0');
      }
    }
  }
  if (i < len) return false; // "123hello"
  if (signedE) e10 = -e10;
  
  e10+= offset;
  e10-= dotIndex<eIndex? eIndex-dotIndex-1 : 0;
  
  if (m10 == 0) {
    zeroRes:
    *result = signedM ? -0.0 : 0.0;
    return true;
  }

#ifdef RYU_DEBUG
  printf("Input=%s\n", buffer);
  printf("m10digits = %d\n", m10digits);
  printf("m10 * 10^e10 = %" PRIu64 " * 10^%d\n", m10, e10);
#endif

  if ((m10digits + e10 <= -324) || (m10 == 0)) {
    goto zeroRes; // Number is less than 1e-324, which should be rounded down to 0; return +/-0.0.
  }
  if (m10digits + e10 >= 310) {
    // Number is larger than 1e+309, which should be rounded to +/-Infinity.
    uint64_t ieee = (((uint64_t) signedM) << (DOUBLE_EXPONENT_BITS + DOUBLE_MANTISSA_BITS)) | (0x7ffull << DOUBLE_MANTISSA_BITS);
    *result = int64Bits2Double(ieee);
    return true;
  }

  // Convert to binary float m2 * 2^e2, while retaining information about whether the conversion
  // was exact (trailingZeros).
  int32_t e2;
  uint64_t m2;
  bool trailingZeros;
  if (e10 >= 0) {
    // The length of m * 10^e in bits is:
    //   log2(m10 * 10^e10) = log2(m10) + e10 log2(10) = log2(m10) + e10 + e10 * log2(5)
    //
    // We want to compute the DOUBLE_MANTISSA_BITS + 1 top-most bits (+1 for the implicit leading
    // one in IEEE format). We therefore choose a binary output exponent of
    //   log2(m10 * 10^e10) - (DOUBLE_MANTISSA_BITS + 1).
    //
    // We use floor(log2(5^e10)) so that we get at least this many bits; better to
    // have an additional bit than to not have enough bits.
    e2 = floor_log2(m10) + e10 + log2pow5(e10) - (DOUBLE_MANTISSA_BITS + 1);

    // We now compute [m10 * 10^e10 / 2^e2] = [m10 * 5^e10 / 2^(e2-e10)].
    // To that end, we use the DOUBLE_POW5_SPLIT table.
    int j = e2 - e10 - ceil_log2pow5(e10) + DOUBLE_POW5_BITCOUNT;
    assert(j >= 0);
#if RYU_OPTIMIZE_SIZE
    uint64_t pow5[2];
    double_computePow5(e10, pow5);
    m2 = mulShift64(m10, pow5, j);
#else
    assert(e10 < DOUBLE_POW5_TABLE_SIZE);
    m2 = mulShift64(m10, DOUBLE_POW5_SPLIT[e10], j);
#endif
    // We also compute if the result is exact, i.e.,
    //   [m10 * 10^e10 / 2^e2] == m10 * 10^e10 / 2^e2.
    // This can only be the case if 2^e2 divides m10 * 10^e10, which in turn requires that the
    // largest power of 2 that divides m10 + e10 is greater than e2. If e2 is less than e10, then
    // the result must be exact. Otherwise we use the existing multipleOfPowerOf2 function.
    trailingZeros = e2 < e10 || (e2 - e10 < 64 && multipleOfPowerOf2(m10, e2 - e10));
  } else {
    e2 = floor_log2(m10) + e10 - ceil_log2pow5(-e10) - (DOUBLE_MANTISSA_BITS + 1);
    int j = e2 - e10 + ceil_log2pow5(-e10) - 1 + DOUBLE_POW5_INV_BITCOUNT;
#if RYU_OPTIMIZE_SIZE
    uint64_t pow5[2];
    double_computeInvPow5(-e10, pow5);
    m2 = mulShift64(m10, pow5, j);
#else
    assert(-e10 < DOUBLE_POW5_INV_TABLE_SIZE);
    m2 = mulShift64(m10, DOUBLE_POW5_INV_SPLIT[-e10], j);
#endif
    trailingZeros = multipleOfPowerOf5(m10, -e10);
  }

#ifdef RYU_DEBUG
  printf("m2 * 2^e2 = %" PRIu64 " * 2^%d\n", m2, e2);
#endif

  // Compute the final IEEE exponent.
  uint32_t ieee_e2 = (uint32_t) max32(0, e2 + DOUBLE_BIAS + floor_log2(m2));

  if (ieee_e2 > 0x7fe) {
    // Final IEEE exponent is larger than the maximum representable; return +/-Infinity.
    uint64_t ieee = (((uint64_t) signedM) << (DOUBLE_EXPONENT_BITS + DOUBLE_MANTISSA_BITS)) | (0x7ffull << DOUBLE_MANTISSA_BITS);
    *result = int64Bits2Double(ieee);
    return true;
  }

  // We need to figure out how much we need to shift m2. The tricky part is that we need to take
  // the final IEEE exponent into account, so we need to reverse the bias and also special-case
  // the value 0.
  int32_t shift = (ieee_e2 == 0 ? 1 : ieee_e2) - e2 - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;
  assert(shift >= 0);
#ifdef RYU_DEBUG
  printf("ieee_e2 = %d\n", ieee_e2);
  printf("shift = %d\n", shift);
#endif
  
  // We need to round up if the exact value is more than 0.5 above the value we computed. That's
  // equivalent to checking if the last removed bit was 1 and either the value was not just
  // trailing zeros or the result would otherwise be odd.
  //
  // We need to update trailingZeros given that we have the exact output exponent ieee_e2 now.
  trailingZeros &= (m2 & ((1ull << (shift - 1)) - 1)) == 0;
  uint64_t lastRemovedBit = (m2 >> (shift - 1)) & 1;
  bool roundUp = (lastRemovedBit != 0) && (!trailingZeros || (((m2 >> shift) & 1) != 0));

#ifdef RYU_DEBUG
  printf("roundUp = %d\n", roundUp);
  printf("ieee_m2 = %" PRIu64 "\n", (m2 >> shift) + roundUp);
#endif
  uint64_t ieee_m2 = (m2 >> shift) + roundUp;
  assert(ieee_m2 <= (1ull << (DOUBLE_MANTISSA_BITS + 1)));
  ieee_m2 &= (1ull << DOUBLE_MANTISSA_BITS) - 1;
  if (ieee_m2 == 0 && roundUp) {
    // Due to how the IEEE represents +/-Infinity, we don't need to check for overflow here.
    ieee_e2++;
  }
  
  uint64_t ieee = (((((uint64_t) signedM) << DOUBLE_EXPONENT_BITS) | (uint64_t)ieee_e2) << DOUBLE_MANTISSA_BITS) | ieee_m2;
  *result = int64Bits2Double(ieee);
  return true;
}

void ryu_init(void) {
  gc_add(fmt_nan = m_c8vec("NaN", 3));
  gc_add(fmt_zero = m_c8vec("0", 1));
  { u16* d; B c = m_c16arrv(&d, 1);            d[0]=U'∞'; gc_add(c); fmt_inf[0]  = c; }
  { u16* d; B c = m_c16arrv(&d, 2); d[0]=U'¯'; d[1]=U'∞'; gc_add(c); fmt_inf[1]  = c; }
}
#endif
