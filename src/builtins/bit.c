#include "../core.h"
#include "../builtins.h"
#include "../ns.h"

typedef void (*AndBytesFn)(u8*, u8*, u64, u64);
#if SINGELI
  extern uint64_t* const si_spaced_masks;
  #define SINGELI_FILE bit_arith
  #include "../utils/includeSingeli.h"
  const AndBytesFn andBytes_fn = si_andBytes;
  #define BITARITH_IDX(OP) (OP-op_add)*4 + owl-3
#else
  static void base_andBytes(u8* r, u8* x, u64 repeatedMask, u64 numBytes) {
    u64* x64 = (u64*)x; usz i;
    vfor (i = 0; i < numBytes/8; i++) ((u64*)r)[i] = x64[i] & repeatedMask;
    if (i*8 != numBytes) {
      u64 v = x64[i]&repeatedMask;
      for (usz j = 0; j < (numBytes&7); j++) r[i*8 + j] = v>>(j*8);
    }
  }
  const AndBytesFn andBytes_fn = base_andBytes;
#endif



typedef struct CastType { usz s; bool c; } CastType;
static bool isCharArr(B x) {
  return elChr(TI(x,elType));
}
static CastType getCastType(B e, bool hasVal, B val) { // returns a valid type (doesn't check if it can apply to val)
  usz s; bool c;
  if (isNum(e)) {
    s = o2s(e);
    if (s!=1 && s!=8 && s!=16 && s!=32 && s!=64) thrF("•bit._cast: Unsupported width %s", s);
    c = hasVal? isCharArr(val) : 0;
  } else {
    if (!isArr(e) || RNK(e)!=1 || IA(e)!=2) thrM("•bit._cast: 𝕗 elements must be numbers or two-element lists");
    SGetU(e)
    s = o2s(GetU(e,0));
    u32 t = o2c(GetU(e,1));
    c = t=='c';
    if      (c     ) { if (s!=8 && s!=16 && s!=32) { badWidth: thrF("•bit._cast: Unsupported width %s for type '%c'", s, (char)t); } }
    else if (t=='i') { if (s!=8 && s!=16 && s!=32) goto badWidth; }
    else if (t=='u') { if (s!=1) goto badWidth; }
    else if (t=='f') { if (s!=64) goto badWidth; }
    else thrM("•bit._cast: Type descriptor in 𝕗 must be one of \"iufnc\"");
    
  }
  return (CastType) { s, c };
  
}
static B convert(CastType t, B x, char* name) {
  switch (t.s) { default: thrF("•bit._%U: Unsupported data width %i", name, t.s);
    case  1: return taga(toBitArr(x));
    case  8: return t.c ? toC8Any (x) : toI8Any (x);
    case 16: return t.c ? toC16Any(x) : toI16Any(x);
    case 32: return t.c ? toC32Any(x) : toI32Any(x);
    case 64: return toF64Any(x);
  }
}
static TyArr* copy(CastType t, B x) {
  switch (t.s) { default: UD;
    case  1: return (TyArr*) (cpyBitArr(x));
    case  8: return (TyArr*) (t.c ? cpyC8Arr (x) : cpyI8Arr (x));
    case 16: return (TyArr*) (t.c ? cpyC16Arr(x) : cpyI16Arr(x));
    case 32: return (TyArr*) (t.c ? cpyC32Arr(x) : cpyI32Arr(x));
    case 64: return (TyArr*) (cpyF64Arr(x));
  }
}
static u8 typeOfCast(CastType t) {
  switch (t.s) { default: UD;
    case  1: return t_bitarr;
    case  8: return t.c ? t_c8arr  : t_i8arr ;
    case 16: return t.c ? t_c16arr : t_i16arr;
    case 32: return t.c ? t_c32arr : t_i32arr;
    case 64: return t_f64arr;
  }
}
static B set_bit_result(B r, u8 rt, ur rr, usz rl, usz *sh) {
  // Cast to output type
  v(r)->type = ARR_IS_SLICE(v(r)->type) ? ARR_TO_SLICE(rt) : rt;
  // Adjust shape
  Arr* a = a(r);
  if (rr<=1) {
    a->ia = rl;
    a->sh = &a->ia;
  } else {
    ShArr* old = shObj(r);
    if (old->refc>1) {
      usz* rsh = a->sh = m_shArr(rr)->a;
      shcpy(rsh, sh, rr-1);
      sh = rsh;
      ptr_decR(old); // shouldn't typically go to zero as refc>1 was just checked above, but can still happen if GC happens curing m_shArr; must be delayed to after m_shArr as sh==old->a for bitcast_impl
      SPRNK(a, rr);
    }
    sh[rr-1] = rl;
    a->ia = rl * shProd(sh, 0, rr-1);
  }
  return r;
}

B bitcast_impl(B el0, B el1, B x) {
  ur xr;
  if (!isArr(x) || (xr=RNK(x))<1) thrM("•bit._cast: 𝕩 must have rank at least 1");
  
  CastType xct = getCastType(el0, true, x);
  CastType rct = getCastType(el1, false, m_f64(0));
  usz* sh = SH(x);
  u64 s=xct.s*(u64)sh[xr-1], rl=s/rct.s;
  if (rl*rct.s != s) thrM("•bit._cast: Incompatible lengths");
  if (rl>=USZ_MAX) thrM("•bit._cast: Output too large");
  B r = convert(xct, x, "cast");
  u8 rt = typeOfCast(rct);
  if (rt==t_bitarr) {
    if (reusable(r) && !ARR_IS_SLICE(TY(r))) {
      REUSE(r);
    } else {
      r = taga(copy(xct, r));
    }
  } else if (!reusable(r)) {
    B r0 = incG(r);
    Arr* r2 = TI(r,slice)(r, 0, IA(r));
    r = taga(arr_shSetI(r2, xr, shObj(r0)));
    decG(r0);
    goto possibly_unaligned;
  } else {
    REUSE(r);
    #if VERIFY_TAIL
      if (xct.s==1 && rct.s!=1) {
        FINISH_OVERALLOC(a(r), offsetof(TyArr,a)+IA(r)/8, offsetof(TyArr,a) + (BIT_N(IA(r))<<3));
      }
    #endif
    goto possibly_unaligned;
  }
  
  if (0) {
    possibly_unaligned:;
    #if STRICT_ALIGN || FOR_BUILD
    if (IS_TYSLICE(TY(r))) {
      assert(rct.s != 1 && xct.s != 1); // rt==t_bitarr handles rct.s==1, and xct.s==1 never currently makes a slice (..though it could)
      u8 arrt = SLICE_TO_ARR(TY(r));
      void* r0p = tyslicev_ptr(a(r));
      if (ptr2u64(r0p) & ((rct.s>>3) - 1)) {
        Arr* r1;
        void* r2p = m_tyarrp(&r1, xct.s>>3, IA(r), arrt);
        memcpy(r2p, r0p, IA(r)*(xct.s>>3));
        arr_shCopyUnchecked(r1, r);
        decG(r);
        r = taga(r1);
      }
    }
    #endif
  }
  
  return set_bit_result(r, rt, xr, rl, sh);
}

B bitcast_c1(Md1D* d, B x) { B f = d->f;
  if (!isArr(f) || RNK(f)!=1 || IA(f)!=2) thrM("•bit._cast: 𝕗 must be a 2-element list (from‿to)");
  SGetU(f)
  return bitcast_impl(GetU(f,0), GetU(f,1), x);
}
B bitcast_im(Md1D* d, B x) { B f = d->f;
  if (!isArr(f) || RNK(f)!=1 || IA(f)!=2) thrM("•bit._cast: 𝕗 must be a 2-element list (from‿to)");
  SGetU(f)
  return bitcast_impl(GetU(f,1), GetU(f,0), x);
}

static usz req2(usz s, char* name) {
  usz top = 1ull << (8*sizeof(usz)-1); // Prevent 0 from passing
  if ((top|s) & (s-1)) thrF("•bit._%U: Sizes in 𝕗 must be powers of 2 (contained %s)", name, s);
  return s;
}

enum BitOp1 { op_not, op_neg };
enum BitOp2 { // sync with tables in bit_arith.singeli
  op_and, op_or, op_xor, // bitwise ops; must end with op_xor
  op_add, op_sub, op_mul, // elementwise ops; must start with op_add
};
#define OWL_8_64 if (!(owl>=3 && owl<=6)) thrF("•bit._%U: Operation width %i %S", name, ow, owl<3?"too small":"too large")

static B bitop1(B f, B x, enum BitOp1 op, char* name) {
  usz ow, rw, xw; // Operation width, result width, x width
  if (isAtm(f)) {
    ow = rw = xw = req2(o2s(f), name);
  } else {
    if (RNK(f)>1) thrF("•bit._%U: 𝕗 must have rank at most 1 (%i≡≠𝕗)", name, RNK(f));
    usz ia = IA(f);
    if (ia<1 || ia>3) thrF("•bit._%U: 𝕗 must contain between 1 and 3 numbers (%s≡≠𝕗)", name, ia);
    SGetU(f)
    usz t[3];
    PLAINLOOP for (usz i=0 ; i<ia; i++) t[i] = req2(o2s(GetU(f, i)), name);
    PLAINLOOP for (usz i=ia; i<3 ; i++) t[i] = t[ia-1];
    ow = t[0]; rw = t[1]; xw = t[2];
  }
  
  ur xr;
  if (!isArr(x) || (xr=RNK(x))<1) thrF("•bit._%U: 𝕩 must have rank at least 1", name);
  
  usz* sh = SH(x);
  usz rws = CTZ(rw);
  usz xws = CTZ(xw);
  u64 n = IA(x) << xws;
  u64 s = (u64)sh[xr-1] << xws;
  u64 rl = s >> rws;
  if ((s & (ow-1)) || (rl<<rws != s)) thrF("•bit._%U: Incompatible lengths", name);
  if (rl>=USZ_MAX) thrF("•bit._%U: Output too large", name);
  
  x = convert((CastType){ xw, isCharArr(x) }, x, name);
  u8 rt = typeOfCast((CastType){ rw, 0 });
  u64* xp = tyany_ptr(x);
  B r; u64* rp;
  if (!reusable(x) || ARR_IS_SLICE(TY(x))) {
    Arr* ra = m_arr(offsetof(TyArr,a) + (n+7)/8, rt, n>>rws);
    arr_shCopyUnchecked(ra, x);
    r = taga(ra); rp = tyany_ptr(r);
  } else {
    r = incG(REUSE(x)); rp = xp;
  }
  
  switch (op) { default: UD;
    case op_not: {
      usz l = n/64; bit_negatePtr(rp, xp, l);
      usz q = (-n)%64; if (q) rp[l] ^= (~(u64)0 >> q) & (rp[l]^~xp[l]);
    } break;
    
    case op_neg: {
    #if SINGELI
      u8 owl = CTZ(ow);
      OWL_8_64;
      if (n>0) si_bitarith_sa[BITARITH_IDX(op_sub)](rp, 0, xp, n >> owl);
    #else
      switch (ow) {
        default: thrF("•bit._%U: Unhandled operation width %s", name, ow);
        #define CASE(W) case W: \
          NOUNROLL vfor (usz i=0; i<n/W; i++) ((u##W*)rp)[i] = -((u##W*)xp)[i]; \
          break;
        CASE(8) CASE(16) CASE(32) CASE(64)
        #undef CASE
      }
    #endif
    } break;
  }
  
  set_bit_result(r, rt, xr, rl, sh);
  decG(x);
  return r;
}
B bitnot_c1(Md1D* d, B x) { return bitop1(d->f, x, op_not, "not"); }
B bitneg_c1(Md1D* d, B x) { return bitop1(d->f, x, op_neg, "neg"); }

static B bitop2(B f, B w, B x, enum BitOp2 op, char* name) {
  usz ow, rw, xw, ww; // Operation width, result width, x width, w width
  if (isAtm(f)) {
    ow = rw = xw = ww = req2(o2s(f), name);
  } else {
    if (RNK(f)>1) thrF("•bit._%U: 𝕗 must have rank at most 1 (%i≡≠𝕗)", name, RNK(f));
    usz ia = IA(f);
    if (ia<1 || ia>4) thrF("•bit._%U: 𝕗 must contain between 1 and 4 numbers (%s≡≠𝕗)", name, ia);
    SGetU(f)
    usz t[4];
    for (usz i=0 ; i<ia; i++) t[i] = req2(o2s(GetU(f, i)), name);
    for (usz i=ia; i<4 ; i++) t[i] = t[ia-1];
    ow = t[0]; rw = t[1]; xw = t[2]; ww = t[3];
  }
  
  if (isAtm(x)) x = m_unit(x);
  if (isAtm(w)) w = m_unit(w);
  ur wr=RNK(w); usz* wsh = SH(w); u64 s = wr==0? ww : ww*(u64)wsh[wr-1];
  ur xr=RNK(x); usz*  sh = SH(x); u64 t = xr==0? xw : xw*(u64) sh[xr-1];
  bool negw = 0; // Negate 𝕨 to subtract from 𝕩
  bool noextend = wr == xr && s == t;
  if (wr==xr && xr==0) thrF("•bit._%U: Some argument must have rank at least 1", name);
  if (noextend) {
    for (usz i=0; i<xr-1; i++) if (sh[i]!=wsh[i]) thrF("•bit._%U: 𝕨 and 𝕩 leading shapes must match", name);
  } else {
    if (wr>1 || s!=ow || xr==0) { // Need to extend 𝕩
      if (xr>1 || t!=ow || wr==0) {
        if (wr!=xr && wr>1 && xr>1) thrF("•bit._%U: 𝕨 and 𝕩 must have equal ranks if more than 1", name);
        thrF("•bit._%U: 𝕨 or 𝕩 1-cell width must equal operation width if extended", name);
      }
      { B t=w; w=x; x=t; }
      { usz t=ww; ww=xw; xw=t; }
      negw=op==op_sub; if (negw) op=op_add;
      t = s; xr = wr; sh = wsh;
    }
  }
  usz rws = CTZ(rw);
  u64 n = IA(x) << CTZ(xw);
  u64 rl = t >> rws;
  if ((t & (ow-1)) || (rl<<rws != t)) thrF("•bit._%U: Incompatible lengths", name);
  if (rl>=USZ_MAX) thrF("•bit._%U: Output too large", name);
  
  w = convert((CastType){ ww, isCharArr(w) }, w, name);
  x = convert((CastType){ xw, isCharArr(x) }, x, name);
  u8 rt = typeOfCast((CastType){ rw, 0 });
  Arr* ra = m_arr(offsetof(TyArr,a) + (n+7)/8, rt, n>>rws);
  arr_shCopyUnchecked(ra, x);
  B r = taga(ra);
  u64* wp = tyany_ptr(w);
  u64* xp = tyany_ptr(x);
  u64* rp = tyany_ptr(r);
  
  #if SINGELI
    u8 owl = CTZ(ow);
    if (noextend) {
      if (op <= op_xor) {
        if (n>0) si_bitwise_aa[op](rp, wp, xp, (n+7)/8);
      } else {
        OWL_8_64;
        if (n>0) si_bitarith_aa[BITARITH_IDX(op)](rp, wp, xp, n >> owl);
      }
    } else {
      if (owl > 6) thrF("•bit._%U: Scalar extension with width over 64 not supported", name, ow);
      u64 mul = si_spaced_masks[ow-1];
      u64 wv = loadu_u64(wp);
      if (op <= op_xor) {
        wv = (wv & ((1ULL<<ow)-1)) * mul;
        if (n>0) si_bitwise_as[op]((void*)rp, (void*)xp, wv, (n+7)/8);
      } else {
        OWL_8_64;
        if (negw) wv = -wv;
        if (n>0) si_bitarith_sa[BITARITH_IDX(op)](rp, wv, xp, n >> owl);
      }
    }
  #else
    #define CASES(O,Q,P) case op_##O: \
      switch(ow) { default: thrF("•bit._%U: Unhandled operation width %s", name, ow); \
        CASE(8,Q,P) CASE(16,Q,P) CASE(32,Q,P) CASE(64,Q,P)                  \
      } break;
    #define SWITCH \
      switch (op) { default: UD;                     \
        BINOP(and,&) BINOP(or,|) BINOP(xor,^)        \
        CASES(add,u,+) CASES(sub,u,-) CASES(mul,i,*) \
      }
    if (noextend) {
      #define BINOP(O,P) case op_##O: { \
        usz l = n/64; NOUNROLL vfor (usz i=0; i<l; i++) rp[i] = wp[i] P xp[i];     \
        usz q = (-n)%64; if (q) rp[l] ^= (~(u64)0 >> q) & (rp[l]^(wp[l] P xp[l])); \
        } break;
      #define CASE(W, Q, P) case W: \
        NOUNROLL vfor (usz i=0; i<n/W; i++)                 \
          ((Q##W*)rp)[i] = ((Q##W*)wp)[i] P ((Q##W*)xp)[i]; \
        break;
      SWITCH
      #undef BINOP
      #undef CASE
    } else {
      u64 wn; if (negw) { wn=-*wp; wp=&wn; }
      #define BINOP(O,P) case op_##O: { \
        if (ow>64) thrF("•bit._%U: Scalar extension with width over 64 not supported", name); \
        u64 wv = *wp & (~(u64)0>>(64-ow));                                      \
        for (usz tw=ow; tw<64; tw*=2) wv|=wv<<tw;                               \
        usz l = n/64; NOUNROLL vfor (usz i=0; i<l; i++) rp[i] = wv P xp[i];     \
        usz q = (-n)%64; if (q) rp[l] ^= (~(u64)0 >> q) & (rp[l]^(wv P xp[l])); \
        } break;
      #define CASE(W, Q, P) case W: { \
        Q##W wv = *(Q##W*)wp;                   \
        NOUNROLL vfor (usz i=0; i<n/W; i++)     \
          ((Q##W*)rp)[i] = wv P ((Q##W*)xp)[i]; \
        } break;
      SWITCH
      #undef BINOP
      #undef CASE
    }
    #undef CASES
    #undef SWITCH
  #endif
  
  set_bit_result(r, rt, xr, rl, sh);
  decG(w); decG(x);
  return r;
}
#define DEF_OP2(OP) \
  B bit##OP##_c2(Md1D* d, B w, B x) { return bitop2(d->f, w, x, op_##OP, #OP); }
DEF_OP2(and) DEF_OP2(or) DEF_OP2(xor)
DEF_OP2(add) DEF_OP2(sub) DEF_OP2(mul)
#undef DEF_OP2

STATIC_GLOBAL B bitNS;
B getBitNS(void) {
  if (bitNS.u == 0) {
    #define F(X) incG(bi_bit##X),
    Body* d = m_nnsDesc("cast","not","neg","and","or","xor","add","sub","mul");
    bitNS = m_nns(d,   F(cast)F(not)F(neg)F(and)F(or)F(xor)F(add)F(sub)F(mul));
    #undef F
    gc_add(bitNS);
  }
  return incG(bitNS);
}
