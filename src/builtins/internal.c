#include "../core.h"
#include "../utils/mut.h"

B itype_c1(B t, B x) {
  B r;
  if(isVal(x)) {
    r = m_str8l(format_type(v(x)->type));
  } else {
    if      (isF64(x)) r = m_str32(U"tagged f64");
    else if (isI32(x)) r = m_str32(U"tagged i32");
    else if (isC32(x)) r = m_str32(U"tagged c32");
    else if (isTag(x)) r = m_str32(U"tagged tag");
    else if (isVar(x)) r = m_str32(U"tagged var");
    else if (isExt(x)) r = m_str32(U"tagged extvar");
    else               r = m_str32(U"tagged unknown");
  }
  dec(x);
  return r;
}
B refc_c1(B t, B x) {
  B r = isVal(x)? m_i32(v(x)->refc) : m_str32(U"(not heap-allocated)");
  dec(x);
  return r;
}
B squeeze_c1(B t, B x) {
  return bqn_squeeze(x);
}
B isPure_c1(B t, B x) {
  B r = m_f64(isPureFn(x));
  dec(x);
  return r;
}

B info_c1(B t, B x) {
  B s = inc(bi_emptyCVec);
  AFMT("%xl: ", x.u);
  if (isVal(x)) {
    Value* xv = v(x);
    AFMT("refc:%i ", xv->refc);
    AFMT("mmInfo:%i ", xv->mmInfo);
    AFMT("flags:%i ", xv->flags);
    AFMT("extra:%i ", xv->extra);
    AFMT("type:%i=%S ", xv->type, format_type(xv->type));
    AFMT("alloc:%l", mm_size(xv));
    dec(x);
  } else {
    A8("not heap-allocated");
  }
  return s;
}

static B internalNS;
B getInternalNS() {
  if (internalNS.u == 0) {
    #define F(X) inc(bi_##X),
    B fn = bqn_exec(m_str32(U"{  Type,  Refc,  Squeeze,  IsPure,  Info⟩⇐𝕩}"), inc(bi_emptyHVec), inc(bi_emptyHVec));
    B arg =    m_caB(7, (B[]){F(itype)F(refc)F(squeeze)F(isPure)F(info)});
    #undef F
    internalNS = c1(fn,arg);
    gc_add(internalNS);
    dec(fn);
  }
  return inc(internalNS);
}
