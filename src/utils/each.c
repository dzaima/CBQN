#include "../core.h"
#include "each.h"

static inline B  mv(B*     p, usz n) { B r = p  [n]; p  [n] = m_f64(0); return r; }
static inline B hmv(HArr_p p, usz n) { B r = p.a[n]; p.a[n] = m_f64(0); return r; }

B eachd_fn(B fo, B w, B x, BBB2B f) {
  ur wr, xr; // if rank is 0, respective w/x will be disclosed
  if (isArr(w)) { wr=RNK(w); if (wr==0) { B c=IGet(w, 0); decG(w); w=c; } } else wr=0;
  if (isArr(x)) { xr=RNK(x); if (xr==0) { B c=IGet(x, 0); decG(x); x=c; } } else xr=0;
  bool wg = wr>xr;
  ur rM = wg? wr : xr;
  ur rm = wg? xr : wr;
  if (rM==0) return m_unit(f(fo, w, x));
  if (rm && !eqShPart(SH(w), SH(x), rm)) thrF("Mapping: Expected equal shape prefix (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x);
  bool rw = rM==wr && reusable(w) && TY(w)==t_harr; // dereferencing is safe as rank>0 from rM==
  bool rx = rM==xr && reusable(x) && TY(x)==t_harr;
  if (rw|rx && (wr==xr | rm==0)) {
    HArr_p r = harr_parts(REUSE(rw? w : x));
    usz ria = r.c->ia;
    if (ria>0) {
      if      (wr==0) { incBy(w, ria-1); for(usz i=0; i<ria; i++) r.a[i] = f(fo, w,   hmv(r,i)); }
      else if (xr==0) { incBy(x, ria-1); for(usz i=0; i<ria; i++) r.a[i] = f(fo, hmv(r,i), x  ); }
      else {
        assert(wr==xr);
        if (rw) { SGet(x) for (usz i = 0; i < ria; i++) r.a[i] = f(fo, hmv(r,i), Get(x,i)); }
        else    { SGet(w) for (usz i = 0; i < ria; i++) r.a[i] = f(fo, Get(w,i), hmv(r,i)); }
        decG(rw? x : w);
      }
    } else {
      dec(rw? x : w);
    }
    return any_squeeze(r.b);
  }
  
  B bo = wg? w : x;
  usz ria = IA(bo);
  B rb;
  if (ria==0) {
    rb = rM==1? emptyHVec() : m_harrUc(bo).b;
  } else {
    M_HARR(r, ria)
    if (wr==xr) {            SGet(x) SGet(w) for(usz ri=0; ri<ria; ri++) HARR_ADD(r, ri, f(fo, Get(w,ri), Get(x,ri))); }
    else if (wr==0) { incBy(w, ria); SGet(x) for(usz ri=0; ri<ria; ri++) HARR_ADD(r, ri, f(fo, w        , Get(x,ri))); }
    else if (xr==0) { incBy(x, ria); SGet(w) for(usz ri=0; ri<ria; ri++) HARR_ADD(r, ri, f(fo, Get(w,ri), x        )); }
    else {
      usz min = wg? IA(x) : IA(w);
      usz ext = ria / min;
      SGet(w) SGet(x)
      if (wg) for (usz i = 0; i < min; i++) { B c=incBy(Get(x,i), ext-1); for (usz j=0; j<ext; j++) HARR_ADDA(r, f(fo, Get(w,HARR_I(r)), c)); }
      else    for (usz i = 0; i < min; i++) { B c=incBy(Get(w,i), ext-1); for (usz j=0; j<ext; j++) HARR_ADDA(r, f(fo, c, Get(x,HARR_I(r)))); }
    }
    rb = any_squeeze(HARR_FC(r, bo));
  }
  dec(w); dec(x);
  return rb;
}

B eachm_fn(B fo, B x, BB2B f) {
  usz ia = IA(x);
  usz i = 0;
  if (ia==0) return x;
  if (reusable(x)) {
    B* xp;
    re_reuse:
    switch (v(x)->type) {
      case t_fillarr: {
        dec(c(FillArr,x)->fill);
        c(FillArr,x)->fill = bi_noFill;
        xp = fillarr_ptr(a(x));
        break;
      }
      case t_harr: {
        xp = harr_ptr(x);
        break;
      }
      case t_fillslice: {
        FillSlice* s = c(FillSlice,x);
        Arr* p = s->p;
        if (p->refc==1 && (p->type==t_fillarr || p->type==t_harr) && ((FillArr*)p)->a == s->a && ptr_eqShape(p, a(x))) {
          x = taga(ptr_inc(p));
          value_free((Value*)s);
          goto re_reuse;
        } else goto base;
      }
      default: goto base;
    }
    REUSE(x);
    for (; i < ia; i++) xp[i] = f(fo, mv(xp, i));
    return any_squeeze(x);
  }
  
  base:;
  M_HARR(r, ia)
  void* xp = tyany_ptr(x);
  switch(TI(x,elType)) { default: UD;
    case el_B: { SGet(x);
               { for (; i<ia; i++) HARR_ADD(r, i, f(fo, Get(x,i))); } break; }
    case el_bit: for (; i<ia; i++) HARR_ADD(r, i, f(fo, m_i32(bitp_get(xp,i)))); break;
    case el_i8:  for (; i<ia; i++) HARR_ADD(r, i, f(fo, m_i32(((i8* )xp)[i]))); break;
    case el_i16: for (; i<ia; i++) HARR_ADD(r, i, f(fo, m_i32(((i16*)xp)[i]))); break;
    case el_i32: for (; i<ia; i++) HARR_ADD(r, i, f(fo, m_i32(((i32*)xp)[i]))); break;
    case el_f64: for (; i<ia; i++) HARR_ADD(r, i, f(fo, m_f64(((f64*)xp)[i]))); break;
    case el_c8:  for (; i<ia; i++) HARR_ADD(r, i, f(fo, m_c32(((u8* )xp)[i]))); break;
    case el_c16: for (; i<ia; i++) HARR_ADD(r, i, f(fo, m_c32(((u16*)xp)[i]))); break;
    case el_c32: for (; i<ia; i++) HARR_ADD(r, i, f(fo, m_c32(((u32*)xp)[i]))); break;
  }
  return any_squeeze(HARR_FCD(r, x));
}

#if CATCH_ERRORS
B arith_recd(BBB2B f, B w, B x) {
  B fx = getFillQ(x);
  if (noFill(fx)) return eachd_fn(bi_N, w, x, f);
  B fw = getFillQ(w);
  B r = eachd_fn(bi_N, w, x, f);
  if (noFill(fw)) { dec(fx); return r; }
  if (CATCH) { freeThrown(); return r; }
  B fr = f(bi_N, fw, fx);
  popCatch();
  return withFill(r, asFill(fr));
}
#endif