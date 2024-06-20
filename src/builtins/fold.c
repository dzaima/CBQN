// Fold (´) and Insert (˝)

// Fold optimized operands:
// ⊣⊢ on all types
// +-∧∨=≠ and synonyms on booleans
// ≤<>≥ on booleans, monadic only, with a search
// +⌈⌊× on numbers
//   Integer +: sum blocks associatively as long as sum can't exceed +-2⋆53
//   COULD implement fast numeric -´
// ∨ on boolean-valued integers, stopping at 1

// •math.Sum: +´ with faster and more precise SIMD code for i32, f64

// Insert with rank (˝˘ or ˝⎉k), or fold on flat array
// SHOULD optimize dyadic insert with rank
// Length 1, ⊣⊢: implemented as ⊏˘
// SHOULD reshape identity for fast ˝˘ on empty rows
// Boolean operand, cell size 1:
//   +: popcount
//     Rows length ≤64: extract rows, popcount each
//       TRIED scan-based version, faster for width 3 only
//       COULD have bit-twiddling versions of +˝˘ on very short rows
//     >64: popcount, boundary corrections (clang auto-vectorizes)
//   ∨∧≠= and synonyms, rows <64: extract from scan or windowed op
//     Dedicated auto-vectorizing code for sizes 2, 4
//     Extract is generic with multiplies or BMI2 (SHOULD add SIMD)
//     COULD use CLMUL for faster windowed ≠
//   ∨∧ and synonyms:
//     Multiples of 8 <256: comparison function, recurse if needed
//     Rows 64<l<128: popcount+compare (linearity makes boundaries cleaner)
//     ≥128: word-at-a-time search (SHOULD use SIMD if larger)
//   ≠=:
//     Multiples of 8 >24: read word, mask, last bit of popcount
//     Rows l<64: word-based scan, correct with SWAR
//     ≥64: xor words (auto-vectorizes), correct boundary, popcount
//   COULD implement boolean -˝˘ with xor, +˝˘, offset
// Arithmetic on rank 2, short rows: transpose then insert, blocked
//   SHOULD extend transpose-insert code to any frame and cell rank
//   SHOULD have dedicated +⌊⌈ insert with rank

#include "../core.h"
#include "../builtins.h"
#include "../utils/mut.h"
#include "../utils/calls.h"

#if SINGELI
  extern uint64_t* const si_spaced_masks;
  #define get_spaced_mask(i) si_spaced_masks[i-1]
  #define SINGELI_FILE fold
  #include "../utils/includeSingeli.h"
#endif

static u64 xor_words(u64* x, u64 l) {
  u64 r = 0;
  for (u64 i = 0; i < l; i++) r^= x[i];
  return r;
}
static bool fold_ne(u64* x, u64 am) {
  u64 r = xor_words(x, am>>6);
  if (am&63) r^= x[am>>6]<<(64-am & 63);
  return POPC(r) & 1;
}

static i64 bit_diff(u64* x, u64 am) {
  i64 r = 0;
  u64 a = 0xAAAAAAAAAAAAAAAA;
  for (u64 i = 0; i < (am>>6); i++) r+= POPC(x[i]^a);
  if (am&63) r+= POPC((x[am>>6]^a)<<(64-am & 63));
  return r - (i64)(am/2);
}

// It's safe to sum a block of integers as long as the current total
// is far enough from +-1ull<<53 (and integer, in dyadic fold).
static const usz sum_small_max = 1<<16;
#define DEF_INT_SUM(T,W,M,A) \
  static i64 sum_small_##T(void* xv, usz ia) {                   \
    i##A s=0; for (usz i=0; i<ia; i++) s+=((T*)xv)[i]; return s; \
  }                                                              \
  static f64 sum_##T(void* xv, usz ia, f64 init) {               \
    usz b=1<<(M-W); i64 lim = (1ull<<53) - (1ull<<M);            \
    T* xp = xv;                                                  \
    f64 r=init; i64 c=init; usz i0=ia;                           \
    if (c == init) {                                             \
      while (i0>0 && -lim<=c && c<=lim) {                        \
        usz e=i0; i0=(i0-1)&~(b-1);                              \
        c+=sum_small_##T(xp+i0, e-i0);                           \
      }                                                          \
      r = c;                                                     \
    }                                                            \
    while (i0--) r+=xp[i0];                                      \
    return r;                                                    \
  }
DEF_INT_SUM(i8 ,8 ,32,32)
DEF_INT_SUM(i16,16,32,32)
DEF_INT_SUM(i32,32,52,64)
#undef DEF_SUM
static f64 sum_f64(void* xv, usz i, f64 r) {
  while (i--) r += ((f64*)xv)[i];
  return r;
}
static i64 (*const sum_small_fns[])(void*, usz) = { sum_small_i8, sum_small_i16, sum_small_i32 };
static f64 (*const sum_fns[])(void*, usz, f64) = { sum_i8, sum_i16, sum_i32, sum_f64 };

B sum_c1(B t, B x) {
  if (isAtm(x) || RNK(x)!=1) thrF("•math.Sum: Argument must be a list (%H ≡ ≢𝕩)", x);
  usz ia = IA(x);
  if (ia==0) { decG(x); return m_f64(0); }
  u8 xe = TI(x,elType);
  if (!elNum(xe)) {
    x = any_squeeze(x); xe = TI(x,elType);
    if (!elNum(xe)) thrF("•math.Sum: Argument elements must be numbers", x);
  }
  f64 r;
  void* xv = tyany_ptr(x);
  if (xe == el_bit) {
    r = bit_sum(xv, ia);
  } else if (xe <= el_i32) {
    u8 sel = xe - el_i8;
    i64 s = 0; r = 0;
    i64 m = 1ull<<48;
    usz b = sum_small_max;
    for (usz i=0; i<ia; i+=b) {
      s += sum_small_fns[sel]((u8*)xv + (i<<sel), ia-i<b? ia-i : b);
      if (s >=  m) { r+=m; s-=m; }
      if (s <= -m) { r-=m; s+=m; }
    }
    r += s;
  } else {
    #if SINGELI
      r = si_sum_f64(xv, ia);
    #else
      r=0; for (usz i=0; i<ia; i++) r+=((f64*)xv)[i];
    #endif
  }
  decG(x); return m_f64(r);
}

// Try to keep to i32 product, go to f64 on overflow or non-i32 initial
#define DEF_INT_PROD(T) \
  static NOINLINE f64 prod_##T(void* xv, usz i, f64 init) {  \
    while (i--) { init*=((T*)xv)[i]; }  return init;         \
  }                                                          \
  static f64 prod_int_##T(void* xv, usz ia, i32 init) {      \
    T* xp = xv;                                              \
    while (ia--) {                                           \
      i32 i0=init;                                           \
      if (mulOn(init,xp[ia])) return prod_##T(xv, ia+1, i0); \
    }                                                        \
    return init;                                             \
  }
DEF_INT_PROD(i8)
DEF_INT_PROD(i16)
DEF_INT_PROD(i32)
#undef DEF_PROD
static f64 prod_f64(void* xv, usz i, f64 r) {
  while (i--) r *= ((f64*)xv)[i];
  return r;
}
static f64 (*const prod_int_fns[])(void*, usz, i32) = { prod_int_i8, prod_int_i16, prod_int_i32 };
static f64 (*const prod_fns[])(void*, usz, f64) = { prod_i8, prod_i16, prod_i32, prod_f64 };

#define MIN_MAX(T,C) \
  T* xp = xv; T r = xp[0]; \
  for (usz i=1; i<ia; i++) if (xp[i] C r) r=xp[i]; \
  return r;
#define DEF_MIN_MAX(T) \
  static f64 min_##T(void* xv, usz ia) { MIN_MAX(T,<) } \
  static f64 max_##T(void* xv, usz ia) { MIN_MAX(T,>) }
DEF_MIN_MAX(i8) DEF_MIN_MAX(i16) DEF_MIN_MAX(i32)
#if SINGELI
  static f64 min_f64(void* xv, usz ia) { return si_fold_min_f64(xv,ia); }
  static f64 max_f64(void* xv, usz ia) { return si_fold_max_f64(xv,ia); }
#else
  DEF_MIN_MAX(f64)
#endif
#undef DEF_MIN_MAX
#undef MIN_MAX
static f64 (*const min_fns[])(void*, usz) = { min_i8, min_i16, min_i32, min_f64 };
static f64 (*const max_fns[])(void*, usz) = { max_i8, max_i16, max_i32, max_f64 };

B fold_c1(Md1D* d, B x) { B f = d->f;
  if (isAtm(x) || RNK(x)!=1) thrF("´: Argument must be a list (%H ≡ ≢𝕩)", x);
  usz ia = IA(x);
  if (ia<=2) {
    if (ia==2) {
      SGet(x)
      B x0 = Get(x,0);
      B x1 = Get(x,1);
      decG(x);
      return c2(f, x0, x1);
    } else if (ia==1) {
      B r = IGet(x,0);
      decG(x);
      return r;
    } else {
      decG(x);
      if (isFun(f)) {
        B r = TI(f,identity)(f);
        if (!q_N(r)) return r;
      }
      thrM("´: Identity not found");
    }
  }
  if (RARE(!isFun(f))) { decG(x); if (isMd(f)) thrM("Calling a modifier"); return inc(f); }
  u8 xe = TI(x,elType);
  if (v(f)->flags) {
    u8 rtid = v(f)->flags-1;
    if (rtid==n_ltack) { B r = IGet(x, 0   ); decG(x); return r; }
    if (rtid==n_rtack) { B r = IGet(x, ia-1); decG(x); return r; }
    if (xe>el_f64) goto base;
    if (xe==el_bit) {
      u64* xp = bitarr_ptr(x);
      f64 r;
      switch (rtid) { default: goto base;
        case n_mul: 
        case n_and:case n_floor: r = bit_has (xp, ia, 0) ^ 1;     break;
        case n_or: case n_ceil:  r = bit_has (xp, ia, 1);         break;
        case n_add:              r = bit_sum (xp, ia);            break;
        case n_sub:              r = bit_diff(xp, ia);            break;
        case n_ne:               r = fold_ne (xp, ia);            break;
        case n_eq:               r = fold_ne (xp, ia) ^ (1&~ia);  break;
        case n_lt:               r = bit_find(xp, ia, 1) == ia-1; break;
        case n_le:               r = bit_find(xp, ia, 0) != ia-1; break;
        case n_gt:               r = bit_find(xp, ia, 0) & 1;     break;
        case n_ge:               r =~bit_find(xp, ia, 1) & 1;     break;
      }
      decG(x); return m_f64(r);
    }
    if (rtid==n_add) { // +
      void *xv = tyany_ptr(x);
      bool small = xe<=el_i32 & ia<=sum_small_max;
      u8 sel = xe - el_i8;
      f64 r = small ? sum_small_fns[sel](xv, ia)
                    : sum_fns[sel](xv, ia, 0);
      decG(x); return m_f64(r);
    }
    if (rtid==n_floor) { f64 r=min_fns[xe-el_i8](tyany_ptr(x), ia); decG(x); return m_f64(r); } // ⌊
    if (rtid==n_ceil ) { f64 r=max_fns[xe-el_i8](tyany_ptr(x), ia); decG(x); return m_f64(r); } // ⌈
    if (rtid==n_mul | rtid==n_and) { // ×/∧
      void *xv = tyany_ptr(x);
      assert(xe >= el_i8);
      u8 sel = xe - el_i8;
      f64 r = xe<=el_i32 ? prod_int_fns[sel](xv, ia, 1)
                         : prod_fns[sel](xv, ia, 1);
      decG(x); return m_f64(r);
    }
    if (rtid==n_or) { // ∨
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); usz i=ia; while (i--) { i8  c=xp[i]; if (c==1) break; if (c!=0) goto base; } decG(x); return m_i32(i+1 > 0); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); usz i=ia; while (i--) { i16 c=xp[i]; if (c==1) break; if (c!=0) goto base; } decG(x); return m_i32(i+1 > 0); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); usz i=ia; while (i--) { i32 c=xp[i]; if (c==1) break; if (c!=0) goto base; } decG(x); return m_i32(i+1 > 0); }
    }
  }
  base:;
  SLOW2("𝕎´ 𝕩", f, x);
  
  SGet(x)
  FC2 fc2 = c2fn(f);
  B c;
  if (TI(x,elType)==el_i32) {
    i32* xp = i32any_ptr(x);
    c = m_i32(xp[ia-1]);
    for (usz i = ia-1; i>0; i--) c = fc2(f, m_i32(xp[i-1]), c);
  } else {
    c = Get(x, ia-1);
    for (usz i = ia-1; i>0; i--) c = fc2(f, Get(x, i-1), c);
  }
  decG(x);
  return c;
}

B fold_c2(Md1D* d, B w, B x) { B f = d->f;
  if (isAtm(x) || RNK(x)!=1) thrF("´: 𝕩 must be a list (%H ≡ ≢𝕩)", x);
  usz ia = IA(x);
  if (RARE(ia==0)) { decG(x); return w; }
  if (RARE(!isFun(f))) { dec(w); decG(x); if (isMd(f)) thrM("Calling a modifier"); return inc(f); }
  
  u8 xe = TI(x,elType);
  if (v(f)->flags) {
    u8 rtid = v(f)->flags-1;
    if (rtid==n_ltack) {
      B r = IGet(x, 0);
      dec(w); decG(x); return r;
    }
    if (rtid==n_rtack) { decG(x); return w; }
    if (!isF64(w) || xe>el_f64) goto base;
    f64 wf = o2fG(w);
    if (xe==el_bit) {
      i32 wi = wf; if (wi!=wf) goto base;
      u64* xp = bitarr_ptr(x);
      if (rtid==n_add) { B r = m_f64(wi            + bit_sum (xp, ia)); decG(x); return r; }
      if (rtid==n_sub) { B r = m_f64((ia&1?-wi:wi) + bit_diff(xp, ia)); decG(x); return r; }
      if (wi!=(wi&1)) goto base;
      if (rtid==n_and | rtid==n_mul | rtid==n_floor) { B r = m_i32(wi && !bit_has(xp, ia, 0)); decG(x); return r; }
      if (rtid==n_or  |               rtid==n_ceil ) { B r = m_i32(wi ||  bit_has(xp, ia, 1)); decG(x); return r; }
      if (rtid==n_ne) { bool r=wi^fold_ne(xp, ia)         ; decG(x); return m_i32(r); }
      if (rtid==n_eq) { bool r=wi^fold_ne(xp, ia) ^ (1&ia); decG(x); return m_i32(r); }
      goto base;
    }
    if (rtid==n_add) { // +
      u8 sel = xe - el_i8;
      f64 r = sum_fns[sel](tyany_ptr(x), ia, wf);
      decG(x); return m_f64(r);
    }
    if (rtid==n_floor) { f64 r=wf; f64 m=min_fns[xe-el_i8](tyany_ptr(x), ia); if (m<r) r=m; decG(x); return m_f64(r); } // ⌊
    if (rtid==n_ceil ) { f64 r=wf; f64 m=max_fns[xe-el_i8](tyany_ptr(x), ia); if (m>r) r=m; decG(x); return m_f64(r); } // ⌈
    i32 wi = wf;
    if (rtid==n_mul | rtid==n_and) { // ×/∧
      void *xv = tyany_ptr(x);
      bool isint = xe<=el_i32 && wi==wf;
      u8 sel = xe - el_i8;
      f64 r = isint ? prod_int_fns[sel](xv, ia, wi)
                    : prod_fns[sel](xv, ia, wf);
      decG(x); return m_f64(r);
    }
    if (rtid==n_or && (wi&1)==wf) { // ∨
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); usz i=ia; if (!wi) while (i--) { i8  c=xp[i]; if (c==1) break; if (c!=0) goto base; } decG(x); return m_i32(i+1 > 0); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); usz i=ia; if (!wi) while (i--) { i16 c=xp[i]; if (c==1) break; if (c!=0) goto base; } decG(x); return m_i32(i+1 > 0); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); usz i=ia; if (!wi) while (i--) { i32 c=xp[i]; if (c==1) break; if (c!=0) goto base; } decG(x); return m_i32(i+1 > 0); }
    }
  }
  base:;
  SLOW3("𝕨 F´ 𝕩", w, x, f);
  
  B c = w;
  SGet(x)
  FC2 fc2 = c2fn(f);
  for (usz i = ia; i>0; i--) c = fc2(f, Get(x, i-1), c);
  decG(x);
  return c;
}

NOINLINE i64 bit_sum(u64* x, u64 am) {
  i64 r = 0;
  for (u64 i = 0; i < (am>>6); i++) r+= POPC(x[i]);
  if (am&63) r+= POPC(x[am>>6]<<(64-am & 63));
  return r;
}

NOINLINE static u64 usum_generic(B x, usz xia) {
  SGetU(x)
  u64 r = 0;
  for (usz i = 0; i < xia; i++) {
    u64 c = o2u64(GetU(x,i));
    if (addOn(r,c)) thrM("Sum too big");
  }
  return r;
}
u64 usum(B x) { // doesn't consume; will error on non-integers, or elements <0, or if sum overflows u64
  assert(isArr(x));
  u64 r = 0;
  usz xia = IA(x);
  u8 xe = TI(x,elType);
  if      (xe==el_bit) return bit_sum(bitarr_ptr(x), xia);
  else if (xe==el_i8 ) { i8*  p = i8any_ptr (x); i8  m=0; for (usz i = 0; i < xia; ) { usz b=1<< 8; i16 s=0; for (usz e = xia-i<b?xia:i+b; i < e; i++) { m|=p[i]; s+=p[i]; } if (m<0) goto neg; if (addOn(r,(u16)s)) goto overflow; } }
  else if (xe==el_i16) { i16* p = i16any_ptr(x); i16 m=0; for (usz i = 0; i < xia; ) { usz b=1<<16; i32 s=0; for (usz e = xia-i<b?xia:i+b; i < e; i++) { m|=p[i]; s+=p[i]; } if (m<0) goto neg; if (addOn(r,(u32)s)) goto overflow; } }
  else if (xe==el_i32) { i32* p = i32any_ptr(x); i32 m=0; for (usz i = 0; i < xia; i++) { m|=p[i]; if (addOn(r,p[i])) { if (m<0) goto neg; else goto overflow; } } if (m<0) goto neg; }
  else if (xe==el_f64) {
    f64* p = f64any_ptr(x);
    for (usz i = 0; i < xia; i++) {
      f64 c = p[i];
      if (!q_fu64(c)) expU_f64(c);
      u64 ci = (u64)c;
      if (ci<0) goto neg;
      if (addOn(r,ci)) goto overflow;
    }
  } else {
    return usum_generic(x, xia);
  }
  
  return r;
  
  overflow: thrM("Sum too big");
  neg: return usum_generic(x, xia); // ensure proper error message
}

B select_c1(B, B);
B select_c2(B, B, B);
static B m1c1(B t, B f, B x) { // consumes x
  B fn = m1_d(inc(t), inc(f));
  B r = c1(fn, x);
  decG(fn);
  return r;
}
extern GLOBAL B rt_insert;
extern B insert_base(B f, B x, bool has_w, B w); // from cells.c

B insert_c1(Md1D* d, B x) { B f = d->f;
  if (isAtm(x) || RNK(x)==0) thrM("˝: 𝕩 must have rank at least 1");
  usz len = *SH(x);
  if (len==0) { SLOW2("!𝕎˝𝕩", f, x); return m1c1(rt_insert, f, x); }
  if (len==1) return C1(select, x);
  if (RARE(!isFun(f))) { decG(x); if (isMd(f)) thrM("Calling a modifier"); return inc(f); }
  ur xr = RNK(x);
  if (xr==1 && isPervasiveDyExt(f)) return m_unit(fold_c1(d, x));
  if (v(f)->flags) {
    u8 rtid = v(f)->flags-1;
    if (rtid==n_ltack) return C1(select, x);
    if (rtid==n_rtack) return C2(select, m_f64(-1), x);
    if (rtid==n_join) {
      if (xr==1) return x;
      ShArr* rsh;
      if (xr>2) {
        rsh = m_shArr(xr-1);
        usz* xsh = SH(x);
        shcpy(rsh->a+1, xsh+2, xr-2);
        rsh->a[0] = xsh[0] * xsh[1];
      }
      Arr* r = TI(x,slice)(x, 0, IA(x));
      if (xr>2) arr_shSetUG(r, xr-1, rsh);
      else arr_shVec(r);
      return taga(r);
    }
  }
  return insert_base(f, x, 0, m_f64(0));
}
B insert_c2(Md1D* d, B w, B x) { B f = d->f;
  if (isAtm(x) || RNK(x)==0) thrM("˝: 𝕩 must have rank at least 1");
  if (*SH(x)==0) { decG(x); return w; }
  if (RARE(!isFun(f))) { dec(w); decG(x); if (isMd(f)) thrM("Calling a modifier"); return inc(f); }
  if (RNK(x)==1 && isPervasiveDyExt(f)) {
    if (isAtm(w)) {
      to_fold: return m_unit(fold_c2(d, w, x));
    }
    if (RNK(w)==0) {
      B w0=w; w = IGet(w,0); decG(w0);
      goto to_fold;
    }
  }
  if (v(f)->flags) {
    u8 rtid = v(f)->flags-1;
    if (rtid==n_ltack) { dec(w); return C1(select, x); }
    if (rtid==n_rtack) { decG(x); return w; }
  }
  return insert_base(f, x, 1, w);
}

// Arithmetic fold/insert on rows of flat rank-2 array x
B transp_c1(B, B);
B join_c2(B, B, B);
B fold_rows(Md1D* fd, B x) {
  assert(isArr(x) && RNK(x)==2);
  // Target block size trying to avoid power-of-two lengths, from:
  // {𝕩/˜⌊´⊸= +˝˘ +˝¬∨`2|>⌊∘÷⟜2⍟(↕12) ⌊0.5+32÷˜𝕩÷⌜1+↕64} +⟜↕2⋆16
  u64 block = (116053*8) >> arrTypeBitsLog(TY(x));
  if (TI(x,elType)==el_bit || IA(x)/2 <= block) {
    x = C1(transp, x);
    return insert_c1(fd, x);
  } else {
    usz *sh = SH(x); usz n = sh[0]; usz m = sh[1];
    usz b = (block + m - 1) / m; // Normal block length
    usz b_max = b + b/4;         // Last block max length
    MAKE_MUT(r, n); MUT_APPEND_INIT(r);
    BSS2A slice = TI(x,slice);
    for (usz i=0, im=0; i<n; ) {
      usz l = n-i; if (l > b_max) { incG(x); l = b; }
      usz sia = l * m;
      Arr* sl = slice(x, im, sia);
      usz* ssh = arr_shAlloc(sl, 2);
      ssh[0] = l;
      ssh[1] = m;
      B sr = insert_c1(fd, C1(transp, taga(sl)));
      MUT_APPEND(r, sr, 0, l);
      decG(sr);
      i += l; im += sia;
    }
    return mut_fv(r);
  }
}

B sum_rows_bit(B x, usz n, usz m) {
  u64* xp = bitarr_ptr(x);
  if (m < 128) {
    if (m == 2) return bi_N; // Transpose is faster
    i8* rp; B r = m_i8arrv(&rp, n);
    if (m <= 64) {
      if (m%8 == 0) {
        usz k = m/8; u64 b = (m==64? 0 : 1ull<<m)-1;
        for (usz i=0; i<n; i++) rp[i] = POPC(b & *(u64*)((u8*)xp+k*i));
      } else {
        if (m<=58 || m==60) {
          u64 b = (1ull<<m)-1;
          for (usz i=0, j=0; i<n; i++, j+=m) {
            u64 xw = *(u64*)((u8*)xp+j/8) >> (j%8);
            rp[i] = POPC(b & xw);
          }
        } else {
          // Row may not fit in an aligned word
          // Read a word containing the last bit, combine with saved bits
          u64 b = ~(~(u64)0 >> m);
          u64 prev = 0;
          for (usz i=0, j=m; i<n; i++, j+=m) {
            u64 xw = ((u64*)((u8*)xp+(j+7)/8))[-1];
            usz sh = (-j)%8;
            rp[i] = POPC(b & (xw<<sh | prev));
            prev = xw >> (m-sh);
          }
        }
      }
    } else { // 64<m<128, specialization of unaligned case below
      u64 o = 0;
      for (usz i=0, j=0; i<n; i++) {
        u64 in = (i+1)*m;
        u64 s = o + POPC(xp[j]);
        usz jn = in/64;
        u64 e = xp[jn];
        s += POPC(e & ((u64)j - (u64)jn)); // mask is 0 if j==jn, or -1
        o  = POPC(e >> (in%64));
        rp[i] = s - o;
        j = jn+1;
      }
    }
    decG(x); return r;
  } else if (m < 1<<15) {
    i16* rp; B r = m_i16arrv(&rp, n);
    usz l = m/64;
    if (m%64==0) {
      for (usz i=0; i<n; i++) rp[i] = bit_sum(xp+l*i, m);
    } else {
      u64 o = 0; // Carry
      for (usz i=0, j=0; i<n; i++) {
        u64 in = (i+1)*m; // bit index of start of next row
        u64 s = o + bit_sum(xp + j, 64*l);
        usz jn = in/64;
        u64 e = xp[jn];
        s += POPC(e &- (u64)(jn >= j+l));
        o  = POPC(e >> (in%64));
        rp[i] = s - o;
        j = jn+1;
      }
    }
    decG(x); return r;
  } else {
    return bi_N;
  }
}

// Fold n cells of size m, stride 1
// Return a vector regardless of argument shape, or bi_N if not handled
B fold_rows_bit(Md1D* fd, B x, usz n, usz m) {
  assert(isArr(x) && TI(x,elType)==el_bit && IA(x)==n*m);
  if (!v(fd->f)->flags) return bi_N;
  u8 rtid = v(fd->f)->flags-1;
  if (rtid==n_add) return sum_rows_bit(x, n, m);
  #if SINGELI
  bool is_or = rtid==n_or |rtid==n_ceil;
  bool andor = rtid==n_and|rtid==n_floor|rtid==n_mul|is_or;
  if (rtid==n_ne|rtid==n_eq|andor) {
    if (andor && m < 256) while (m%8 == 0) {
      usz f = CTZ(m|32);
      m >>= f; usz c = m*n;
      u64* yp; B y = m_bitarrv(&yp, c);
      u8 e = el_i8 + f-3;
      CmpASFn cmp = is_or ? CMP_AS_FN(ne, e) : CMP_AS_FN(eq, e);
      CMP_AS_CALL(cmp, yp, bitarr_ptr(x), m_f64(is_or-1), c);
      decG(x); if (m==1) return y;
      x = y;
    }
    u64* xp = bitarr_ptr(x);
    u64* rp; B r = m_bitarrv(&rp, n);
    if (andor) si_or_rows_bit(xp, rp, n, m, !is_or);
    else      si_xor_rows_bit(xp, rp, n, m, rtid==n_eq);
    decG(x); return r;
  }
  #endif
  return bi_N;
}
