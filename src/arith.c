#include "h.h"
#include <math.h>

static inline B arith_recm(BB2B f, B x) {
  B fx = getFillQ(x);
  B r = eachm_fn(f, bi_N, x);
  return withFill(r, fx);
}
#ifdef CATCH_ERRORS
static inline B arith_recd(BBB2B f, B w, B x) {
  B fx = getFillQ(x);
  if (noFill(fx)) return eachd_fn(f, bi_N, w, x);
  B fw = getFillQ(w);
  B r = eachd_fn(f, bi_N, w, x);
  if (noFill(fw)) return r;
  if (CATCH) { dec(catchMessage); return r; }
  B fr = f(bi_N, fw, fx);
  popCatch();
  return withFill(r, asFill(fr));
}
#else
static inline B arith_recd(BBB2B f, B w, B x) {
  return eachd_fn(f, bi_N, w, x);
}
#endif


#define P1(N) { if(         isArr(x)) return arith_recm(N##_c1,    x); }
#define P2(N) { if(isArr(w)|isArr(x)) return arith_recd(N##_c2, w, x); }
#if TYPED_ARITH
  static B f64_maybe_i32(B x) {
    f64* xp = f64arr_ptr(x);
    usz ia = a(x)->ia;
    if (ia==0) { dec(x); return inc(bi_emptyIVec); }
    if (xp[0] != (i32)xp[0]) return x;
    i32* rp; B r = m_i32arrc(&rp, x);
    for (usz i = 0; i < ia; i++) {
      f64 cf = xp[i];
      i32 c = (i32)cf;
      if (cf!=c) { dec(r); return x; }
      rp[i] = c;
    }
    dec(x);
    return r;
  }
  #define ffnx(NAME, EXPR, EXTRA) B NAME##_c2(B t, B w, B x) {               \
    if (isF64(w) & isF64(x)) return m_f64(EXPR);                             \
    EXTRA                                                                    \
    if (isArr(w)|isArr(x)) { B ow=w; B ox=x;                                 \
      if (isArr(w)&isArr(x) && rnk(w)==rnk(x)) {                             \
        usz ia = a(x)->ia;                                                   \
        u8 we = TI(w).elType;                                                \
        u8 xe = TI(x).elType;                                                \
        if (isNumEl(we)&isNumEl(xe)) {                                       \
          f64* rp; B r = m_f64arrc(&rp, x);                                  \
          if (we==el_i32) { B w,x/*shadow*/; i32* wp = i32any_ptr(ow);       \
            if (xe==el_i32) { i32* xp = i32any_ptr(ox); for (usz i = 0; i < ia; i++) {w.f=wp[i];x.f=xp[i];rp[i]=EXPR;} } \
            else            { f64* xp = f64any_ptr(ox); for (usz i = 0; i < ia; i++) {w.f=wp[i];x.f=xp[i];rp[i]=EXPR;} } \
          } else {          B w,x/*shadow*/; f64* wp = f64any_ptr(ow);       \
            if (xe==el_i32) { i32* xp = i32any_ptr(ox); for (usz i = 0; i < ia; i++) {w.f=wp[i];x.f=xp[i];rp[i]=EXPR;} } \
            else            { f64* xp = f64any_ptr(ox); for (usz i = 0; i < ia; i++) {w.f=wp[i];x.f=xp[i];rp[i]=EXPR;} } \
          }                                                                  \
          dec(w); dec(x); return f64_maybe_i32(r);                           \
        }                                                                    \
      } else if (isF64(w)&isArr(x)) { usz ia = a(x)->ia;                     \
        u8 xe = TI(x).elType; f64*rp;                                        \
        if (xe==el_i32) { B r=m_f64arrc(&rp, x); i32*xp=i32any_ptr(x);       \
          for (usz i = 0; i < ia; i++) {B x/*shadow*/;x.f=xp[i];rp[i]=EXPR;} \
          dec(x); return f64_maybe_i32(r);                                   \
        }                                                                    \
        if (xe==el_f64) { B r=m_f64arrc(&rp, x); f64*xp=f64any_ptr(x);       \
          for (usz i = 0; i < ia; i++) {B x/*shadow*/;x.f=xp[i];rp[i]=EXPR;} \
          dec(x); return f64_maybe_i32(r);                                   \
        }                                                                    \
      } else if (isF64(x)&isArr(w)) { usz ia = a(w)->ia;                     \
        u8 we = TI(w).elType; f64*rp;                                        \
        if (we==el_i32) { B r=m_f64arrc(&rp, w); i32*wp=i32any_ptr(w);       \
          for (usz i = 0; i < ia; i++) {B w/*shadow*/;w.f=wp[i];rp[i]=EXPR;} \
          dec(w); return f64_maybe_i32(r);                                   \
        }                                                                    \
        if (we==el_f64) { B r=m_f64arrc(&rp, w); f64*wp=f64any_ptr(w);       \
          for (usz i = 0; i < ia; i++) {B w/*shadow*/;w.f=wp[i];rp[i]=EXPR;} \
          dec(w); return f64_maybe_i32(r);                                   \
        }                                                                    \
      }                                                                      \
      P2(NAME)                                                               \
    }                                                                        \
    thrM(#NAME ": invalid arithmetic");                                      \
  }
#else // if !TYPED_ARITH
  #define ffnx(name, expr, extra) B name##_c2(B t, B w, B x) { \
    if (isF64(w) & isF64(x)) return m_f64(expr); \
    extra \
    P2(name) \
    thrM(#name ": invalid arithmetic"); \
  }
#endif // TYPED_ARITH
#define ffn(name, op, extra) ffnx(name, w.f op x.f, extra)

f64 pfmod(f64 a, f64 b) {
  f64 r = fmod(a, b);
  if (a<0 != b<0 && r!=0) r+= b;
  return r;
}

ffn(add, +, {
  if (isC32(w) & isF64(x)) { u64 r = (u64)(o2cu(w)+o2i64(x)); if(r>CHR_MAX)thrM("+: Invalid character"); return m_c32((u32)r); }
  if (isF64(w) & isC32(x)) { u64 r = (u64)(o2cu(x)+o2i64(w)); if(r>CHR_MAX)thrM("+: Invalid character"); return m_c32((u32)r); }
})
ffn(sub, -, {
  if (isC32(w) & isF64(x)) { u64 r = (u64)(o2cu(w)-o2u64(x)); if(r>CHR_MAX)thrM("-: Invalid character"); return m_c32((u32)r); }
  if (isC32(w) & isC32(x)) return m_f64((i32)(u32)w.u - (i32)(u32)x.u);
})
ffn(mul, *, {})
ffn(and, *, {})
ffn(div, /, {})
ffnx(pow  ,       pow(w.f, x.f), {})
ffnx(floor,      fmin(w.f, x.f), {})
ffnx(ceil ,      fmax(w.f, x.f), {})
ffnx(stile,     pfmod(x.f, w.f), {})
ffnx(log  ,   log(x.f)/log(w.f), {})
ffnx(or   , (w.f+x.f)-(w.f*x.f), {})
ffnx(not  , 1+w.f-x.f          , {})

#define CMP(X, N, G) \
  ffn(N, X, { \
    if (isC32(w) & isC32(x)) return m_f64(w.u X x.u); \
    if (isF64(w) & isC32(x)) return m_f64(1-G); \
    if (isC32(w) & isF64(x)) return m_f64(G); \
  })
CMP(<=, le, 0)
CMP(>=, ge, 1)
CMP(< , lt, 0)
CMP(> , gt, 1)
#undef CMP

#undef ffn
#undef ffnx

B decp_c1(B t, B x);
B eq_c2(B t, B w, B x) {
  if(isF64(w)&isF64(x)) return m_i32(w.f==x.f);
  P2(eq);
  B r = m_i32(atomEqual(w, x));
  dec(w); dec(x);
  return r;
}
B ne_c2(B t, B w, B x) {
  if(isF64(w)&isF64(x)) return m_i32(w.f!=x.f);
  P2(ne);
  B r = m_i32(!atomEqual(w, x));
  dec(w); dec(x);
  return r;
}

B rt_merge;
B gt_c1(B t, B x) {
  if (isAtm(x)) return x;
  usz xia = a(x)->ia;
  ur xr = rnk(x);
  if (xia==0) {
    B xf = getFillE(x);
    if (isAtm(xf)) { dec(xf); return x; }
    B r = m_fillarrp(0);
    i32 xfr = rnk(xf);
    fillarr_setFill(r, getFillQ(xf));
    if (xr+xfr > UR_MAX) thrM(">: Result rank too large");
    usz* rsh = arr_shAllocI(r, 0, xr+xfr);
    if (rsh) {
      memcpy       (rsh   , a(x)->sh,  xr *sizeof(usz));
      if(xfr)memcpy(rsh+xr, a(xf)->sh, xfr*sizeof(usz));
    }
    return r;
  }
  
  BS2B xgetU = TI(x).getU;
  B x0 = xgetU(x, 0);
  usz* elSh = isArr(x0)? a(x0)->sh : NULL;
  ur elR = isArr(x0)? rnk(x0) : 0;
  usz elIA = isArr(x0)? a(x0)->ia : 1;
  B fill = getFillQ(x0);
  if (xr+elR > UR_MAX) thrM(">: Result rank too large");
  
  MAKE_MUT(r, xia*elIA);
  usz rp = 0;
  for (usz i = 0; i < xia; i++) {
    B c = xgetU(x, i);
    if (isArr(c)? (elR!=rnk(c) || !eqShPrefix(elSh, a(c)->sh, elR)) : elR!=0) { mut_pfree(r, rp); thrF(">: Elements didn't have equal shapes (contained %H and %H)", x0, c); }
    if (isArr(c)) mut_copy(r, rp, c, 0, elIA);
    else mut_set(r, rp, c);
    if (!noFill(fill)) fill = fill_or(fill, getFillQ(c));
    rp+= elIA;
  }
  B rb = mut_fp(r);
  usz* rsh = arr_shAllocR(rb, xr+elR);
  if (rsh) {
    memcpy         (rsh   , a(x)->sh, xr *sizeof(usz));
    if (elSh)memcpy(rsh+xr, elSh,     elR*sizeof(usz));
  }
  dec(x);
  return withFill(rb,fill);
}

B   add_c1(B t, B x) { return x; }
B   sub_c1(B t, B x) { if (isF64(x)) return m_f64(     -x.f ); P1(  sub); thrM("-: Negating non-number"); }
B   not_c1(B t, B x) { if (isF64(x)) return m_f64(    1-x.f ); P1(  not); thrM("¬: Argument was not a number"); }
B   mul_c1(B t, B x) { if (isF64(x)) return m_f64(x.f==0?0:x.f>0?1:-1); P1(mul); thrM("×: Getting sign of non-number"); }
B   div_c1(B t, B x) { if (isF64(x)) return m_f64(    1/x.f ); P1(  div); thrM("÷: Getting reciprocal of non-number"); }
B   pow_c1(B t, B x) { if (isF64(x)) return m_f64(  exp(x.f)); P1(  pow); thrM("⋆: Getting exp of non-number"); }
B floor_c1(B t, B x) { if (isF64(x)) return m_f64(floor(x.f)); P1(floor); thrM("⌊: Argument was not a number"); }
B  ceil_c1(B t, B x) { if (isF64(x)) return m_f64( ceil(x.f)); P1( ceil); thrM("⌈: Argument was not a number"); }
B stile_c1(B t, B x) { if (isF64(x)) return m_f64( fabs(x.f)); P1(stile); thrM("|: Argument was not a number"); }
B   log_c1(B t, B x) { if (isF64(x)) return m_f64(  log(x.f)); P1(  log); thrM("⋆⁼: Getting log of non-number"); }

B lt_c1(B t, B x) { return m_unit(x); }
B eq_c1(B t, B x) { B r = m_i32(isArr(x)? rnk(x) : 0); decR(x); return r; }
B ne_c1(B t, B x) { B r = m_f64(isArr(x)&&rnk(x)? *a(x)->sh : 1); decR(x); return r; }

B rt_sortAsc, rt_sortDsc;
B and_c1(B t, B x) { return c1(rt_sortAsc, x); }
B  or_c1(B t, B x) { return c1(rt_sortDsc, x); }

#undef P1
#undef P2

#define F(A,M,D) A(add) A(sub) A(mul) A(div) A(pow) A(floor) A(ceil) A(stile) A(eq) A(ne) D(le) D(ge) A(lt) A(gt) A(and) A(or) A(not) A(log)
BI_FNS0(F);
static inline void arith_init() { BI_FNS1(F)
  c(BFn,bi_add)->ident = c(BFn,bi_sub)->ident = c(BFn,bi_or )->ident = c(BFn,bi_ne)->ident = c(BFn,bi_gt)->ident = m_i32(0);
  c(BFn,bi_mul)->ident = c(BFn,bi_div)->ident = c(BFn,bi_and)->ident = c(BFn,bi_eq)->ident = c(BFn,bi_ge)->ident = c(BFn,bi_pow)->ident = c(BFn,bi_not)->ident = m_i32(1);
  c(BFn,bi_floor)->ident = m_f64(1.0/0.0);
  c(BFn,bi_ceil )->ident = m_f64(-1.0/0.0);
}
#undef F
