#include "../core.h"
#include "each.h"

static inline B  mv(B*     p, usz n) { B r = p  [n]; p  [n] = m_f64(0); return r; }
static inline B hmv(HArr_p p, usz n) { B r = p.a[n]; p.a[n] = m_f64(0); return r; }

B eachd_fn(B fo, B w, B x, BBB2B f) {
  if (isAtm(w)) w = m_atomUnit(w);
  if (isAtm(x)) x = m_atomUnit(x);
  ur wr = rnk(w); SGet(w);
  ur xr = rnk(x); SGet(x);
  bool wg = wr>xr;
  ur rM = wg? wr : xr;
  ur rm = wg? xr : wr;
  if (rM==0) {
    B r = f(fo, Get(w,0), Get(x,0));
    decG(w); decG(x);
    return m_unit(r);
  }
  if (rm && !eqShPart(a(w)->sh, a(x)->sh, rm)) thrF("Mapping: Expected equal shape prefix (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x);
  bool rw = rM==wr && ((v(w)->type==t_harr) & reusable(w)); // v(…) is safe as rank>0
  bool rx = rM==xr && ((v(x)->type==t_harr) & reusable(x));
  if (rw|rx && (wr==xr | rm==0)) {
    HArr_p r = harr_parts(REUSE(rw? w : x));
    usz ria = r.c->ia;
    if      (wr==0) { B c=Get(w, 0); for(usz i = 0; i < ria; i++) r.a[i] = f(fo, inc(c),   hmv(r,i)); dec(c); }
    else if (xr==0) { B c=Get(x, 0); for(usz i = 0; i < ria; i++) r.a[i] = f(fo, hmv(r,i), inc(c)  ); dec(c); }
    else {
      assert(wr==xr);
      if (rw) for (usz i = 0; i < ria; i++) r.a[i] = f(fo, hmv(r,i), Get(x,i));
      else    for (usz i = 0; i < ria; i++) r.a[i] = f(fo, Get(w,i), hmv(r,i));
    }
    decG(rw? x : w);
    return any_squeeze(r.b);
  }
  
  B bo = wg? w : x;
  usz ria = a(bo)->ia;
  M_HARR(r, ria)
  if (wr==xr)                      for(usz ri=0; ri<ria; ri++) HARR_ADD(r, ri, f(fo, Get(w,ri), Get(x,ri)));
  else if (wr==0) { B c=Get(w, 0); for(usz ri=0; ri<ria; ri++) HARR_ADD(r, ri, f(fo, inc(c)   , Get(x,ri))); dec(c); }
  else if (xr==0) { B c=Get(x, 0); for(usz ri=0; ri<ria; ri++) HARR_ADD(r, ri, f(fo, Get(w,ri), inc(c)   )); dec(c); }
  else if (ria>0) {
    usz min = wg? a(x)->ia : a(w)->ia;
    usz ext = ria / min;
    if (wg) for (usz i = 0; i < min; i++) { B c=Get(x,i); for (usz j = 0; j < ext; j++) HARR_ADDA(r, f(fo, Get(w,HARR_I(r)), inc(c))); }
    else    for (usz i = 0; i < min; i++) { B c=Get(w,i); for (usz j = 0; j < ext; j++) HARR_ADDA(r, f(fo, inc(c), Get(x,HARR_I(r)))); }
  }
  B rb = HARR_FC(r, bo);
  decG(w); decG(x);
  return any_squeeze(rb);
}

B eachm_fn(B fo, B x, BB2B f) { // TODO definitely rewrite this. Probably still has refcounting errors
  usz ia = a(x)->ia;
  if (ia==0) return x;
  SGet(x);
  usz i = 0;
  B cr = f(fo, Get(x,0));
  HArr_p rH;
  if (TI(x,canStore)(cr)) {
    bool reuse = reusable(x);
    if (v(x)->type==t_harr) {
      B* xp = harr_ptr(x);
      if (reuse) {
        dec(xp[i]); xp[i++] = cr;
        for (; i < ia; i++) xp[i] = f(fo, mv(xp,i));
        return any_squeeze(REUSE(x));
      } else {
        M_HARR(rHc, ia)
        HARR_ADD(rHc, i, cr);
        for (usz i = 1; i < ia; i++) HARR_ADD(rHc, i, f(fo, inc(xp[i])));
        return any_squeeze(HARR_FCD(rHc, x));
      }
    } else if (TI(x,elType)==el_i32) {
      i32* xp = i32any_ptr(x);
      B r; i32* rp;
      if (reuse && v(x)->type==t_i32arr) { r=incG(REUSE(x)); rp = xp; }
      else r = m_i32arrc(&rp, x);
      rp[i++] = o2iu(cr);
      for (; i < ia; i++) {
        cr = f(fo, m_i32(xp[i]));
        if (!q_i32(cr)) {
          rH = m_harr0c(x);
          for (usz j = 0; j < i; j++) rH.a[j] = m_i32(rp[j]);
          decG(r);
          goto fallback;
        }
        rp[i] = o2iu(cr);
      }
      decG(x);
      return num_squeeze(r);
    } else if (TI(x,elType)==el_f64) {
      f64* xp = f64any_ptr(x);
      B r; f64* rp;
      if (reuse && v(x)->type==t_f64arr) { r=incG(REUSE(x)); rp = xp; }
      else       r = m_f64arrc(&rp, x);
      rp[i++] = o2fu(cr);
      for (; i < ia; i++) {
        cr = f(fo, m_f64(xp[i]));
        if (!q_f64(cr)) {
          rH = m_harr0c(x);
          for (usz j = 0; j < i; j++) rH.a[j] = m_f64(rp[j]);
          decG(r);
          goto fallback;
        }
        rp[i] = o2fu(cr);
      }
      decG(x);
      return num_squeeze(r);
    } else if (v(x)->type==t_fillarr) {
      B* xp = fillarr_ptr(a(x));
      if (reuse) {
        dec(c(FillArr,x)->fill);
        c(FillArr,x)->fill = bi_noFill;
        dec(xp[i]); xp[i++] = cr;
        for (; i < ia; i++) xp[i] = f(fo, mv(xp,i));
        return any_squeeze(REUSE(x));
      } else {
        M_HARR(rHc, ia)
        HARR_ADD(rHc, i, cr);
        for (usz i = 1; i < ia; i++) HARR_ADD(rHc, i, f(fo, inc(xp[i])));
        return any_squeeze(HARR_FCD(rHc, x));
      }
    } else
    rH = m_harr0c(x);
  } else
  rH = m_harr0c(x);
  fallback:
  rH.a[i++] = cr;
  for (; i < ia; i++) rH.a[i] = f(fo, Get(x,i));
  decG(x);
  return any_squeeze(rH.b);
}