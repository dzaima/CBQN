#include "../core.h"
#include "../builtins.h"

#if SINGELI
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-variable"
  #include "../singeli/gen/fold.c"
  #pragma GCC diagnostic pop
#endif

static bool fold_ne(u64* x, u64 am) {
  u64 r = 0;
  for (u64 i = 0; i < (am>>6); i++) r^= x[i];
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

// Try to keep to i32 product, go to f64 on overflow or non-i32 initial
#define DEF_INT_PROD(T) \
  static f64 prod_##T(void* xv, usz i, f64 init) {           \
    while (i--) init*=((T*)xv)[i]; return init;              \
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
  static f64 min_f64(void* xv, usz ia) { return avx2_fold_min_f64(xv,ia); }
  static f64 max_f64(void* xv, usz ia) { return avx2_fold_max_f64(xv,ia); }
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
  if (ia==0) {
    decG(x);
    if (isFun(f)) {
      B r = TI(f,identity)(f);
      if (!q_N(r)) return inc(r);
    }
    thrM("´: No identity found");
  }
  u8 xe = TI(x,elType);
  if (isFun(f) && v(f)->flags) {
    u8 rtid = v(f)->flags-1;
    if (rtid==n_ltack) { B r = IGet(x, 0   ); decG(x); return r; }
    if (rtid==n_rtack) { B r = IGet(x, ia-1); decG(x); return r; }
    if (xe>el_f64) goto base;
    if (xe==el_bit) {
      u64* xp = bitarr_ptr(x);
      f64 r;
      switch (rtid) { default: goto base;
        case n_add:                           r = bit_sum (xp, ia);           break;
        case n_sub:                           r = bit_diff(xp, ia);           break;
        case n_and: case n_mul: case n_floor: r = bit_has (xp, ia, 0) ^ 1;    break;
        case n_or:              case n_ceil:  r = bit_has (xp, ia, 1)    ;    break;
        case n_ne:                            r = fold_ne (xp, ia)          ; break;
        case n_eq:                            r = fold_ne (xp, ia) ^ (1&~ia); break;
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
      u8 sel = xe - el_i8;
      f64 r = xe<=el_i32 ? prod_int_fns[sel](xv, ia, 1)
                         : prod_fns[sel](xv, ia, 1);
      decG(x); return m_f64(r);
    }
    if (rtid==n_or) { // ∨
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); bool r=0; for (usz i=0; i<ia; i++) { i8  c=xp[i]; if (c!=0&&c!=1)goto base; r|=c; } decG(x); return m_i32(r); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); bool r=0; for (usz i=0; i<ia; i++) { i16 c=xp[i]; if (c!=0&&c!=1)goto base; r|=c; } decG(x); return m_i32(r); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); bool r=0; for (usz i=0; i<ia; i++) { i32 c=xp[i]; if (c!=0&&c!=1)goto base; r|=c; } decG(x); return m_i32(r); }
    }
  }
  base:;
  SLOW2("𝕎´ 𝕩", f, x);
  
  SGet(x)
  BBB2B fc2 = c2fn(f);
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
  u8 xe = TI(x,elType);
  if (isFun(f) && v(f)->flags) {
    u8 rtid = v(f)->flags-1;
    if (rtid==n_ltack) {
      B r = w;
      if (ia) { dec(w); r=IGet(x, 0); }
      decG(x); return r;
    }
    if (rtid==n_rtack) { decG(x); return w; }
    if (!isF64(w) || xe>el_f64) goto base;
    f64 wf = w.f;
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
    if (rtid==n_floor) { f64 r=wf; if (ia>0) { f64 m=min_fns[xe-el_i8](tyany_ptr(x), ia); if (m<r) r=m; } decG(x); return m_f64(r); } // ⌊
    if (rtid==n_ceil ) { f64 r=wf; if (ia>0) { f64 m=max_fns[xe-el_i8](tyany_ptr(x), ia); if (m>r) r=m; } decG(x); return m_f64(r); } // ⌈
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
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); bool q=wi; for (usz i=0; i<ia; i++) { i8  c=xp[i]; if (c!=0&&c!=1)goto base; q|=c; } decG(x); return m_i32(q); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); bool q=wi; for (usz i=0; i<ia; i++) { i16 c=xp[i]; if (c!=0&&c!=1)goto base; q|=c; } decG(x); return m_i32(q); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); bool q=wi; for (usz i=0; i<ia; i++) { i32 c=xp[i]; if (c!=0&&c!=1)goto base; q|=c; } decG(x); return m_i32(q); }
    }
  }
  base:;
  SLOW3("𝕨 F´ 𝕩", w, x, f);
  
  B c = w;
  SGet(x)
  BBB2B fc2 = c2fn(f);
  for (usz i = ia; i>0; i--) c = fc2(f, Get(x, i-1), c);
  decG(x);
  return c;
}
