#include "../core.h"
#include "../builtins.h"
#include "../nfns.h"


static NFnDesc* fn_invRegDesc;
static NFnDesc* fn_invSwapDesc;
B fn_invReg_c1 (B t,      B x) { B f = nfn_objU(t); return TI(f, fn_im)(f,    x); }
B fn_invReg_c2 (B t, B w, B x) { B f = nfn_objU(t); return TI(f, fn_ix)(f, w, x); }
B fn_invSwap_c1(B t,      B x) { B f = nfn_objU(t); return TI(f, fn_is)(f,    x); }
B fn_invSwap_c2(B t, B w, B x) { B f = nfn_objU(t); return TI(f, fn_iw)(f, w, x); }

extern B rt_undo;
B undo_c1(Md1D* d, B x) { B f = d->f;
  if (isFun(f)) return TI(f, fn_im)(f, x);
  B fi = m1_d(incG(rt_undo), inc(f));
  B r = c1(fi, x);
  decG(fi);
  return r;
}
B undo_c2(Md1D* d, B w, B x) { B f = d->f;
  if (isFun(f)) return TI(f, fn_ix)(f, w, x);
  B fi = m1_d(incG(rt_undo), inc(f));
  B r = c2(fi, w, x);
  decG(fi);
  return r;
}

B setInvReg_c1 (B t, B x) { rt_invFnReg  = x; rt_invFnRegFn  = c(Fun,x)->c1; return inc(bi_nativeInvReg); }
B setInvSwap_c1(B t, B x) { rt_invFnSwap = x; rt_invFnSwapFn = c(Fun,x)->c1; return inc(bi_nativeInvSwap); }
B nativeInvReg_c1(B t, B x) {
  if (isFun(x)) return m_nfn(fn_invRegDesc, x);
  return c1(rt_invFnReg, x);
}
B nativeInvSwap_c1(B t, B x) {
  if (isFun(x)) return m_nfn(fn_invSwapDesc, x);
  return c1(rt_invFnSwap, x);
}

void inverse_init() {
  fn_invRegDesc = registerNFn(m_ascii0("(fn_invReg)"), fn_invReg_c1, fn_invReg_c2);
  fn_invSwapDesc = registerNFn(m_ascii0("(fn_invSwap)"), fn_invSwap_c1, fn_invSwap_c2);
}