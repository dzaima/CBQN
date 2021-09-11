#include "../core.h"
#include "../utils/each.h"
#include "../utils/mut.h"
#include "../utils/builtins.h"
#include "../utils/talloc.h"

static Arr* take_impl(usz ria, B x) { // consumes x; returns v↑⥊𝕩 without set shape; v is non-negative
  usz xia = a(x)->ia;
  if (ria>xia) {
    B xf = getFillE(x);
    MAKE_MUT(r, ria); mut_init(r, TI(x,elType));
    mut_copyG(r, 0, x, 0, xia);
    mut_fill(r, xia, xf, ria-xia);
    dec(x); dec(xf);
    return mut_fp(r);
  } else {
    return TI(x,slice)(x,0,ria);
  }
}

static B unitV1(B x) {
  if (isF64(x)) {
    i32 i = (i32)x.f;
    if (RARE(x.f != i))     { f64* rp; B r = m_f64arrv(&rp, 1); rp[0] = x.f; return r; }
    if      (x.f == (i8 )i) { i8*  rp; B r = m_i8arrv (&rp, 1); rp[0] = i; return r; }
    else if (x.f == (i16)i) { i16* rp; B r = m_i16arrv(&rp, 1); rp[0] = i; return r; }
    else                    { i32* rp; B r = m_i32arrv(&rp, 1); rp[0] = i; return r; }
  }
  if (isC32(x)) {
    u32 c = o2cu(x);
    if      (LIKELY(c<U8_MAX )) { u8*  rp; B r = m_c8arrv (&rp, 1); rp[0] = c; return r; }
    else if (LIKELY(c<U16_MAX)) { u16* rp; B r = m_c16arrv(&rp, 1); rp[0] = c; return r; }
    else                        { u32* rp; B r = m_c32arrv(&rp, 1); rp[0] = c; return r; }
  }
  Arr* ra = m_fillarrp(1); arr_shVec(ra);
  fillarr_setFill(ra, asFill(inc(x)));
  fillarr_ptr(ra)[0] = x;
  return taga(ra);
}

B shape_c1(B t, B x) {
  if (isAtm(x)) return unitV1(x);
  usz ia = a(x)->ia;
  if (ia==1 && TI(x,elType)<el_B) {
    B n = IGet(x,0);
    dec(x);
    return unitV1(n);
  }
  if (reusable(x)) {
    decSh(v(x)); arr_shVec(a(x));
    return x;
  }
  Arr* r = TI(x,slice)(x, 0, ia); arr_shVec(r);
  return taga(r);
}
B shape_c2(B t, B w, B x) {
  usz xia = isArr(x)? a(x)->ia : 1;
  usz nia = 1;
  ur nr;
  ShArr* sh;
  if (isF64(w)) {
    nia = o2s(w);
    nr = 1;
    sh = NULL;
  } else {
    if (isAtm(w)) w = m_atomUnit(w);
    if (rnk(w)>1) thrM("⥊: 𝕨 must have rank at most 1");
    if (a(w)->ia>UR_MAX) thrM("⥊: Result rank too large");
    nr = a(w)->ia;
    sh = nr<=1? NULL : m_shArr(nr);
    if (TI(w,elType)==el_i32) {
      i32* wi = i32any_ptr(w);
      if (nr>1) for (i32 i = 0; i < nr; i++) sh->a[i] = wi[i];
      bool bad=false, good=false;
      for (i32 i = 0; i < nr; i++) {
        if (wi[i]<0) thrF("⥊: 𝕨 contained %i", wi[i]);
        bad|= uszMul(&nia, wi[i]);
        good|= wi[i]==0;
      }
      if (bad && !good) thrM("⥊: 𝕨 too large");
    } else {
      SGetU(w)
      i32 unkPos = -1;
      i32 unkInd;
      bool bad=false, good=false;
      for (i32 i = 0; i < nr; i++) {
        B c = GetU(w, i);
        if (isF64(c)) {
          usz v = o2s(c);
          if (sh) sh->a[i] = v;
          bad|= uszMul(&nia, v);
          good|= v==0;
        } else {
          if (isArr(c) || !isVal(c)) thrM("⥊: 𝕨 must consist of natural numbers or ∘ ⌊ ⌽ ↑");
          if (unkPos!=-1) thrM("⥊: 𝕨 contained multiple computed axes");
          unkPos = i;
          unkInd = ((i32)v(c)->flags) - 1;
        }
      }
      if (bad && !good) thrM("⥊: 𝕨 too large");
      if (unkPos!=-1) {
        if (unkInd!=52 & unkInd!=6 & unkInd!=30 & unkInd!=25) thrM("⥊: 𝕨 must consist of natural numbers or ∘ ⌊ ⌽ ↑");
        if (nia==0) thrM("⥊: Can't compute axis when the rest of the shape is empty");
        i64 div = xia/nia;
        i64 mod = xia%nia;
        usz item;
        bool fill = false;
        if (unkInd == 52) {
          if (mod!=0) thrM("⥊: Shape must be exact when reshaping with ∘");
          item = div;
        } else if (unkInd == 6) {
          item = div;
        } else if (unkInd == 30) {
          item = mod? div+1 : div;
        } else if (unkInd == 25) {
          item = mod? div+1 : div;
          fill = true;
        } else UD;
        if (sh) sh->a[unkPos] = item;
        nia = uszMulT(nia, item);
        if (fill) {
          if (!isArr(x)) x = m_atomUnit(x);
          Arr* a = take_impl(nia, x);
          arr_shVec(a);
          x = taga(a);
          xia = nia;
        }
      }
    }
  }
  dec(w);
  
  B xf;
  if (isAtm(x)) {
    xf = asFill(inc(x));
    // goes to unit
  } else {
    if (nia <= xia) {
      B r; Arr* ra;
      if (reusable(x) && xia==nia) { r = x; decSh(v(x)); ra = (Arr*)v(r); }
      else { ra = TI(x,slice)(x, 0, nia); r = taga(ra); }
      arr_shSetU(ra, nr, sh);
      return r;
    } else {
      xf = getFillQ(x);
      if (xia<=1) {
        if (RARE(xia==0)) {
          thrM("⥊: Empty 𝕩 and non-empty result");
          // if (xf.u == bi_noFill.u) thrM("⥊: No fill for empty array");
          // dec(x);
          // x = inc(xf);
        } else {
          B n = IGet(x,0);
          dec(x);
          x = n;
        }
        goto unit;
      }
      
      MAKE_MUT(m, nia); mut_init(m, TI(x,elType));
      i64 div = nia/xia;
      i64 mod = nia%xia;
      for (i64 i = 0; i < div; i++) mut_copyG(m, i*xia, x, 0, xia);
      mut_copyG(m, div*xia, x, 0, mod);
      dec(x);
      Arr* ra = mut_fp(m);
      arr_shSetU(ra, nr, sh);
      return withFill(taga(ra), xf);
    }
  }
  
  unit:
  if (isF64(x)) { decA(xf);
    i32 n = (i32)x.f;
    if (RARE(n!=x.f))  { f64* rp; Arr* r = m_f64arrp(&rp,nia); arr_shSetU(r,nr,sh); for (u64 i=0; i<nia; i++) rp[i]=x.f; return taga(r); }
    else if(n==(i8 )n) { i8*  rp; Arr* r = m_i8arrp (&rp,nia); arr_shSetU(r,nr,sh); for (u64 i=0; i<nia; i++) rp[i]=n  ; return taga(r); }
    else if(n==(i16)n) { i16* rp; Arr* r = m_i16arrp(&rp,nia); arr_shSetU(r,nr,sh); for (u64 i=0; i<nia; i++) rp[i]=n  ; return taga(r); }
    else               { i32* rp; Arr* r = m_i32arrp(&rp,nia); arr_shSetU(r,nr,sh); for (u64 i=0; i<nia; i++) rp[i]=n  ; return taga(r); }
  }
  if (isC32(x)) { decA(xf);
    u32* rp; Arr* r = m_c32arrp(&rp, nia); arr_shSetU(r, nr, sh);
    u32 c = o2cu(x);
    for (u64 i = 0; i < nia; i++) rp[i] = c;
    return taga(r);
  }
  Arr* r = m_fillarrp(nia); arr_shSetU(r, nr, sh);
  B* rp = fillarr_ptr(r);
  if (nia) incBy(x, nia-1);
  else dec(x);
  for (u64 i = 0; i < nia; i++) rp[i] = x;
  fillarr_setFill(r, xf);
  return taga(r);
}

extern B rt_pick;
B pick_c1(B t, B x) {
  if (isAtm(x)) return x;
  if (RARE(a(x)->ia==0)) {
    thrM("⊑: Argument cannot be empty");
    // B r = getFillE(x);
    // dec(x);
    // return r;
  }
  B r = IGet(x, 0);
  dec(x);
  return r;
}
B pick_c2(B t, B w, B x) {
  if (isNum(w) && isArr(x) && rnk(x)==1) {
    usz p = WRAP(o2i64(w), a(x)->ia, thrF("⊑: indexing out-of-bounds (𝕨≡%R, %s≡≠𝕩)", w, iaW));
    B r = IGet(x, p);
    dec(x);
    return r;
  }
  return c2(rt_pick, w, x);
}

extern B rt_select;
B select_c1(B t, B x) {
  if (isAtm(x)) thrM("⊏: Argument cannot be an atom");
  ur xr = rnk(x);
  if (xr==0) thrM("⊏: Argument cannot be rank 0");
  if (a(x)->sh[0]==0) thrF("⊏: Argument shape cannot start with 0 (%H ≡ ≢𝕩)", x);
  usz ia = 1;
  for (i32 i = 1; i < xr; i++) ia*= a(x)->sh[i];
  Arr* r = TI(x,slice)(inc(x),0, ia);
  usz* sh = arr_shAlloc(r, xr-1);
  if (sh) for (i32 i = 1; i < xr; i++) sh[i-1] = a(x)->sh[i];
  dec(x);
  return taga(r);
}
B select_c2(B t, B w, B x) {
  if (isAtm(x)) thrM("⊏: 𝕩 cannot be an atom");
  ur xr = rnk(x);
  if (isAtm(w)) {
    if (xr==0) thrM("⊏: 𝕩 cannot be a unit");
    usz csz = arr_csz(x);
    usz cam = a(x)->sh[0];
    usz wi = WRAP(o2i64(w), cam, thrF("⊏: Indexing out-of-bounds (𝕨≡%R, %s≡≠𝕩)", w, cam));
    Arr* r = TI(x,slice)(inc(x), wi*csz, csz);
    usz* sh = arr_shAlloc(r, xr-1);
    if (sh) memcpy(sh, a(x)->sh+1, (xr-1)*sizeof(usz));
    dec(x);
    return taga(r);
  }
  B xf = getFillQ(x);
  SGet(x)
  usz wia = a(w)->ia;
  
  if (xr==1) {
    usz xia = a(x)->ia;
    #define CASE(T,E) if (TI(x,elType)==el_##T) { \
      E* rp; B r = m_##T##arrc(&rp, w);  \
      E* xp = T##any_ptr(x);             \
      for (usz i = 0; i < wia; i++) {    \
        rp[i] = xp[WRAP(wp[i], xia, thrF("⊏: Indexing out-of-bounds (%i∊𝕨, %s≡≠𝕩)", wp[i], xia))]; \
      }                                  \
      dec(w); dec(x); return r;          \
    }
    #define TYPE(W) { \
      W* wp = W##any_ptr(w); \
      CASE(i8,i8) CASE(i16,i16) CASE(i32,i32) \
      CASE(c8,u8) CASE(c16,u16) CASE(c32,u32) CASE(f64,f64) \
      usz i=0; HArr_p r = m_harrs(wia, &i); \
      if (v(x)->type==t_harr || v(x)->type==t_hslice) { \
        B* xp = hany_ptr(x); \
        for (; i < wia; i++) r.a[i] = inc(xp[WRAP(wp[i], xia, thrF("⊏: Indexing out-of-bounds (%i∊𝕨, %s≡≠𝕩)", wp[i], xia))]); \
        dec(x); return harr_fcd(r, w); \
      } \
      for (; i < wia; i++) r.a[i] = Get(x, WRAP(wp[i], xia, thrF("⊏: Indexing out-of-bounds (%i∊𝕨, %s≡≠𝕩)", wp[i], xia))); \
      dec(x); return withFill(harr_fcd(r,w),xf); \
    }
    if (TI(w,elType)==el_i8) TYPE(i8)
    else if (TI(w,elType)==el_i16) TYPE(i16)
    else if (TI(w,elType)==el_i32) TYPE(i32)
    else {
      usz i=0; HArr_p r = m_harrs(wia, &i);
      SGetU(w)
      for (; i < wia; i++) {
        B cw = GetU(w, i);
        if (!isNum(cw)) { harr_abandon(r); goto base; }
        usz c = WRAP(o2i64(cw), xia, thrF("⊏: Indexing out-of-bounds (%R∊𝕨, %s≡≠𝕩)", cw, xia));
        r.a[i] = Get(x, c);
      }
      dec(x);
      return withFill(harr_fcd(r,w),xf);
    }
    #undef CASE
  } else {
    SGetU(w)
    ur wr = rnk(w);
    i32 rr = wr+xr-1;
    if (xr==0) thrM("⊏: 𝕩 cannot be a unit");
    if (rr>UR_MAX) thrF("⊏: Result rank too large (%i≡=𝕨, %i≡=𝕩)", wr, xr);
    usz csz = arr_csz(x);
    usz cam = a(x)->sh[0];
    MAKE_MUT(r, wia*csz); mut_init(r, TI(x,elType));
    for (usz i = 0; i < wia; i++) {
      B cw = GetU(w, i);
      if (!isNum(cw)) { mut_pfree(r, i*csz); goto base; }
      f64 c = o2f(cw);
      if (c<0) c+= cam;
      if ((usz)c >= cam) { mut_pfree(r, i*csz); thrF("⊏: Indexing out-of-bounds (%R∊𝕨, %s≡≠𝕩)", cw, cam); }
      mut_copyG(r, i*csz, x, csz*(usz)c, csz);
    }
    Arr* ra = mut_fp(r);
    usz* rsh = arr_shAlloc(ra, rr);
    if (rsh) {
      memcpy(rsh   , a(w)->sh  ,  wr   *sizeof(usz));
      memcpy(rsh+wr, a(x)->sh+1, (xr-1)*sizeof(usz));
    }
    dec(w); dec(x);
    return withFill(taga(ra),xf);
  }
  base:
  dec(xf);
  return c2(rt_select, w, x);
}

static NOINLINE B slash_c1R(B x, u64 s) {
  usz xia = a(x)->ia;
  SGetU(x)
  f64* rp; B r = m_f64arrv(&rp, s); usz ri = 0;
  for (usz i = 0; i < xia; i++) {
    usz c = o2s(GetU(x, i));
    for (usz j = 0; j < c; j++) rp[ri++] = i;
  }
  dec(x);
  return r;
}
extern B rt_slash;
B slash_c1(B t, B x) {
  if (RARE(isAtm(x)) || RARE(rnk(x)!=1)) thrF("/: Argument must have rank 1 (%H ≡ ≢𝕩)", x);
  i64 s = isum(x);
  if(s<0) thrM("/: Argument must consist of natural numbers");
  usz xia = a(x)->ia;
  if (RARE(xia>=I32_MAX)) return slash_c1R(x, s);
  i32* rp; B r = m_i32arrv(&rp, s);
  u8 xe = TI(x,elType);
  if (xe==el_i8) {
    i8* xp = i8any_ptr(x);
    while (xia>0 && !xp[xia-1]) xia--;
    for (u64 i = 0; i < xia; i++) {
      i32 c = xp[i];
      if (LIKELY(c==0 || c==1)) {
        *rp = i;
        rp+= c;
      } else {
        if (RARE(c)<0) thrF("/: Argument must consist of natural numbers (contained %i)", c);
        for (i32 j = 0; j < c; j++) *rp++ = i;
      }
    }
  } else if (xe==el_i32) {
    i32* xp = i32any_ptr(x);
    while (xia>0 && !xp[xia-1]) xia--;
    for (u64 i = 0; i < xia; i++) {
      i32 c = xp[i];
      if (LIKELY(c==0 || c==1)) {
        *rp = i;
        rp+= c;
      } else {
        if (RARE(c)<0) thrF("/: Argument must consist of natural numbers (contained %i)", c);
        for (i32 j = 0; j < c; j++) *rp++ = i;
      }
    }
  } else {
    SGetU(x)
    for (u64 i = 0; i < xia; i++) {
      usz c = o2s(GetU(x, i));
      for (u64 j = 0; j < c; j++) *rp++ = i;
    }
  }
  dec(x);
  return r;
}
B slash_c2(B t, B w, B x) {
  if (isArr(x) && rnk(x)==1 && isArr(w) && rnk(w)==1 && depth(w)==1) {
    usz wia = a(w)->ia;
    usz xia = a(x)->ia;
    if (RARE(wia!=xia)) {
      if (wia==0) { dec(w); return x; }
      thrF("/: Lengths of components of 𝕨 must match 𝕩 (%s ≠ %s)", wia, xia);
    }
    B xf = getFillQ(x);
    
    usz ri = 0;
    #define CASE(WT,XT) if (TI(x,elType)==el_##XT) { \
      XT* xp = XT##any_ptr(x);                       \
      XT* rp; B r = m_##XT##arrv(&rp, wsum);         \
      if (or<2) for (usz i = 0; i < wia; i++) {      \
        *rp = xp[i];                                 \
        rp+= wp[i];                                  \
      } else for (usz i = 0; i < wia; i++) {         \
        WT cw = wp[i]; XT cx = xp[i];                \
        for (i64 j = 0; j < cw; j++) *rp++ = cx;     \
      }                                              \
      dec(w); dec(x); return r;                      \
    }
    #define TYPED(WT,SIGN) { \
      WT* wp = WT##any_ptr(w);           \
      while (wia>0 && !wp[wia-1]) wia--; \
      i64 wsum = 0;                      \
      u32 or = 0;                        \
      for (usz i = 0; i < wia; i++) {    \
        wsum+= wp[i];                    \
        or|= (u32)wp[i];                 \
      }                                  \
      if (or>>SIGN) thrM("/: 𝕨 must consist of natural numbers"); \
      CASE(WT,i8) CASE(WT,i16) CASE(WT,i32) CASE(WT,f64) \
      HArr_p r = m_harrs(wsum, &ri); SGetU(x)        \
      for (usz i = 0; i < wia; i++) {                \
        i32 cw = wp[i]; if (cw==0) continue;         \
        B cx = incBy(GetU(x, i), cw);                \
        for (i64 j = 0; j < cw; j++) r.a[ri++] = cx; \
      }                                              \
      dec(w); dec(x);                                \
      return withFill(harr_fv(r), xf);               \
    }
    if (TI(w,elType)==el_i8 ) TYPED(i8,7);
    if (TI(w,elType)==el_i32) TYPED(i32,31);
    #undef TYPED
    #undef CASE
    i64 ria = isum(w);
    if (ria>USZ_MAX) thrOOM();
    HArr_p r = m_harrs(ria, &ri);
    SGetU(w)
    SGetU(x)
    for (usz i = 0; i < wia; i++) {
      usz c = o2s(GetU(w, i));
      if (c) {
        B cx = incBy(GetU(x, i), c);
        for (usz j = 0; RARE(j < c); j++) r.a[ri++] = cx;
      }
    }
    dec(w); dec(x);
    return withFill(harr_fv(r), xf);
  }
  if (isArr(x) && rnk(x)==1 && q_i32(w)) {
    usz xia = a(x)->ia;
    i32 wv = o2i(w);
    if (wv<=0) {
      if (wv<0) thrM("/: 𝕨 cannot be negative");
      Arr* r = TI(x,slice)(x, 0, 0); arr_shVec(r);
      return taga(r);
    }
    if (TI(x,elType)==el_i32) {
      i32* xp = i32any_ptr(x);
      i32* rp; B r = m_i32arrv(&rp, xia*wv);
      for (usz i = 0; i < xia; i++) {
        for (i64 j = 0; j < wv; j++) *rp++ = xp[i];
      }
      dec(x);
      return r;
    } else {
      B xf = getFillQ(x);
      HArr_p r = m_harrUv(xia*wv);
      SGetU(x)
      for (usz i = 0; i < xia; i++) {
        B cx = incBy(GetU(x, i), wv);
        for (i64 j = 0; j < wv; j++) *r.a++ = cx;
      }
      dec(x);
      return withFill(r.b, xf);
    }
  }
  return c2(rt_slash, w, x);
}

static B slicev(B x, usz s, usz ia) {
  usz xia = a(x)->ia; assert(s+ia <= xia);
  Arr* r = TI(x,slice)(x, s, ia); arr_shVec(r);
  return taga(r);
}
extern B rt_take, rt_drop;
B take_c1(B t, B x) { return c1(rt_take, x); }
B drop_c1(B t, B x) { return c1(rt_drop, x); }
B take_c2(B t, B w, B x) {
  if (isNum(w)) {
    if (!isArr(x)) x = m_atomUnit(x);
    i64 wv = o2i64(w);
    ur xr = rnk(x);
    usz csz = 1;
    usz* xsh;
    if (xr>1) {
      csz = arr_csz(x);
      xsh = a(x)->sh;
      ptr_inc(shObjS(xsh)); // we'll look at it at the end and dec there
    }
    i64 t = wv*csz; // TODO error on overflow somehow
    Arr* a;
    if (t>=0) {
      a = take_impl(t, x);
    } else {
      t = -t;
      usz xia = a(x)->ia;
      if (t>xia) {
        B xf = getFillE(x);
        MAKE_MUT(r, t); mut_init(r, TI(x,elType));
        mut_fill(r, 0, xf, t-xia);
        mut_copyG(r, t-xia, x, 0, xia);
        dec(x); dec(xf);
        a = mut_fp(r);
      } else {
        a = TI(x,slice)(x,xia-t,t);
      }
    }
    if (xr<=1) {
      arr_shVec(a);
    } else {
      usz* rsh = arr_shAlloc(a, xr); // xr>1, don't have to worry about 0
      rsh[0] = wv<0?-wv:wv;
      for (i32 i = 1; i < xr; i++) rsh[i] = xsh[i];
      ptr_dec(shObjS(xsh));
    }
    return taga(a);
  }
  return c2(rt_take, w, x);
}
B drop_c2(B t, B w, B x) {
  if (isNum(w) && isArr(x) && rnk(x)==1) {
    i64 v = o2i64(w);
    usz ia = a(x)->ia;
    if (v<0) return -v>ia? slicev(x, 0, 0) : slicev(x, 0, v+ia);
    else     return  v>ia? slicev(x, 0, 0) : slicev(x, v, ia-v);
  }
  return c2(rt_drop, w, x);
}

extern B rt_join;
B join_c1(B t, B x) {
  if (isAtm(x)) thrM("∾: Argument must be an array");
  if (rnk(x)==1) {
    usz xia = a(x)->ia;
    if (xia==0) {
      B xf = getFillE(x);
      if (isAtm(xf)) {
        decA(xf);
        dec(x);
        if (!PROPER_FILLS) return emptyHVec();
        thrM("∾: Empty vector 𝕩 cannot have an atom fill element");
      }
      dec(x);
      ur ir = rnk(xf);
      if (ir==0) thrM("∾: Empty vector 𝕩 cannot have a unit fill element");
      B xff = getFillQ(xf);
      HArr_p r = m_harrUp(0);
      usz* sh = arr_shAlloc((Arr*)r.c, ir);
      if (sh) {
        sh[0] = 0;
        memcpy(sh+1, a(xf)->sh+1, sizeof(usz)*(ir-1));
      }
      dec(xf);
      return withFill(r.b, xff);
    }
    SGetU(x)
    
    B x0 = GetU(x,0);
    B rf; if(SFNS_FILLS) rf = getFillQ(x0);
    if (isAtm(x0)) thrM("∾: Rank of items must be equal or greater than rank of argument");
    usz ir = rnk(x0);
    usz* x0sh = a(x0)->sh;
    if (ir==0) thrM("∾: Rank of items must be equal or greater than rank of argument");
    
    usz csz = arr_csz(x0);
    usz cam = x0sh[0];
    for (usz i = 1; i < xia; i++) {
      B c = GetU(x, i);
      if (!isArr(c) || rnk(c)!=ir) thrF("∾: All items in argument should have same rank (contained items with ranks %i and %i)", ir, isArr(c)? rnk(c) : 0);
      usz* csh = a(c)->sh;
      if (ir>1) for (usz j = 1; j < ir; j++) if (csh[j]!=x0sh[j]) thrF("∾: Item trailing shapes must be equal (contained arrays with shapes %H and %H)", x0, c);
      cam+= a(c)->sh[0];
      if (SFNS_FILLS && !noFill(rf)) rf = fill_or(rf, getFillQ(c));
    }
    
    MAKE_MUT(r, cam*csz);
    usz ri = 0;
    for (usz i = 0; i < xia; i++) {
      B c = GetU(x, i);
      usz cia = a(c)->ia;
      mut_copy(r, ri, c, 0, cia);
      ri+= cia;
    }
    assert(ri==cam*csz);
    Arr* ra = mut_fp(r);
    usz* sh = arr_shAlloc(ra, ir);
    if (sh) {
      sh[0] = cam;
      memcpy(sh+1, x0sh+1, sizeof(usz)*(ir-1));
    }
    dec(x);
    return SFNS_FILLS? qWithFill(taga(ra), rf) : taga(ra);
  }
  return c1(rt_join, x);
}
B join_c2(B t, B w, B x) {
  if (isAtm(w)) w = m_atomUnit(w);
  ur wr = rnk(w);
  if (isAtm(x)) {
    if (wr==1 && inplace_add(w, x)) return w;
    x = m_atomUnit(x);
  }
  usz wia = a(w)->ia; usz* wsh = a(w)->sh;
  usz xia = a(x)->ia; usz* xsh = a(x)->sh; ur xr = rnk(x);
  B f = fill_both(w, x);
  ur c = wr>xr?wr:xr;
  if (c==0) {
    HArr_p r = m_harrUv(2);
    r.a[0] = IGet(w,0); dec(w);
    r.a[1] = IGet(x,0); dec(x);
    return qWithFill(r.b, f);
  }
  if (c-wr > 1 || c-xr > 1) thrF("∾: Argument ranks must differ by 1 or less (%i≡=𝕨, %i≡=𝕩)", wr, xr);
  if (c==1) {
    B r = vec_join(w, x);
    if (rnk(r)==0) srnk(r,1);
    return qWithFill(r, f);
  }
  MAKE_MUT(r, wia+xia); mut_init(r, el_or(TI(w,elType), TI(x,elType)));
  mut_copyG(r, 0,   w, 0, wia);
  mut_copyG(r, wia, x, 0, xia);
  Arr* ra = mut_fp(r);
  usz* sh = arr_shAlloc(ra, c);
  if (sh) {
    for (i32 i = 1; i < c; i++) {
      usz s = xsh[i+xr-c];
      if (wsh[i+wr-c] != s) { mut_pfree(r, wia+xia); thrF("∾: Lengths not matchable (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x); }
      sh[i] = s;
    }
    sh[0] = (wr==c? wsh[0] : 1) + (xr==c? xsh[0] : 1);
  }
  dec(w); dec(x);
  return qWithFill(taga(ra), f);
}


B couple_c1(B t, B x) {
  if (isAtm(x)) return unitV1(x);
  usz rr = rnk(x);
  usz ia = a(x)->ia;
  Arr* r = TI(x,slice)(inc(x),0, ia);
  usz* sh = arr_shAlloc(r, rr+1);
  if (sh) { sh[0] = 1; memcpy(sh+1, a(x)->sh, rr*sizeof(usz)); }
  dec(x);
  return taga(r);
}
B couple_c2(B t, B w, B x) {
  if (isAtm(w)&isAtm(x)) {
    if (LIKELY(isNum(w)&isNum(x))) {
      i32 wi=w.f; i32 xi=x.f;
      if (RARE(wi!=w.f | xi!=x.f))        { f64* rp; B r = m_f64arrv(&rp, 2); rp[0]=o2fu(w); rp[1]=o2fu(x); return r; }
      else if (wi==(i8 )wi & xi==(i8 )xi) { i8*  rp; B r = m_i8arrv (&rp, 2); rp[0]=o2iu(w); rp[1]=o2iu(x); return r; }
      else if (wi==(i16)wi & xi==(i16)xi) { i16* rp; B r = m_i16arrv(&rp, 2); rp[0]=o2iu(w); rp[1]=o2iu(x); return r; }
      else                                { i32* rp; B r = m_i32arrv(&rp, 2); rp[0]=o2iu(w); rp[1]=o2iu(x); return r; }
    }
    if (isC32(x)&isC32(w)) { u32* rp; B r = m_c32arrv(&rp, 2); rp[0]=o2cu(w); rp[1]=o2cu(x); return r; }
  }
  if (isAtm(w)) w = m_atomUnit(w);
  if (isAtm(x)) x = m_atomUnit(x);
  if (!eqShape(w, x)) thrF("≍: 𝕨 and 𝕩 must have equal shapes (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x);
  usz ia = a(w)->ia;
  ur wr = rnk(w);
  MAKE_MUT(r, ia*2); mut_init(r, el_or(TI(w,elType), TI(x,elType)));
  mut_copyG(r, 0,  w, 0, ia);
  mut_copyG(r, ia, x, 0, ia);
  Arr* ra = mut_fp(r);
  usz* sh = arr_shAlloc(ra, wr+1);
  if (sh) { sh[0]=2; memcpy(sh+1, a(w)->sh, wr*sizeof(usz)); }
  if (!SFNS_FILLS) { dec(w); dec(x); return taga(ra); }
  B rf = fill_both(w, x);
  dec(w); dec(x);
  return qWithFill(taga(ra), rf);
}


static inline void shift_check(B w, B x) {
  ur wr = rnk(w); usz* wsh = a(w)->sh;
  ur xr = rnk(x); usz* xsh = a(x)->sh;
  if (wr+1!=xr & wr!=xr) thrF("shift: =𝕨 must be =𝕩 or ¯1+=𝕩 (%i≡=𝕨, %i≡=𝕩)", wr, xr);
  for (i32 i = 1; i < xr; i++) if (wsh[i+wr-xr] != xsh[i]) thrF("shift: Lengths not matchable (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x);
}

B shiftb_c1(B t, B x) {
  if (isAtm(x) || rnk(x)==0) thrM("»: Argument cannot be a scalar");
  usz ia = a(x)->ia;
  if (ia==0) return x;
  B xf = getFillE(x);
  usz csz = arr_csz(x);
  
  MAKE_MUT(r, ia); mut_init(r, TI(x,elType));
  mut_copyG(r, csz, x, 0, ia-csz);
  mut_fill(r, 0, xf, csz);
  return qWithFill(mut_fcd(r, x), xf);
}
B shiftb_c2(B t, B w, B x) {
  if (isAtm(x) || rnk(x)==0) thrM("»: 𝕩 cannot be a scalar");
  if (isAtm(w)) w = m_atomUnit(w);
  shift_check(w, x);
  B f = fill_both(w, x);
  usz wia = a(w)->ia;
  usz xia = a(x)->ia;
  MAKE_MUT(r, xia); mut_init(r, el_or(TI(w,elType), TI(x,elType)));
  int mid = wia<xia? wia : xia;
  mut_copyG(r, 0  , w, 0, mid);
  mut_copyG(r, mid, x, 0, xia-mid);
  dec(w);
  return qWithFill(mut_fcd(r, x), f);
}

B shifta_c1(B t, B x) {
  if (isAtm(x) || rnk(x)==0) thrM("«: Argument cannot be a scalar");
  usz ia = a(x)->ia;
  if (ia==0) return x;
  B xf = getFillE(x);
  usz csz = arr_csz(x);
  MAKE_MUT(r, ia); mut_init(r, TI(x,elType));
  mut_copyG(r, 0, x, csz, ia-csz);
  mut_fill(r, ia-csz, xf, csz);
  return qWithFill(mut_fcd(r, x), xf);
}
B shifta_c2(B t, B w, B x) {
  if (isAtm(x) || rnk(x)==0) thrM("«: 𝕩 cannot be a scalar");
  if (isAtm(w)) w = m_atomUnit(w);
  shift_check(w, x);
  B f = fill_both(w, x);
  usz wia = a(w)->ia;
  usz xia = a(x)->ia;
  MAKE_MUT(r, xia); mut_init(r, el_or(TI(w,elType), TI(x,elType)));
  if (wia < xia) {
    usz m = xia-wia;
    mut_copyG(r, 0, x, wia, m);
    mut_copyG(r, m, w, 0, wia);
  } else {
    mut_copyG(r, 0, w, wia-xia, xia);
  }
  dec(w);
  return qWithFill(mut_fcd(r, x), f);
}

extern B rt_group;
B group_c1(B t, B x) {
  return c1(rt_group, x);
}
B group_c2(B t, B w, B x) {
  if (isArr(w)&isArr(x) && rnk(w)==1 && rnk(x)==1 && depth(w)==1) {
    usz wia = a(w)->ia;
    usz xia = a(x)->ia;
    if (wia-xia > 1) thrF("⊔: ≠𝕨 must be either ≠𝕩 or one bigger (%s≡≠𝕨, %s≡≠𝕩)", wia, xia);
    
    if (TI(w,elType)==el_i32) {
      i32* wp = i32any_ptr(w);
      i64 ria = wia==xia? 0 : wp[xia];
      if (ria<-1) thrM("⊔: 𝕨 can't contain elements less than ¯1");
      ria--;
      for (usz i = 0; i < xia; i++) if (wp[i]>ria) ria = wp[i];
      if (ria > (i64)(USZ_MAX-1)) thrOOM();
      ria++;
      TALLOC(i32, lenO, ria+1); i32* len = lenO+1;
      TALLOC(i32, pos, ria);
      for (usz i = 0; i < ria; i++) len[i] = pos[i] = 0;
      for (usz i = 0; i < xia; i++) {
        i32 n = wp[i];
        if (n<-1) thrM("⊔: 𝕨 can't contain elements less than ¯1");
        len[n]++; // overallocation makes this safe after n<-1 check
      }
      
      Arr* r = m_fillarrp(ria); fillarr_setFill(r, m_f64(0)); arr_shVec(r);
      B* rp = fillarr_ptr(r);
      for (usz i = 0; i < ria; i++) rp[i] = m_f64(0); // don't break if allocation errors
      B xf = getFillQ(x);
      
      Arr* rf = m_fillarrp(0); fillarr_setFill(rf, m_f64(0)); arr_shVec(rf);
      fillarr_setFill(r, taga(rf));
      if (TI(x,elType)==el_i8) {
        for (usz i = 0; i < ria; i++) { i8* t; rp[i] = m_i8arrv(&t, len[i]); }
        i8* xp = i8any_ptr(x);
        for (usz i = 0; i < xia; i++) {
          i32 n = wp[i];
          if (n>=0) i8arr_ptr(rp[n])[pos[n]++] = xp[i];
        }
      } else if (TI(x,elType)==el_i32) {
        for (usz i = 0; i < ria; i++) { i32* t; rp[i] = m_i32arrv(&t, len[i]); }
        i32* xp = i32any_ptr(x);
        for (usz i = 0; i < xia; i++) {
          i32 n = wp[i];
          if (n>=0) i32arr_ptr(rp[n])[pos[n]++] = xp[i];
        }
      } else if (TI(x,elType)==el_c32) {
        for (usz i = 0; i < ria; i++) { u32* t; rp[i] = m_c32arrv(&t, len[i]); }
        u32* xp = c32any_ptr(x);
        for (usz i = 0; i < xia; i++) {
          i32 n = wp[i];
          if (n>=0) c32arr_ptr(rp[n])[pos[n]++] = xp[i];
        }
      } else {
        for (usz i = 0; i < ria; i++) {
          Arr* c = m_fillarrp(len[i]);
          fillarr_setFill(c, inc(xf));
          c->ia = 0;
          rp[i] = taga(c);
        }
        SGet(x)
        for (usz i = 0; i < xia; i++) {
          i32 n = wp[i];
          if (n>=0) fillarr_ptr(a(rp[n]))[pos[n]++] = Get(x, i);
        }
        for (usz i = 0; i < ria; i++) { a(rp[i])->ia = len[i]; arr_shVec(a(rp[i])); }
      }
      fillarr_setFill(rf, xf);
      dec(w); dec(x); TFREE(lenO); TFREE(pos);
      return taga(r);
    } else {
      SGetU(w)
      i64 ria = wia==xia? 0 : o2i64(GetU(w, xia));
      if (ria<-1) thrM("⊔: 𝕨 can't contain elements less than ¯1");
      ria--;
      for (usz i = 0; i < xia; i++) {
        B cw = GetU(w, i);
        if (!q_i64(cw)) goto base;
        i64 c = o2i64u(cw);
        if (c>ria) ria = c;
        if (c<-1) thrM("⊔: 𝕨 can't contain elements less than ¯1");
      }
      if (ria > (i64)(USZ_MAX-1)) thrOOM();
      ria++;
      TALLOC(i32, lenO, ria+1); i32* len = lenO+1;
      TALLOC(i32, pos, ria);
      for (usz i = 0; i < ria; i++) len[i] = pos[i] = 0;
      for (usz i = 0; i < xia; i++) len[o2i64u(GetU(w, i))]++;
      
      Arr* r = m_fillarrp(ria); fillarr_setFill(r, m_f64(0)); arr_shVec(r);
      B* rp = fillarr_ptr(r);
      for (usz i = 0; i < ria; i++) rp[i] = m_f64(0); // don't break if allocation errors
      B xf = getFillQ(x);
      
      for (usz i = 0; i < ria; i++) {
        Arr* c = m_fillarrp(len[i]);
        fillarr_setFill(c, inc(xf));
        c->ia = 0;
        rp[i] = taga(c);
      }
      Arr* rf = m_fillarrp(0); arr_shVec(rf);
      fillarr_setFill(rf, xf);
      fillarr_setFill(r, taga(rf));
      SGet(x)
      for (usz i = 0; i < xia; i++) {
        i64 n = o2i64u(GetU(w, i));
        if (n>=0) fillarr_ptr(a(rp[n]))[pos[n]++] = Get(x, i);
      }
      for (usz i = 0; i < ria; i++) { a(rp[i])->ia = len[i]; arr_shVec(a(rp[i])); }
      dec(w); dec(x); TFREE(lenO); TFREE(pos);
      return taga(r);
    }
  }
  base:
  return c2(rt_group, w, x);
}

extern B rt_reverse;
B reverse_c1(B t, B x) {
  if (isAtm(x) || rnk(x)==0) thrM("⌽: Argument cannot be a unit");
  B xf = getFillQ(x);
  u8 xe = TI(x,elType);
  usz xia = a(x)->ia;
  if (rnk(x)==1 && xe<el_B) {
    B r; if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); i8 * rp; r = m_i8arrv (&rp, xia); for (usz i = 0; i < xia; i++) rp[i] = xp[xia-i-1]; }
    else if (xe==el_c8 ) { u8*  xp = c8any_ptr (x); u8 * rp; r = m_c8arrv (&rp, xia); for (usz i = 0; i < xia; i++) rp[i] = xp[xia-i-1]; }
    else if (xe==el_i16) { i16* xp = i16any_ptr(x); i16* rp; r = m_i16arrv(&rp, xia); for (usz i = 0; i < xia; i++) rp[i] = xp[xia-i-1]; }
    else if (xe==el_c16) { u16* xp = c16any_ptr(x); u16* rp; r = m_c16arrv(&rp, xia); for (usz i = 0; i < xia; i++) rp[i] = xp[xia-i-1]; }
    else if (xe==el_i32) { i32* xp = i32any_ptr(x); i32* rp; r = m_i32arrv(&rp, xia); for (usz i = 0; i < xia; i++) rp[i] = xp[xia-i-1]; }
    else if (xe==el_c32) { u32* xp = c32any_ptr(x); u32* rp; r = m_c32arrv(&rp, xia); for (usz i = 0; i < xia; i++) rp[i] = xp[xia-i-1]; }
    else if (xe==el_f64) { f64* xp = f64any_ptr(x); f64* rp; r = m_f64arrv(&rp, xia); for (usz i = 0; i < xia; i++) rp[i] = xp[xia-i-1]; }
    else UD;
    dec(x);
    return r;
  }
  usz csz = arr_csz(x);
  usz cam = a(x)->sh[0];
  usz rp = 0;
  usz ip = xia;
  MAKE_MUT(r, xia); mut_init(r, xe);
  for (usz i = 0; i < cam; i++) {
    ip-= csz;
    mut_copyG(r, rp, x, ip, csz);
    rp+= csz;
  }
  return withFill(mut_fcd(r, x), xf);
}
B reverse_c2(B t, B w, B x) {
  if (isArr(w)) return c2(rt_reverse, w, x);
  if (isAtm(x) || rnk(x)==0) thrM("⌽: 𝕩 must have rank at least 1 for atom 𝕨");
  usz xia = a(x)->ia;
  if (xia==0) return x;
  B xf = getFillQ(x);
  usz cam = a(x)->sh[0];
  usz csz = arr_csz(x);
  i64 am = o2i64(w);
  if ((u64)am >= (u64)cam) { am%= (i64)cam; if(am<0) am+= cam; }
  am*= csz;
  MAKE_MUT(r, xia); mut_init(r, TI(x,elType));
  mut_copyG(r, 0, x, am, xia-am);
  mut_copyG(r, xia-am, x, 0, am);
  return withFill(mut_fcd(r, x), xf);
}

B reverse_uc1(B t, B o, B x) {
  return reverse_c1(t, c1(o, reverse_c1(t, x)));
}

B pick_uc1(B t, B o, B x) {
  if (isAtm(x) || a(x)->ia==0) return def_fn_uc1(t, o, x);
  B xf = getFillQ(x);
  usz ia = a(x)->ia;
  B arg = IGet(x, 0);
  B rep = c1(o, arg);
  MAKE_MUT(r, ia); mut_init(r, el_or(TI(x,elType), selfElType(rep)));
  mut_setG(r, 0, rep);
  mut_copyG(r, 1, x, 1, ia-1);
  return qWithFill(mut_fcd(r, x), xf);
}

B pick_ucw(B t, B o, B w, B x) {
  if (isArr(w) || isAtm(x) || rnk(x)!=1) return def_fn_ucw(t, o, w, x);
  usz xia = a(x)->ia;
  usz wi = WRAP(o2i64(w), xia, thrF("𝔽⌾(n⊸⊑)𝕩: reading out-of-bounds (n≡%R, %s≡≠𝕩)", w, xia));
  B xf = getFillQ(x);
  B arg = IGet(x, wi);
  B rep = c1(o, arg);
  if (reusable(x) && TI(x,canStore)(rep)) {
    if      (TI(x,elType)==el_i8 ) { i8*  xp = i8any_ptr (x); xp[wi] = o2iu(rep); return x; }
    else if (TI(x,elType)==el_i16) { i16* xp = i16any_ptr(x); xp[wi] = o2iu(rep); return x; }
    else if (TI(x,elType)==el_i32) { i32* xp = i32any_ptr(x); xp[wi] = o2iu(rep); return x; }
    else if (TI(x,elType)==el_f64) { f64* xp = f64any_ptr(x); xp[wi] = o2fu(rep); return x; } 
    else if (v(x)->type==t_harr) {
      B* xp = harr_ptr(x);
      dec(xp[wi]);
      xp[wi] = rep;
      return x;
    } else if (v(x)->type==t_fillarr) {
      B* xp = fillarr_ptr(a(x));
      dec(xp[wi]);
      xp[wi] = rep;
      return x;
    }
  }
  MAKE_MUT(r, xia); mut_init(r, el_or(TI(x,elType), selfElType(rep)));
  mut_setG(r, wi, rep);
  mut_copyG(r, 0, x, 0, wi);
  mut_copyG(r, wi+1, x, wi+1, xia-wi-1);
  return qWithFill(mut_fcd(r, x), xf);
}

B slash_ucw(B t, B o, B w, B x) {
  if (isAtm(w) || isAtm(x) || rnk(w)!=1 || rnk(x)!=1 || a(w)->ia!=a(x)->ia) return def_fn_ucw(t, o, w, x);
  usz ia = a(x)->ia;
  SGetU(w)
  if (TI(w,elType)!=el_i32) for (usz i = 0; i < ia; i++) if (!q_i32(GetU(w,i))) return def_fn_ucw(t, o, w, x);
  B arg = slash_c2(t, inc(w), inc(x));
  usz argIA = a(arg)->ia;
  B rep = c1(o, arg);
  if (isAtm(rep) || rnk(rep)!=1 || a(rep)->ia != argIA) thrF("𝔽⌾(a⊸/)𝕩: Result of 𝔽 must have the same shape as a/𝕩 (expected ⟨%s⟩, got %H)", argIA, rep);
  MAKE_MUT(r, ia); mut_init(r, el_or(TI(x,elType), TI(rep,elType)));
  SGet(x)
  SGetU(rep)
  SGet(rep)
  usz repI = 0;
  for (usz i = 0; i < ia; i++) {
    i32 cw = o2iu(GetU(w, i));
    if (cw) {
      B cr = Get(rep,repI);
      if (CHECK_VALID) for (i32 j = 1; j < cw; j++) if (!equal(GetU(rep,repI+j), cr)) { mut_pfree(r,i); thrM("𝔽⌾(a⊸/): Incompatible result elements"); }
      mut_setG(r, i, cr);
      repI+= cw;
    } else mut_setG(r, i, Get(x,i));
  }
  dec(w); dec(rep);
  return mut_fcd(r, x);
}

B select_ucw(B t, B o, B w, B x) {
  if (isAtm(x) || rnk(x)!=1 || isAtm(w) || rnk(w)!=1) return def_fn_ucw(t, o, w, x);
  usz xia = a(x)->ia;
  usz wia = a(w)->ia;
  SGetU(w)
  if (TI(w,elType)!=el_i32) for (usz i = 0; i < wia; i++) if (!q_i64(GetU(w,i))) return def_fn_ucw(t, o, w, x);
  B arg = select_c2(t, inc(w), inc(x));
  B rep = c1(o, arg);
  if (isAtm(rep) || rnk(rep)!=1 || a(rep)->ia != wia) thrF("𝔽⌾(a⊸⊏)𝕩: Result of 𝔽 must have the same shape as a⊏𝕩 (expected ⟨%s⟩, got %H)", wia, rep);
  #if CHECK_VALID
    TALLOC(bool, set, xia);
    for (i64 i = 0; i < xia; i++) set[i] = false;
    #define EQ(F) if (set[cw] && (F)) thrM("𝔽⌾(a⊸⊏): Incompatible result elements"); set[cw] = true;
    #define FREE_CHECK TFREE(set)
  #else
    #define EQ(F)
    #define FREE_CHECK
  #endif
  
  u8 we = TI(w,elType);
  u8 xe = TI(x,elType);
  u8 re = TI(rep,elType);
  
  if (we==el_i32) {
    i32* wp = i32any_ptr(w);
    if (re<el_f64 && xe<el_f64) {
      u8 me = xe>re?xe:re;
      bool re = reusable(x);
      if (me==el_i32) {
        I32Arr* xn = re? toI32Arr(x) : cpyI32Arr(x);
        rep = toI32Any(rep); i32* rp = i32any_ptr(rep);
        for (usz i = 0; i < wia; i++) {
          i64 cw = wp[i]; if (RARE(cw<0)) cw+= (i64)xia; // we're free to assume w is valid
          i32 cr = rp[i];
          EQ(cr != xn->a[cw]);
          xn->a[cw] = cr;
        }
        dec(w); dec(rep); FREE_CHECK; return taga(xn);
      } else if (me==el_i16) {
        I16Arr* xn = re? toI16Arr(x) : cpyI16Arr(x);
        rep = toI16Any(rep); i16* rp = i16any_ptr(rep);
        for (usz i = 0; i < wia; i++) {
          i64 cw = wp[i]; if (RARE(cw<0)) cw+= (i64)xia;
          i16 cr = rp[i];
          EQ(cr != xn->a[cw]);
          xn->a[cw] = cr;
        }
        dec(w); dec(rep); FREE_CHECK; return taga(xn);
      } else if (me==el_i8) {
        I8Arr* xn = re? toI8Arr(x) : cpyI8Arr(x);
        rep = toI8Any(rep); i8* rp = i8any_ptr(rep);
        for (usz i = 0; i < wia; i++) {
          i64 cw = wp[i]; if (RARE(cw<0)) cw+= (i64)xia;
          i8 cr = rp[i];
          EQ(cr != xn->a[cw]);
          xn->a[cw] = cr;
        }
        dec(w); dec(rep); FREE_CHECK; return taga(xn);
      } else UD;
    }
    if (reusable(x) && xe==re) {
      if (v(x)->type==t_harr) {
        B* xp = harr_ptr(x);
        SGet(rep)
        for (usz i = 0; i < wia; i++) {
          i64 cw = wp[i]; if (RARE(cw<0)) cw+= (i64)xia;
          B cr = Get(rep, i);
          EQ(!equal(cr,xp[cw]));
          dec(xp[cw]);
          xp[cw] = cr;
        }
        dec(w); dec(rep); FREE_CHECK;
        return x;
      }
    }
    MAKE_MUT(r, xia); mut_init(r, el_or(xe, re));
    mut_copyG(r, 0, x, 0, xia);
    SGet(rep)
    for (usz i = 0; i < wia; i++) {
      i64 cw = wp[i]; if (RARE(cw<0)) cw+= (i64)xia;
      B cr = Get(rep, i);
      EQ(!equal(mut_getU(r, cw), cr));
      mut_rm(r, cw);
      mut_setG(r, cw, cr);
    }
    dec(w); dec(rep); FREE_CHECK;
    return mut_fcd(r, x);
  }
  MAKE_MUT(r, xia); mut_init(r, el_or(xe, re));
  mut_copyG(r, 0, x, 0, xia);
  SGet(rep)
  for (usz i = 0; i < wia; i++) {
    i64 cw = o2i64u(GetU(w, i)); if (RARE(cw<0)) cw+= (i64)xia;
    B cr = Get(rep, i);
    EQ(!equal(mut_getU(r, cw), cr));
    mut_rm(r, cw);
    mut_setG(r, cw, cr);
  }
  dec(w); dec(rep); FREE_CHECK;
  return mut_fcd(r, x);
  #undef EQ
  #undef FREE_CHECK
}


void sfns_init() {
  c(BFn,bi_pick)->uc1 = pick_uc1;
  c(BFn,bi_reverse)->uc1 = reverse_uc1;
  c(BFn,bi_pick)->ucw = pick_ucw;
  c(BFn,bi_slash)->ucw = slash_ucw;
  c(BFn,bi_select)->ucw = select_ucw;
}
