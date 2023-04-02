#include "../core.h"
#include "../utils/mut.h"
#include "../utils/calls.h"
#include "../utils/utf.h"
#include "../utils/talloc.h"
#include "../builtins.h"
#include <stdarg.h>

bool please_tail_call_err = true;

void before_exit(void);
bool inErr;
NORETURN NOINLINE void err(char* s) {
  NOGC_E;
  #if MM!=0
    gc_depth=1;
  #endif
  if (inErr) {
    fputs("\nCBQN encountered fatal error during information printing of another fatal error. Exiting without printing more info.", stderr);
    #ifdef DEBUG
      __builtin_trap();
    #endif
    exit(1);
  }
  inErr = true;
  fputs(s, stderr); fputc('\n', stderr); fflush(stderr);
  vm_pstLive(); fflush(stderr); fflush(stdout);
  print_vmStack(); fflush(stderr);
  fputs("CBQN interpreter entered unexpected state, exiting.\n", stderr);
  before_exit();
  #ifdef DEBUG
    __builtin_trap();
  #endif
  exit(1);
}

NOINLINE B c1F(B f, B x) { dec(x);
  if (isMd(f)) thrM("Calling a modifier");
  return inc(VALIDATE(f));
}
NOINLINE B c2F(B f, B w, B x) { dec(w); dec(x);
  if (isMd(f)) thrM("Calling a modifier");
  return inc(VALIDATE(f));
}
NOINLINE void value_freeF(Value* x) { value_free(x); }
NOINLINE void decA_F(B x) { dec(x); }
void noop_visit(Value* x) { }
#if HEAP_VERIFY
  void arr_visit(Value* x) { VISIT_SHAPE(x); }
#endif
NOINLINE B c1_bad(B f,      B x) { thrM("This function can't be called monadically"); }
NOINLINE B c2_bad(B f, B w, B x) { thrM("This function can't be called dyadically"); }
NOINLINE B m1c1_bad(Md1D* d,      B x) { thrM("This 1-modifier can't be called monadically"); }
NOINLINE B m1c2_bad(Md1D* d, B w, B x) { thrM("This 1-modifier can't be called dyadically"); }
NOINLINE B m2c1_bad(Md2D* d,      B x) { thrM("This 2-modifier can't be called monadically"); }
NOINLINE B m2c2_bad(Md2D* d, B w, B x) { thrM("This 2-modifier can't be called dyadically"); }

NOINLINE B md_c1(B t,      B x) { thrM("Cannot call a modifier"); }
NOINLINE B md_c2(B t, B w, B x) { thrM("Cannot call a modifier"); }
NOINLINE B arr_c1(B t,      B x) {         dec(x); return inc(t); }
NOINLINE B arr_c2(B t, B w, B x) { dec(w); dec(x); return inc(t); }


extern B rt_under, bi_before;
static B rtUnder_c1(B f, B g, B x) { // consumes x
  SLOW3("!runtime 𝕎⌾F 𝕩", f, x, g);
  B fn = m2_d(incG(rt_under), inc(f), inc(g));
  B r = c1(fn, x);
  decG(fn);
  return r;
}
static B rtUnder_cw(B f, B g, B w, B x) { // consumes w,x
  SLOW3("!runtime 𝔽⌾(𝕨⊸F) 𝕩", w, x, g);
  B fn = m2_d(incG(rt_under), inc(f), m2_d(incG(bi_before), w, inc(g)));
  B r = c1(fn, x);
  decG(fn);
  return r;
}
B def_fn_uc1(B t, B o,                B x) { return rtUnder_c1(o, t,    x); }
B def_fn_ucw(B t, B o,           B w, B x) { return rtUnder_cw(o, t, w, x); }
B def_m1_uc1(Md1* t, B o, B f,           B x) { B t2 = m1_d(tag(ptr_inc(t),MD1_TAG),inc(f)       ); B r = rtUnder_c1(o, t2,    x); decG(t2); return r; }
B def_m1_ucw(Md1* t, B o, B f,      B w, B x) { B t2 = m1_d(tag(ptr_inc(t),MD1_TAG),inc(f)       ); B r = rtUnder_cw(o, t2, w, x); decG(t2); return r; }
B def_m2_uc1(Md2* t, B o, B f, B g,      B x) { B t2 = m2_d(tag(ptr_inc(t),MD2_TAG),inc(f),inc(g)); B r = rtUnder_c1(o, t2,    x); decG(t2); return r; }
B def_m2_ucw(Md2* t, B o, B f, B g, B w, B x) { B t2 = m2_d(tag(ptr_inc(t),MD2_TAG),inc(f),inc(g)); B r = rtUnder_cw(o, t2, w, x); decG(t2); return r; }
B def_decompose(B x) {
  return m_hVec2(m_i32(isCallable(x)? (isImpureBuiltin(x)? 1 : 0) : -1),x);
}

B bi_emptyHVec, bi_emptyIVec, bi_emptyCVec, bi_emptySVec;

NOINLINE TStack* ts_e(TStack* o, u32 elsz, u64 am) { u64 size = o->size;
  u64 alsz = mm_round(fsizeof(TStack, data, u8, (size+am)*elsz));
  TStack* n;
  if (alsz==mm_sizeUsable((Value*)o)) {
    n = o;
  } else {
    n = (TStack*)mm_alloc(alsz, t_temp);
    memcpy(n->data, o->data, o->cap*elsz);
    mm_free((Value*)o);
    n->size = size;
  }
  n->cap = (mm_sizeUsable((Value*)n)-offsetof(TStack,data))/elsz;
  return n;
}

NOINLINE void farr_print(FILE* f, B x) { // should accept refc=0 arguments for debugging purposes
  ur r = RNK(x);
  SGetU(x)
  usz ia = IA(x);
  if (r!=1) {
    if (r==0) {
      fprintf(f, "<");
      fprintI(f, GetU(x,0));
      return;
    }
    usz* sh = SH(x);
    for (i32 i = 0; i < r; i++) {
      if(i!=0) fprintCodepoint(f, U'‿');
      fprintf(f, N64d, (u64)sh[i]);
    }
    fprintCodepoint(f, U'⥊');
  } else if (ia>0) {
    for (usz i = 0; i < ia; i++) {
      B c = GetU(x, i);
      if (!isC32(c) || o2cG(c)<32) goto reg;
    }
    fprintf(f, "\"");
    fprintsB(f, taga(a(x))); // allow incorrectly tagged input (e.g. by heapverify)
    fprintf(f, "\"");
    return;
  }
  reg:;
  fprintCodepoint(f, U'⟨');
  for (usz i = 0; i < ia; i++) {
    if (i!=0) fprintf(f, ", ");
    fprintI(f, GetU(x,i));
  }
  fprintCodepoint(f, U'⟩');
}

NOINLINE void fprintI(FILE* f, B x) {
  if (isF64(x)) {
    NUM_FMT_BUF(buf, x.f);
    fprintf(f, "%s", buf);
  } else if (isC32(x)) {
    if      (o2cG(x)>=32) fprintsU32(f, (u32[3]){'\'', o2cG(x), '\''}, 3);
    else if (o2cG(x)>=16) fprintf(f, "\\x%x", o2cG(x));
    else fprintf(f, "\\x0%x", o2cG(x));
  } else if (isVal(x)) {
    #ifdef DEBUG
    if (isVal(x) && (TY(x)==t_freed || TY(x)==t_empty)) {
      u8 t = TY(x);
      v(x)->type = v(x)->flags;
      fprintf(f, t==t_freed?"FREED:":"EMPTY:");
      TI(x,print)(f, x);
      v(x)->type = t;
      return;
    }
    #endif
    TI(x,print)(f, x);
  }
  else if (isVar(x)) fprintf(f, "(var d=%d i=%d)", (u16)(x.u>>32), (i32)x.u);
  else if (isExt(x)) fprintf(f, "(extvar d=%d i=%d)", (u16)(x.u>>32), (i32)x.u);
  else if (x.u==bi_N.u) fprintf(f, "(native ·)");
  else if (x.u==bi_optOut.u) fprintf(f, "(value optimized out)");
  else if (x.u==bi_noVar.u) fprintf(f, "(unset variable placeholder)");
  else if (x.u==bi_okHdr.u) fprintf(f, "(accepted SETH placeholder)");
  else if (x.u==bi_noFill.u) fprintf(f, "(no fill placeholder)");
  else fprintf(f, "(todo tag "N64x")", x.u>>48);
}

void printI   (B x) { fprintI   (stdout, x); }
void printsB  (B x) { fprintsB  (stdout, x); }
void arr_print(B x) { farr_print(stdout, x); }

i32 num_fmt(char buf[30], f64 x) {
  // for (int i = 0; i < 30; i++) buf[i] = 'a';
  snprintf(buf, 30, "%.16g", x); // should be %.17g to (probably?) never lose precision, but that also makes things ugly
  i32 len = strlen(buf);
  i32 neg = buf[0]=='-'?1:0;
  if (buf[neg] == 'i') {
    i32 o = neg*2;
    if (neg) { buf[0] = 0xC2; buf[1] = 0xAF; }
    buf[o] = 0xE2; buf[o+1] = 0x88; buf[o+2] = 0x9E; buf[o+3] = 0; len = o+3;
  } else if (buf[neg] == 'n') {
    buf[0] = 'N';  buf[1] = 'a';  buf[2] = 'N';  buf[3] = 0; len = 3;
  } else {
    if (buf[0] == '-') {
      memmove(buf+2, buf+1, len);
      buf[0] = 0xC2; buf[1] = 0xAF; // "¯""
      len+= 1;
    }
    for (i32 i = 0; i < len; i++) {
      if (buf[i] == 'e') {
        if (buf[i+1] == '+') {
          memcpy(buf+i+1, buf+i+2, len-i-1);
          len-= 1;
          break;
        } else if (buf[i+1] == '-') {
          memcpy(buf+i+3, buf+i+2, len-i-1);
          buf[i+1] = 0xC2; buf[i+2] = 0xAF;
        }
      }
    }
  }
  return len;
}

static B appendRaw(B s, B x) { assert(isArr(x) && RNK(x)==1); // consumes x
  if (TI(x,elType)==el_c32) AJOIN(x);
  else {
    B sq = chr_squeezeChk(x);
    if (!elChr(TI(sq,elType))) FL_KEEP(sq, ~fl_squoze);
    AJOIN(sq);
  }
  return s;
}
NOINLINE B do_fmt(B s, char* p, va_list a) {
  char buf[30];
  char c;
  char* lp = p;
  while (*p != 0) { c = *p++;
    if (c!='%') continue;
    if (lp!=p-1) AJOIN(utf8Decode(lp, p-1-lp));
    switch(c = *p++) { default: printf("Unknown format character '%c'", c); err(""); UD;
      case 'R': {
        B b = va_arg(a, B);
        if (isNum(b)) AFMT("%f", o2f(b));
        else s = appendRaw(s, inc(b));
        break;
      }
      case 'B': {
        B b = va_arg(a, B);
        s = appendRaw(s, bqn_repr(inc(b)));
        break;
      }
      case '2':
      case 'H': {
        ur r;
        usz* sh;
        if (c=='2') {
          if ('H' != *p++) err("Invalid format string: expected H after %2");
          r = va_arg(a, int);
          sh = va_arg(a, usz*);
        } else {
          B o = va_arg(a, B);
          r = isArr(o)? RNK(o) : 0;
          sh = isArr(o)? SH(o) : NULL;
        }
        if (r==0) AU("⟨⟩");
        else if (r==1) AFMT("⟨%s⟩", sh[0]);
        else {
          for (i32 i = 0; i < r; i++) {
            if(i) AU("‿");
            AFMT("%s", sh[i]);
          }
        }
        break;
      }
      case 'S': {
        A8(va_arg(a, char*));
        break;
      }
      case 'U': {
        AU(va_arg(a, char*));
        break;
      }
      case 'u': case 'x': case 'i': case 'l': {
        i32 mode = c=='u'? 1 : c=='x'? 2 : 0;
        if (mode) c = *p++;
        assert(c);
        if (mode) {
         assert(c=='l'||c=='i');
          if (c=='i') snprintf(buf, 30, mode==1? "%u" : "%x", va_arg(a, u32));
          else        snprintf(buf, 30, mode==1? N64u : N64x, va_arg(a, u64));
        } else {
          if (c=='i') {
            i32 v = va_arg(a, i32);
            if (v<0) AU("¯");
            snprintf(buf, 30, N64u, (u64)(v<0?-v:v));
          } else { assert(c=='l');
            i64 v = va_arg(a, i64);
            if (v<0) AU("¯");
            if (v==I64_MIN) snprintf(buf, 30, "9223372036854775808");
            else snprintf(buf, 30, N64u, (u64)(v<0?-v:v));
          }
        }
        A8(buf);
        break;
      }
      case 's': {
        usz v = va_arg(a, usz);
        snprintf(buf, 30, sizeof(usz)==4? "%u" : N64u, v);
        A8(buf);
        break;
      }
      case 'p': {
        snprintf(buf, 30, "%p", va_arg(a, void*));
        A8(buf);
        break;
      }
      case 'f': {
        NUM_FMT_BUF(buf, va_arg(a, f64));
        AU(buf);
        break;
      }
      case 'c': {
        buf[0] = va_arg(a, int); buf[1] = 0;
        A8(buf);
        break;
      }
      case '%': {
        buf[0] = '%'; buf[1] = 0;
        A8(buf);
        break;
      }
    }
    lp = p;
  }
  if (lp!=p) AJOIN(utf8Decode(lp, p-lp));
  return s;
}
NOINLINE B append_fmt(B s, char* p, ...) {
  va_list a;
  va_start(a, p);
  B r = do_fmt(s, p, a);
  va_end(a);
  return r;
}
NOINLINE B make_fmt(char* p, ...) {
  va_list a;
  va_start(a, p);
  B r = do_fmt(emptyCVec(), p, a);
  va_end(a);
  return r;
}
NOINLINE void fprint_fmt(FILE* f, char* p, ...) {
  va_list a;
  va_start(a, p);
  B r = do_fmt(emptyCVec(), p, a);
  va_end(a);
  fprintsB(f, r);
  decG(r);
}
NOINLINE void print_fmt(char* p, ...) {
  va_list a;
  va_start(a, p);
  B r = do_fmt(emptyCVec(), p, a);
  va_end(a);
  printsB(r);
  decG(r);
}
NOINLINE void thrF(char* p, ...) {
  va_list a;
  va_start(a, p);
  B r = do_fmt(emptyCVec(), p, a);
  va_end(a);
  thr(r);
}


#define CMP(W,X) ({ AUTO wt = (W); AUTO xt = (X); (wt>xt?1:0)-(wt<xt?1:0); })
NOINLINE i32 compareF(B w, B x) {
  if (isNum(w) & isC32(x)) return -1;
  if (isC32(w) & isNum(x)) return  1;
  if (isAtm(w) & isAtm(x)) thrM("Invalid comparison");
  bool wa=isAtm(w); usz wia; ur wr; usz* wsh; AS2B wgetU; Arr* wArr;
  bool xa=isAtm(x); usz xia; ur xr; usz* xsh; AS2B xgetU; Arr* xArr;
  if(wa) { wia=1; wr=0; wsh=NULL; } else { wia=IA(w); wr=RNK(w); wsh=SH(w); wgetU=TI(w,getU); wArr = a(w); }
  if(xa) { xia=1; xr=0; xsh=NULL; } else { xia=IA(x); xr=RNK(x); xsh=SH(x); xgetU=TI(x,getU); xArr = a(x); }
  if (wia==0 || xia==0) return CMP(wia, xia);
  
  i32 rc = CMP(wr+(wa?0:1), xr+(xa?0:1));
  ur rr = wr<xr? wr : xr;
  i32 ri = 0; // matching shape tail
  i32 rm = 1;
  while (ri<rr  &&  wsh[wr-1-ri] == xsh[xr-1-ri]) {
    rm*= wsh[wr-ri-1];
    ri++;
  }
  if (ri<rr) {
    usz wm = wsh[wr-1-ri];
    usz xm = xsh[xr-1-ri];
    rc = CMP(wm, xm);
    rm*= wm<xm? wm : xm;
  }
  for (u64 i = 0; i < (u64)rm; i++) {
    int c = compare(wa? w : wgetU(wArr,i), xa? x : xgetU(xArr,i));
    if (c!=0) return c;
  }
  return rc;
}
#undef CMP

NOINLINE bool atomEqualF(B w, B x) {
  if (TY(w)!=TY(x)) return false;
  B2B dcf = TI(w,decompose);
  if (dcf == def_decompose) return false;
  B wd=dcf(inc(w)); B* wdp = harr_ptr(wd);
  B xd=dcf(inc(x)); B* xdp = harr_ptr(xd);
  if (o2i(wdp[0])<=1) { decG(wd);decG(xd); return false; }
  usz wia = IA(wd);
  if (wia!=IA(xd)) { decG(wd);decG(xd); return false; }
  for (u64 i = 0; i<wia; i++) if(!equal(wdp[i], xdp[i]))
                      { decG(wd);decG(xd); return false; }
                        decG(wd);decG(xd); return true;
}

// Functions in eqFns compare segments for matching
// data argument comes from eqFnData
static const u8 n = 99;
u8 eqFnData[] = { // for the main diagonal, amount to shift length by; otherwise, whether to swap arguments
  0,0,0,0,0,n,n,n,
  1,0,0,0,0,n,n,n,
  1,1,1,0,0,n,n,n,
  1,1,1,2,0,n,n,n,
  1,1,1,1,0,n,n,n,
  n,n,n,n,n,0,0,0,
  n,n,n,n,n,1,1,0,
  n,n,n,n,n,1,1,2,
};

#if SINGELI_SIMD || SINGELI_X86_64
  #define F(X) simd_equal_##X
  #define SINGELI_FILE equal
  #include "../utils/includeSingeli.h"
#else
  #define F(X) equal_##X
  bool F(1_1)(void* w, void* x, u64 l, u64 d) {
    u64* wp = w; u64* xp = x;
    usz q = l/64;
    for (usz i=0; i<q; i++) if (wp[i] != xp[i]) return false;
    usz r = (-l)%64; return r==0 || (wp[q]^xp[q])<<r == 0;
  }
  #define DEF_EQ_U1(N, T) \
    bool F(1_##N)(void* w, void* x, u64 l, u64 d) {                    \
      if (d!=0) { void* t=w; w=x; x=t; }                               \
      u64* wp = w; T* xp = x;                                          \
      for (usz i=0; i<l; i++) if (bitp_get(wp,i)!=xp[i]) return false; \
      return true;                                                     \
    }
  DEF_EQ_U1(8, i8)
  DEF_EQ_U1(16, i16)
  DEF_EQ_U1(32, i32)
  DEF_EQ_U1(f64, f64)
  #undef DEF_EQ_U1

  #define DEF_EQ_I(NAME, S, T, INIT) \
    bool F(NAME)(void* w, void* x, u64 l, u64 d) {            \
      INIT                                                    \
      S* wp = w; T* xp = x;                                   \
      for (usz i=0; i<l; i++) if (wp[i]!=xp[i]) return false; \
      return true;                                            \
    }
  #define DEF_EQ(N,S,T) DEF_EQ_I(N,S,T, if (d!=0) { void* t=w; w=x; x=t; })
  DEF_EQ_I(8_8, u8, u8, l<<=d;)
  DEF_EQ_I(f64_f64, f64, f64, )
  DEF_EQ(u8_16,  u8, u16)
  DEF_EQ(u8_32,  u8, u32) DEF_EQ(u16_32,  u16, u32)
  DEF_EQ(s8_16,  i8, i16)
  DEF_EQ(s8_32,  i8, i32) DEF_EQ(s16_32,  i16, i32)
  DEF_EQ(s8_f64, i8, f64) DEF_EQ(s16_f64, i16, f64) DEF_EQ(s32_f64, i32, f64)
  #undef DEF_EQ_I
  #undef DEF_EQ
#endif
bool notEq(void* a, void* b, u64 l, u64 data) { return false; }
EqFn eqFns[] = {
  F(1_1),   F(1_8),    F(1_16),    F(1_32),    F(1_f64),   notEq,    notEq,     notEq,
  F(1_8),   F(8_8),    F(s8_16),   F(s8_32),   F(s8_f64),  notEq,    notEq,     notEq,
  F(1_16),  F(s8_16),  F(8_8),     F(s16_32),  F(s16_f64), notEq,    notEq,     notEq,
  F(1_32),  F(s8_32),  F(s16_32),  F(8_8),     F(s32_f64), notEq,    notEq,     notEq,
  F(1_f64), F(s8_f64), F(s16_f64), F(s32_f64), F(f64_f64), notEq,    notEq,     notEq,
  notEq,    notEq,     notEq,      notEq,      notEq,      F(8_8),   F(u8_16),  F(u8_32),
  notEq,    notEq,     notEq,      notEq,      notEq,      F(u8_16), F(8_8),    F(u16_32),
  notEq,    notEq,     notEq,      notEq,      notEq,      F(u8_32), F(u16_32), F(8_8),
};
#undef F

NOINLINE bool equalSlow(B w, B x, usz ia);
NOINLINE bool equal(B w, B x) { // doesn't consume
  bool wa = isAtm(w);
  bool xa = isAtm(x);
  if (wa!=xa) return false;
  if (wa) return atomEqual(w, x);
  ur wr = RNK(w);
  ur xr = RNK(x);
  if (wr!=xr) return false;
  usz ia = IA(x);
  if (LIKELY(wr==1)) {
    if (ia != IA(w)) return false;
  } else {
    usz* wsh = SH(w);
    usz* xsh = SH(x);
    if (wsh!=xsh) for (usz i = 0; i < wr; i++) if (wsh[i]!=xsh[i]) return false;
  }
  if (ia==0) return true;
  u8 we = TI(w,elType);
  u8 xe = TI(x,elType);
  
  if (we<=el_c32 && xe<=el_c32) { // remove & pass a(w) and a(x) to fn so it can do basic loop
    usz idx = EQFN_INDEX(we, xe);
    return eqFns[idx](tyany_ptr(w), tyany_ptr(x), ia, eqFnData[idx]);
  }
  return equalSlow(w, x, ia);
}
bool equalSlow(B w, B x, usz ia) {
  SLOW2("equal", w, x);
  SGetU(x)
  SGetU(w)
  for (usz i = 0; i < ia; i++) if(!equal(GetU(w,i),GetU(x,i))) return false;
  return true;
}

bool atomEEqual(B w, B x) { // doesn't consume (not that that matters really currently)
  if (w.u==x.u) return true;
  #if !NEEQUAL_NEGZERO
    if (isF64(w)&isF64(x)) return w.f==x.f;
  #endif
  if(isF64(w)|isF64(x)) return false;
  if (!isVal(w) | !isVal(x)) return false;
  if (TY(w)!=TY(x)) return false;
  B2B dcf = TI(w,decompose);
  if (dcf == def_decompose) return false;
  B wd=dcf(inc(w)); B* wdp = harr_ptr(wd);
  B xd=dcf(inc(x)); B* xdp = harr_ptr(xd);
  if (o2i(wdp[0])<=1) { decG(wd);decG(xd); return false; }
  usz wia = IA(wd);
  if (wia!=IA(xd)) { decG(wd);decG(xd); return false; }
  for (u64 i = 0; i<wia; i++) if(!eequal(wdp[i], xdp[i]))
                      { decG(wd);decG(xd); return false; }
                        decG(wd);dec(xd); return true;
}
bool eequal(B w, B x) { // doesn't consume
  if (w.u==x.u) return true;
  bool wa = isAtm(w);
  bool xa = isAtm(x);
  if (wa!=xa) return false;
  if (wa) return atomEEqual(w, x);
  // B wf = getFillQ(w);
  // B xf = getFillQ(x);
  // bool feq = eequal(wf, xf);
  // dec(wf); dec(xf);
  // if (!feq) return false;
  if (!eqShape(w,x)) return false;
  u8 we = TI(w,elType);
  u8 xe = TI(x,elType);
  if (we==el_f64 && xe==el_f64) {
    usz ia = IA(x);
    f64* wp = f64any_ptr(w);
    f64* xp = f64any_ptr(x);
    u64 r = 1;
    for (usz i = 0; i < ia; i++) {
      #if NEEQUAL_NEGZERO
      r&= ((u64*)wp)[i] == ((u64*)xp)[i];
      #else
      r&= (wp[i]==xp[i]) | (wp[i]!=wp[i] & xp[i]!=xp[i]);
      #endif
    }
    return r;
  }
  if (we!=el_B && xe!=el_B) return equal(w, x);
  usz ia = IA(x);
  SGetU(x)
  SGetU(w)
  for (usz i = 0; i < ia; i++) if(!eequal(GetU(w,i),GetU(x,i))) return false;
  return true;
}

i64 bit_sum(u64* x, u64 am) {
  i64 r = 0;
  for (u64 i = 0; i < (am>>6); i++) r+= POPC(x[i]);
  if (am&63) r+= POPC(x[am>>6]<<(64-am & 63));
  return r;
}

usz depthF(B x) { // doesn't consume
  u64 r = 0;
  usz ia = IA(x);
  SGetU(x)
  for (usz i = 0; i < ia; i++) {
    u64 n = depth(GetU(x,i));
    if (n>r) r = n;
  }
  return r+1;
}
void tyarr_freeO(Value* x) { decSh(x); }
void slice_freeO(Value* x) { ptr_dec(((Slice*)x)->p); decSh(x); }
void tyarr_freeF(Value* x) { tyarr_freeO(x); mm_free(x); }
void slice_freeF(Value* x) { slice_freeO(x); mm_free(x); }
void slice_visit(Value* x) { mm_visitP(((Slice*)x)->p); VISIT_SHAPE(x); }
void slice_print(B x) { arr_print(x); }

char* type_repr(u8 u) {
  switch(u) { default: return "(unknown type)";
    #define F(X) case t_##X: return #X;
    FOR_TYPE(F)
    #undef F
  }
}
char* eltype_repr(u8 u) {
  switch(u) { default: return "(bad elType)";
    case el_bit: return "el_bit"; case el_f64: return "el_f64"; case el_B: return "el_B";
    case el_i8:  return "el_i8";  case el_c8:  return "el_c8";
    case el_i16: return "el_i16"; case el_c16: return "el_c16";
    case el_i32: return "el_i32"; case el_c32: return "el_c32";
  }
}
bool isPureFn(B x) { // doesn't consume
  if (isCallable(x)) {
    if (isPrim(x)) return true;
    B2B dcf = TI(x,decompose);
    B xd = dcf(inc(x));
    B* xdp = harr_ptr(xd);
    i32 t = o2iG(xdp[0]);
    if (t<2) { decG(xd); return t==0; }
    usz xdia = IA(xd);
    for (u64 i = 1; i<xdia; i++) if(!isPureFn(xdp[i])) { decG(xd); return false; }
    decG(xd); return true;
  } else if (isArr(x)) {
    usz ia = IA(x);
    SGetU(x)
    for (usz i = 0; i < ia; i++) if (!isPureFn(GetU(x,i))) return false;
    return true;
  } else return isNum(x) || isC32(x);
}

B bqn_merge(B x) {
  assert(isArr(x));
  usz xia = IA(x);
  ur xr = RNK(x);
  if (xia==0) {
    B xf = getFillE(x);
    if (isAtm(xf)) { dec(xf); return x; }
    i32 xfr = RNK(xf);
    Arr* r = m_fillarrpEmpty(getFillQ(xf));
    if (xr+xfr > UR_MAX) thrM(">: Result rank too large");
    usz* rsh = arr_shAlloc(r, xr+xfr);
    if (rsh) {
      shcpy       (rsh   , SH(x),  xr);
      if(xfr)shcpy(rsh+xr, SH(xf), xfr);
    }
    decG(x); dec(xf);
    return taga(r);
  }
  
  SGetU(x)
  B x0 = GetU(x, 0);
  usz* elSh = isArr(x0)? SH(x0) : NULL;
  ur elR = isArr(x0)? RNK(x0) : 0;
  usz elIA = isArr(x0)? IA(x0) : 1;
  B fill = getFillQ(x0);
  if (xr+elR > UR_MAX) thrM(">: Result rank too large");
  
  MAKE_MUT(r, xia*elIA);
  usz rp = 0;
  for (usz i = 0; i < xia; i++) {
    B c = GetU(x, i);
    if (isArr(c)? (elR!=RNK(c) || !eqShPart(elSh, SH(c), elR)) : elR!=0) { mut_pfree(r, rp); thrF(">: Elements didn't have equal shapes (contained shapes %H and %H)", x0, c); }
    if (isArr(c)) mut_copy(r, rp, c, 0, elIA);
    else mut_set(r, rp, inc(c));
    if (!noFill(fill)) fill = fill_or(fill, getFillQ(c));
    rp+= elIA;
  }
  Arr* ra = mut_fp(r);
  usz* rsh = arr_shAlloc(ra, xr+elR);
  if (rsh) {
    shcpy         (rsh   , SH(x), xr);
    if (elSh)shcpy(rsh+xr, elSh,  elR);
  }
  decG(x);
  return withFill(taga(ra),fill);
}

#ifdef ALLOC_STAT
  u64* ctr_a = 0;
  u64* ctr_f = 0;
  u64 actrc = 21000;
  u64 talloc = 0;
  #ifdef ALLOC_SIZES
    u32** actrs;
  #endif
#endif

NOINLINE void print_allocStats() {
  #ifdef ALLOC_STAT
    printf("total ever allocated: "N64u"\n", talloc);
    printf("allocated heap size:  "N64u"\n", mm_heapAlloc);
    printf("used heap size:       "N64u"\n", mm_heapUsed());
    ctr_a[t_harr]+= ctr_a[t_harrPartial];
    ctr_a[t_harrPartial] = 0;
    printf("ctrA←"); for (i64 i = 0; i < t_COUNT; i++) { if(i)printf("‿"); printf(N64u, ctr_a[i]); } printf("\n");
    printf("ctrF←"); for (i64 i = 0; i < t_COUNT; i++) { if(i)printf("‿"); printf(N64u, ctr_f[i]); } printf("\n");
    printf("names←⟨"); for (i64 i = 0; i < t_COUNT; i++) { if(i)printf(","); printf("\"%s\"", type_repr(i)); } printf("⟩\n");
    u64 leakedCount = 0;
    for (i64 i = 0; i < t_COUNT; i++) leakedCount+= ctr_a[i]-ctr_f[i];
    printf("leaked object count: "N64u"\n", leakedCount);
    #ifdef ALLOC_SIZES
      for(i64 i = 0; i < actrc; i++) {
        u32* c = actrs[i];
        bool any = false;
        for (i64 j = 0; j < t_COUNT; j++) if (c[j]) any=true;
        if (any) {
          printf(N64u, i*4);
          for (i64 k = 0; k < t_COUNT; k++) printf("‿%u", c[k]);
          printf("\n");
        }
      }
    #endif
  #endif
}


#if USE_VALGRIND
  #include "../utils/valgrind.c"
#endif

// for gdb
B info_c2(B, B, B);
Value* g_v(B x) { return v(x); }
Arr*   g_a(B x) { return a(x); }
B      g_t (void* x) { return tag(x,OBJ_TAG); }
B      g_ta(void* x) { return tag(x,ARR_TAG); }
B      g_tf(void* x) { return tag(x,FUN_TAG); }
bool ignore_bad_tag;
void   g_p(B x) { printI(x); putchar(10); fflush(stdout); }
void   g_i(B x) { B r = info_c2(x, m_f64(1), inc(x)); printI(r); dec(r); putchar(10); fflush(stdout); }
void   g_pv(void* x) { ignore_bad_tag=true; printI(tag(x,OBJ_TAG)); putchar(10); fflush(stdout); ignore_bad_tag=false; }
void   g_iv(void* x) { ignore_bad_tag=true; B xo = tag(x, OBJ_TAG); B r = C2(info, m_f64(1), inc(xo)); printI(r); dec(r); putchar(10); fflush(stdout); ignore_bad_tag=false; }
void   g_pst(void) { vm_pstLive(); fflush(stdout); fflush(stderr); }

#ifdef DEBUG
  bool cbqn_noAlloc;
  NOINLINE void cbqn_NOGC_start() { cbqn_noAlloc=true; }
  #ifdef OBJ_COUNTER
    #define PRINT_ID(X) fprintf(stderr, "Object ID: "N64u"\n", (X)->uid)
  #else
    #define PRINT_ID(X)
  #endif
  NOINLINE Value* VALIDATEP(Value* x) {
    if (x->refc<=0 || (x->refc>>28) == 'a' || x->type==t_empty) {
      PRINT_ID(x);
      fprintf(stderr, "bad refcount for type %d @ %p: %d\nattempting to print: ", x->type, x, x->refc); fflush(stderr);
      fprintI(stderr, tag(x,OBJ_TAG)); fputc('\n', stderr); fflush(stderr);
      err("");
    }
    if (TIv(x,isArr)) {
      Arr* a = (Arr*)x;
      if (PRNK(x)<=1) assert(a->sh == &a->ia);
      else {
        assert(shProd(PSH(a), 0, PRNK(x)) == a->ia);
        VALIDATE(tag(shObjP(x),OBJ_TAG));
      }
    }
    return x;
  }
  NOINLINE B VALIDATE(B x) {
    if (!isVal(x)) return x;
    VALIDATEP(v(x));
    if(isArr(x)!=TI(x,isArr) && v(x)->type!=t_freed && v(x)->type!=t_harrPartial && !ignore_bad_tag) {
      fprintf(stderr, "bad array tag/type: type=%d, obj=%p\n", v(x)->type, TOPTR(void, x.u));
      PRINT_ID(v(x));
      fprintI(stderr, x);
      err("\n");
    }
    return x;
  }
  NOINLINE NORETURN void assert_fail(char* expr, char* file, int line, const char fn[]) {
    fprintf(stderr, "%s:%d: %s: Assertion `%s` failed.\n", file, line, fn, expr);
    err("");
  }
#endif
#if WARN_SLOW
  #if WARN_SLOW==2
    #define ONLY_ALWAYS if (!always) return
  #else
    #define ONLY_ALWAYS
  #endif
  extern bool cbqn_initialized;
  static void warn_ln(B x) {
    if (isArr(x)) fprint_fmt(stderr, "%s items, %S, shape=%H\n", IA(x), eltype_repr(TI(x,elType)), x);
    else {
      fprintf(stderr, "atom: ");
      if (cbqn_initialized && FORMATTER) { fprintsB(stderr, x = bqn_fmt(inc(x))); dec(x); }
      else fprintI(stderr, x);
      fputc('\n', stderr);
    }
  }
  void warn_slow1(char* s, B x) {
    bool always = '!'==*s;
    if (always) s++;
    else if (isArr(x) && IA(x)<100) return;
    ONLY_ALWAYS;
    fprintf(stderr, "slow %s: ", s); warn_ln(x);
    fflush(stderr);
  }
  void warn_slow2(char* s, B w, B x) {
    bool always = '!'==*s;
    if (always) s++;
    else if ((isArr(w)||isArr(x))  &&  (!isArr(w) || IA(w)<50)  &&  (!isArr(x) || IA(x)<50)) return;
    ONLY_ALWAYS;
    fprintf(stderr, "slow %s:\n  𝕨: ", s); warn_ln(w);
    fprintf(stderr, "  𝕩: "); warn_ln(x);
    fflush(stderr);
  }
  void warn_slow3(char* s, B w, B x, B y) {
    bool always = '!'==*s;
    if (always) s++;
    else if ((isArr(w)||isArr(x))  &&  (!isArr(w) || IA(w)<50)  &&  (!isArr(x) || IA(x)<50)) return;
    ONLY_ALWAYS;
    fprintf(stderr, "slow %s:\n  𝕨: ", s); warn_ln(w);
    fprintf(stderr, "  𝕩: "); warn_ln(x);
    fprintf(stderr, "  f: "); warn_ln(y);
    fflush(stderr);
  }
#endif
