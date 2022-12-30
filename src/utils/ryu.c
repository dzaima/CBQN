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
void ryu_init() { }
#else

#include "../core.h"
#include "ryu/ryu_common.h"

// Include either the small or the full lookup tables depending on the mode.
#if defined(RYU_OPTIMIZE_SIZE)
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
#if defined(RYU_OPTIMIZE_SIZE)
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
#if defined(RYU_OPTIMIZE_SIZE)
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


static B fmt_nan, fmt_inf[2], fmt_zero[2];
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
    if (mantissa) r = fmt_nan;
    if (exponent) r = fmt_inf[sign];
    else          r = fmt_zero[sign];
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

void ryu_init() {
  gc_add(fmt_nan = m_c8vec("NaN", 3));
  gc_add(fmt_zero[0] = m_c8vec("0", 1));
  { u16* d; B c = m_c16arrv(&d, 1);            d[0]=U'∞'; gc_add(c); fmt_inf[0]  = c; }
  { u16* d; B c = m_c16arrv(&d, 2); d[0]=U'¯'; d[1]=U'∞'; gc_add(c); fmt_inf[1]  = c; }
  { u8*  d; B c = m_c8arrv (&d, 2); d[0]=U'¯'; d[1]=U'0'; gc_add(c); fmt_zero[1] = c; }
}
#endif
