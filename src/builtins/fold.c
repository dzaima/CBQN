#include "../core.h"
#include "../builtins.h"

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
  if (isFun(f) && v(f)->flags && xe<=el_f64) {
    u8 rtid = v(f)->flags-1;
    if (xe==el_bit) {
      u64* xp = bitarr_ptr(x);
      if (rtid==n_add) { B r = m_f64(bit_sum (xp, ia)); decG(x); return r; }
      if (rtid==n_sub) { B r = m_f64(bit_diff(xp, ia)); decG(x); return r; }
      if (rtid==n_and | rtid==n_mul | rtid==n_floor) { B r = m_i32(!bit_has(xp, ia, 0)); decG(x); return r; }
      if (rtid==n_or  |               rtid==n_ceil ) { B r = m_i32( bit_has(xp, ia, 1)); decG(x); return r; }
      if (rtid==n_ne) { bool r=fold_ne(xp, ia)          ; decG(x); return m_i32(r); }
      if (rtid==n_eq) { bool r=fold_ne(xp, ia) ^ (1&~ia); decG(x); return m_i32(r); }
      goto base;
    }
    if (rtid==n_add) { // +
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); i64 c=0; for (usz i=0; i<ia; i++) c+= xp[i];                    decG(x); return m_f64(c); } // won't worry about 64TB array sum float inaccuracy for now
      if (xe==el_i16) { i16* xp = i16any_ptr(x); i32 c=0; for (usz i=0; i<ia; i++) if (addOn(c,xp[i]))goto base; decG(x); return m_i32(c); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); i32 c=0; for (usz i=0; i<ia; i++) if (addOn(c,xp[i]))goto base; decG(x); return m_i32(c); }
      if (xe==el_f64) { f64* xp = f64any_ptr(x); f64 c=0; for (usz i=ia; i--; ) c+= xp[i];                       decG(x); return m_f64(c); }
    }
    if (rtid==n_mul | rtid==n_and) { // ×/∧
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); i32 c=1; for (usz i=ia; i--; ) if (mulOn(c,xp[i]))goto base; decG(x); return m_f64(c); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); i32 c=1; for (usz i=ia; i--; ) if (mulOn(c,xp[i]))goto base; decG(x); return m_i32(c); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); i32 c=1; for (usz i=ia; i--; ) if (mulOn(c,xp[i]))goto base; decG(x); return m_i32(c); }
      if (xe==el_f64) { f64* xp = f64any_ptr(x); f64 c=1; for (usz i=ia; i--; ) c*= xp[i];                    decG(x); return m_f64(c); }
    }
    if (rtid==n_floor) { // ⌊
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); i8  c=I8_MAX ; for (usz i=0; i<ia; i++) if (xp[i]<c) c=xp[i]; decG(x); return m_i32(c); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); i16 c=I16_MAX; for (usz i=0; i<ia; i++) if (xp[i]<c) c=xp[i]; decG(x); return m_i32(c); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); i32 c=I32_MAX; for (usz i=0; i<ia; i++) if (xp[i]<c) c=xp[i]; decG(x); return m_i32(c); }
    }
    if (rtid==n_ceil) { // ⌈
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); i8  c=I8_MIN ; for (usz i=0; i<ia; i++) if (xp[i]>c) c=xp[i]; decG(x); return m_i32(c); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); i16 c=I16_MIN; for (usz i=0; i<ia; i++) if (xp[i]>c) c=xp[i]; decG(x); return m_i32(c); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); i32 c=I32_MIN; for (usz i=0; i<ia; i++) if (xp[i]>c) c=xp[i]; decG(x); return m_i32(c); }
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
  if (q_i32(w) && isFun(f) && v(f)->flags && elInt(xe)) {
    i32 wi = o2iG(w);
    u8 rtid = v(f)->flags-1;
    if (xe==el_bit) {
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
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); i64 c=wi; for (usz i=0; i<ia; i++) c+=xp[i];                     decG(x); return m_f64(c); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); i32 c=wi; for (usz i=0; i<ia; i++) if (addOn(c,xp[i]))goto base; decG(x); return m_i32(c); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); i32 c=wi; for (usz i=0; i<ia; i++) if (addOn(c,xp[i]))goto base; decG(x); return m_i32(c); }
    }
    if (rtid==n_mul | rtid==n_and) { // ×/∧
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); i32 c=wi; for (usz i=ia; i--; ) if (mulOn(c,xp[i]))goto base; decG(x); return m_i32(c); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); i32 c=wi; for (usz i=ia; i--; ) if (mulOn(c,xp[i]))goto base; decG(x); return m_i32(c); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); i32 c=wi; for (usz i=ia; i--; ) if (mulOn(c,xp[i]))goto base; decG(x); return m_i32(c); }
    }
    if (rtid==n_floor) { // ⌊
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); i32 c=wi; for (usz i=0; i<ia; i++) if (xp[i]<c) c=xp[i]; decG(x); return m_i32(c); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); i32 c=wi; for (usz i=0; i<ia; i++) if (xp[i]<c) c=xp[i]; decG(x); return m_i32(c); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); i32 c=wi; for (usz i=0; i<ia; i++) if (xp[i]<c) c=xp[i]; decG(x); return m_i32(c); }
    }
    if (rtid==n_ceil) { // ⌈
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); i32 c=wi; for (usz i=0; i<ia; i++) if (xp[i]>c) c=xp[i]; decG(x); return m_i32(c); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); i32 c=wi; for (usz i=0; i<ia; i++) if (xp[i]>c) c=xp[i]; decG(x); return m_i32(c); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); i32 c=wi; for (usz i=0; i<ia; i++) if (xp[i]>c) c=xp[i]; decG(x); return m_i32(c); }
    }
    if (rtid==n_or && (wi&1)==wi) { // ∨
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
