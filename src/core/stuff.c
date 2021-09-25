#include "../core.h"
#include "../utils/mut.h"
#include "../utils/utf.h"
#include "../utils/talloc.h"
#include "../builtins.h"


NORETURN NOINLINE void err(char* s) {
  puts(s); fflush(stdout);
  vm_pstLive(); fflush(stdout);
  print_vmStack(); fflush(stdout);
  puts("CBQN interpreter entered unexpected state, exiting.");
  #ifdef DEBUG
    __builtin_trap();
  #endif
  exit(1);
}

NOINLINE B c1_rare(B f, B x) { dec(x);
  if (isMd(f)) thrM("Calling a modifier");
  return inc(VALIDATE(f));
}
NOINLINE B c2_rare(B f, B w, B x) { dec(w); dec(x);
  if (isMd(f)) thrM("Calling a modifier");
  return inc(VALIDATE(f));
}
NOINLINE void value_freeR(Value* x) { value_free(x); }
NOINLINE void decA_rare(B x) { dec(x); }
void noop_visit(Value* x) { }
NOINLINE B c1_bad(B f,      B x) { thrM("This function can't be called monadically"); }
NOINLINE B c2_bad(B f, B w, B x) { thrM("This function can't be called dyadically"); }
NOINLINE B m1c1_bad(Md1D* d,      B x) { thrM("This 1-modifier can't be called monadically"); }
NOINLINE B m1c2_bad(Md1D* d, B w, B x) { thrM("This 1-modifier can't be called dyadically"); }
NOINLINE B m2c1_bad(Md2D* d,      B x) { thrM("This 2-modifier can't be called monadically"); }
NOINLINE B m2c2_bad(Md2D* d, B w, B x) { thrM("This 2-modifier can't be called dyadically"); }
extern B rt_under, bi_before;
static B rtUnder_c1(B f, B g, B x) { // consumes x
  B fn = m2_d(inc(rt_under), inc(f), inc(g));
  B r = c1(fn, x);
  dec(fn);
  return r;
}
static B rtUnder_cw(B f, B g, B w, B x) { // consumes w,x
  B fn = m2_d(inc(rt_under), inc(f), m2_d(inc(bi_before), w, inc(g)));
  B r = c1(fn, x);
  dec(fn);
  return r;
}
B def_fn_uc1(B t, B o,                B x) { return rtUnder_c1(o, t,    x); }
B def_fn_ucw(B t, B o,           B w, B x) { return rtUnder_cw(o, t, w, x); }
B def_m1_uc1(B t, B o, B f,           B x) { B t2 = m1_d(inc(t),inc(f)       ); B r = rtUnder_c1(o, t2,    x); dec(t2); return r; }
B def_m1_ucw(B t, B o, B f,      B w, B x) { B t2 = m1_d(inc(t),inc(f)       ); B r = rtUnder_cw(o, t2, w, x); dec(t2); return r; }
B def_m2_uc1(B t, B o, B f, B g,      B x) { B t2 = m2_d(inc(t),inc(f),inc(g)); B r = rtUnder_c1(o, t2,    x); dec(t2); return r; }
B def_m2_ucw(B t, B o, B f, B g, B w, B x) { B t2 = m2_d(inc(t),inc(f),inc(g)); B r = rtUnder_cw(o, t2, w, x); dec(t2); return r; }
B def_decompose(B x) {
  return m_v2(m_i32(isCallable(x)? (isImpureBuiltin(x)? 1 : 0) : -1),x);
}

B bi_emptyHVec, bi_emptyIVec, bi_emptyCVec, bi_emptySVec;

NOINLINE TStack* ts_e(TStack* o, u32 elsz, u64 am) { u64 size = o->size;
  u64 alsz = mm_round(fsizeof(TStack, data, u8, (size+am)*elsz));
  TStack* n;
  if (alsz==mm_size((Value*)o)) {
    n = o;
  } else {
    n = (TStack*)mm_alloc(alsz, t_temp);
    memcpy(n->data, o->data, o->cap*elsz);
    mm_free((Value*)o);
    n->size = size;
  }
  n->cap = (mm_size((Value*)n)-offsetof(TStack,data))/elsz;
  return n;
}

NOINLINE void arr_print(B x) { // should accept refc=0 arguments for debugging purposes
  ur r = rnk(x);
  SGetU(x)
  usz ia = a(x)->ia;
  if (r!=1) {
    if (r==0) {
      printf("<");
      print(GetU(x,0));
      return;
    }
    usz* sh = a(x)->sh;
    for (i32 i = 0; i < r; i++) {
      if(i==0)printf(N64d,(u64)sh[i]);
      else printf("‿"N64d,(u64)sh[i]);
    }
    printf("⥊");
  } else if (ia>0) {
    for (usz i = 0; i < ia; i++) {
      B c = GetU(x,i);
      if (!isC32(c) || (u32)c.u=='\n') goto reg;
    }
    printf("\"");
    for (usz i = 0; i < ia; i++) printUTF8((u32)GetU(x,i).u); // c32, no need to decrement
    printf("\"");
    return;
  }
  reg:;
  printf("⟨");
  for (usz i = 0; i < ia; i++) {
    if (i!=0) printf(", ");
    print(GetU(x,i));
  }
  printf("⟩");
}

NOINLINE void print(B x) {
  if (isF64(x)) {
    NUM_FMT_BUF(buf, x.f);
    printf("%s", buf);
  } else if (isC32(x)) {
    if ((u32)x.u>=32) { printf("'"); printUTF8((u32)x.u); printf("'"); }
    else if((u32)x.u>15) printf("\\x%x", (u32)x.u);
    else printf("\\x0%x", (u32)x.u);
  } else if (isVal(x)) {
    #ifdef DEBUG
    if (isVal(x) && (v(x)->type==t_freed || v(x)->type==t_empty)) {
      u8 t = v(x)->type;
      v(x)->type = v(x)->flags;
      printf(t==t_freed?"FREED:":"EMPTY:");
      TI(x,print)(x);
      v(x)->type = t;
      return;
    }
    #endif
    TI(x,print)(x);
  }
  else if (isVar(x)) printf("(var d=%d i=%d)", (u16)(x.u>>32), (i32)x.u);
  else if (isExt(x)) printf("(extvar d=%d i=%d)", (u16)(x.u>>32), (i32)x.u);
  else if (x.u==bi_N.u) printf("·");
  else if (x.u==bi_optOut.u) printf("(value optimized out)");
  else if (x.u==bi_noVar.u) printf("(unset variable placeholder)");
  else if (x.u==bi_okHdr.u) printf("(accepted SETH placeholder)");
  else if (x.u==bi_noFill.u) printf("(no fill placeholder)");
  else printf("(todo tag "N64x")", x.u>>48);
}

NOINLINE void printRaw(B x) {
  if (isAtm(x)) {
    if (isF64(x)) { NUM_FMT_BUF(buf, x.f); printf("%s", buf); }
    else if (isC32(x)) printUTF8((u32)x.u);
    else thrM("bad printRaw argument: atom arguments should be either numerical or characters");
  } else {
    usz ia = a(x)->ia;
    SGetU(x)
    for (usz i = 0; i < ia; i++) {
      B c = GetU(x,i);
      #if !CATCH_ERRORS
      if (c.u==0 || noFill(c)) { printf(" "); continue; }
      #endif
      if (!isC32(c)) thrM("bad printRaw argument: expected all character items");
      printUTF8((u32)c.u);
    }
  }
}
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
NOINLINE B do_fmt(B s, char* p, va_list a) {
  char buf[30];
  char c;
  char* lp = p;
  while (*p != 0) { c = *p++;
    if (c!='%') continue;
    if (lp!=p-1) AJOIN(fromUTF8(lp, p-1-lp));
    switch(c = *p++) { default: printf("Unknown format character '%c'", c); UD;
      case 'R': {
        B b = va_arg(a, B);
        if (isNum(b)) {
          AFMT("%f", o2f(b));
        } else { assert(isArr(b) && rnk(b)==1);
          if (TI(b,elType)==el_c32) AJOIN(inc(b));
          else {
            B sq = chr_squeezeChk(b);
            if (!elChr(TI(sq,elType))) FL_KEEP(sq, ~fl_squoze);
            AJOIN(inc(sq));
          }
        }
        break;
      }
      case 'H': {
        B o = va_arg(a, B);
        ur r = isArr(o)? rnk(o) : 0;
        usz* sh = isArr(o)? a(o)->sh : NULL;
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
  if (lp!=p) AJOIN(fromUTF8(lp, p-lp));
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
NOINLINE void thrF(char* p, ...) {
  va_list a;
  va_start(a, p);
  B r = do_fmt(emptyCVec(), p, a);
  va_end(a);
  thr(r);
}


#define CMP(W,X) ({ AUTO wt = (W); AUTO xt = (X); (wt>xt?1:0)-(wt<xt?1:0); })
NOINLINE i32 compareR(B w, B x) {
  if (isNum(w) & isC32(x)) return -1;
  if (isC32(w) & isNum(x)) return  1;
  if (isAtm(w) & isAtm(x)) thrM("Invalid comparison");
  bool wa=isAtm(w); usz wia; ur wr; usz* wsh; AS2B wgetU; Arr* wArr;
  bool xa=isAtm(x); usz xia; ur xr; usz* xsh; AS2B xgetU; Arr* xArr;
  if(wa) { wia=1; wr=0; wsh=NULL; } else { wia=a(w)->ia; wr=rnk(w); wsh=a(w)->sh; wgetU=TI(w,getU); wArr = a(w); }
  if(xa) { xia=1; xr=0; xsh=NULL; } else { xia=a(x)->ia; xr=rnk(x); xsh=a(x)->sh; xgetU=TI(x,getU); xArr = a(x); }
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

NOINLINE bool atomEqualR(B w, B x) {
  if (v(w)->type!=v(x)->type) return false;
  B2B dcf = TI(w,decompose);
  if (dcf == def_decompose) return false;
  B wd=dcf(inc(w)); B* wdp = harr_ptr(wd);
  B xd=dcf(inc(x)); B* xdp = harr_ptr(xd);
  if (o2i(wdp[0])<=1) { dec(wd);dec(xd); return false; }
  usz wia = a(wd)->ia;
  if (wia!=a(xd)->ia) { dec(wd);dec(xd); return false; }
  for (u64 i = 0; i<wia; i++) if(!equal(wdp[i], xdp[i]))
                      { dec(wd);dec(xd); return false; }
                        dec(wd);dec(xd); return true;
}
NOINLINE bool equal(B w, B x) { // doesn't consume
  bool wa = isAtm(w);
  bool xa = isAtm(x);
  if (wa!=xa) return false;
  if (wa) return atomEqual(w, x);
  if (!eqShape(w,x)) return false;
  usz ia = a(x)->ia;
  u8 we = TI(w,elType);
  u8 xe = TI(x,elType);
  if (((we==el_f64 | we==el_i32) && (xe==el_f64 | xe==el_i32))) {
    if (we==el_i32) { i32* wp = i32any_ptr(w);
      if(xe==el_i32) { i32* xp = i32any_ptr(x); for (usz i = 0; i < ia; i++) if(wp[i]!=xp[i]) return false; }
      else           { f64* xp = f64any_ptr(x); for (usz i = 0; i < ia; i++) if(wp[i]!=xp[i]) return false; }
    } else { f64* wp = f64any_ptr(w);
      if(xe==el_i32) { i32* xp = i32any_ptr(x); for (usz i = 0; i < ia; i++) if(wp[i]!=xp[i]) return false; }
      else           { f64* xp = f64any_ptr(x); for (usz i = 0; i < ia; i++) if(wp[i]!=xp[i]) return false; }
    }
    return true;
  }
  if (we==el_c32 && xe==el_c32) {
    u32* wp = c32any_ptr(w);
    u32* xp = c32any_ptr(x);
    for (usz i = 0; i < ia; i++) if(wp[i]!=xp[i]) return false;
    return true;
  }
  SGetU(x)
  SGetU(w)
  for (usz i = 0; i < ia; i++) if(!equal(GetU(w,i),GetU(x,i))) return false;
  return true;
}

bool atomEEqual(B w, B x) { // doesn't consume (not that that matters really currently)
  if (w.u==x.u) return true;
  if(isNum(w)|isNum(x)) return false;
  if (!isVal(w) | !isVal(x)) return false;
  if (v(w)->type!=v(x)->type) return false;
  B2B dcf = TI(w,decompose);
  if (dcf == def_decompose) return false;
  B wd=dcf(inc(w)); B* wdp = harr_ptr(wd);
  B xd=dcf(inc(x)); B* xdp = harr_ptr(xd);
  if (o2i(wdp[0])<=1) { dec(wd);dec(xd); return false; }
  usz wia = a(wd)->ia;
  if (wia!=a(xd)->ia) { dec(wd);dec(xd); return false; }
  for (u64 i = 0; i<wia; i++) if(!eequal(wdp[i], xdp[i]))
                      { dec(wd);dec(xd); return false; }
                        dec(wd);dec(xd); return true;
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
  usz ia = a(x)->ia;
  SGetU(x)
  SGetU(w)
  for (usz i = 0; i < ia; i++) if(!eequal(GetU(w,i),GetU(x,i))) return false;
  return true;
}

u64 depth(B x) { // doesn't consume
  if (isAtm(x)) return 0;
  if (TI(x,arrD1)) return 1;
  u64 r = 0;
  usz ia = a(x)->ia;
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
void slice_visit(Value* x) { mm_visitP(((Slice*)x)->p); }
void slice_print(B x) { arr_print(x); }

char* type_repr(u8 u) {
  switch(u) { default: return"(unknown type)";
    #define F(X) case t_##X: return #X;
    FOR_TYPE(F)
    #undef F
  }
}
bool isPureFn(B x) { // doesn't consume
  if (isCallable(x)) {
    if (isPrim(x)) return true;
    B2B dcf = TI(x,decompose);
    B xd = dcf(inc(x));
    B* xdp = harr_ptr(xd);
    i32 t = o2iu(xdp[0]);
    if (t<2) { dec(xd); return t==0; }
    usz xdia = a(xd)->ia;
    for (u64 i = 1; i<xdia; i++) if(!isPureFn(xdp[i])) { dec(xd); return false; }
    dec(xd); return true;
  } else if (isArr(x)) {
    usz ia = a(x)->ia;
    SGetU(x)
    for (usz i = 0; i < ia; i++) if (!isPureFn(GetU(x,i))) return false;
    return true;
  } else return isNum(x) || isC32(x);
}

B num_squeeze(B x) {
  usz ia = a(x)->ia;
  u8 xe = TI(x,elType);
  assert(xe!=el_bit);
  
  if (xe==el_i8) goto r_x;
  // TODO fast paths for xe<el_f64
  usz i = 0;
  i32 or = 0; // using bitwise or as a heuristical ⌈´|𝕩
  if (xe==el_f64) {
    f64* xp = f64any_ptr(x);
    for (; i < ia; i++) {
      f64 cf = xp[i];
      i32 c = (i32)cf;
      if (c!=cf) goto r_x; // already f64
      or|= c<0?-c:c;
    }
    goto r_or;
  }
  
  B* xp = arr_bptr(x);
  if (xp!=NULL) {
    for (; i < ia; i++) {
      if (RARE(!q_i32(xp[i]))) {
        while (i<ia) if (!isF64(xp[i++])) goto r_x;
        goto r_f64;
      }
      i32 c = o2iu(xp[i]);
      or|= c<0?-c:c;
    }
    goto r_or;
  }
  
  SGetU(x)
  for (; i < ia; i++) {
    B cr = GetU(x,i);
    if (RARE(!q_i32(cr))) {
      while (i<ia) if (!isF64(GetU(x,i++))) goto r_x;
      goto r_f64;
    }
    i32 c = o2iu(cr);
    or|= c<0?-c:c;
  }
  r_or:
  if      (or<=I8_MAX ) goto r_i8;
  else if (or<=I16_MAX) goto r_i16;
  else                  goto r_i32;
  
  r_x  : return FL_SET(x, fl_squoze);
  r_i8 : return FL_SET(toI8Any (x), fl_squoze);
  r_i16: return FL_SET(toI16Any(x), fl_squoze);
  r_i32: return FL_SET(toI32Any(x), fl_squoze);
  r_f64: return FL_SET(toF64Any(x), fl_squoze);
}
B chr_squeeze(B x) {
  usz ia = a(x)->ia;
  u8 xe = TI(x,elType);
  if (xe==el_c8) goto r_x;
  usz i = 0;
  i32 or = 0;
  if (xe==el_c16) {
    u16* xp = c16any_ptr(x);
    for (; i < ia; i++) if (xp[i] != (u8)xp[i]) goto r_x;
    goto r_c8;
  }
  if (xe==el_c32) {
    u32* xp = c32any_ptr(x);
    bool c8 = true;
    for (; i < ia; i++) {
      if (xp[i] != (u16)xp[i]) goto r_c16;
      if (xp[i] != (u8 )xp[i]) c8 = false;
    }
    if (c8) goto r_c8;
    else    goto r_c16;
  }
  
  B* xp = arr_bptr(x);
  if (xp!=NULL) {
    for (; i < ia; i++) {
      if (!isC32(xp[i])) goto r_x;
      or|= o2cu(xp[i]);
    }
  } else {
    SGetU(x)
    for (; i < ia; i++) {
      B cr = GetU(x,i);
      if (!isC32(cr)) goto r_x;
      or|= o2cu(cr);
    }
  }
  if      (or<=U8_MAX ) r_c8:  return FL_SET(toC8Any(x), fl_squoze);
  else if (or<=U16_MAX) r_c16: return FL_SET(toC16Any(x), fl_squoze);
  else                         return FL_SET(toC32Any(x), fl_squoze);
  r_x: return FL_SET(x, fl_squoze);
}

B any_squeeze(B x) {
  assert(isArr(x));
  if (FL_HAS(x,fl_squoze)) return x;
  if (a(x)->ia==0) return FL_SET(x, fl_squoze);
  SGetU(x)
  B x0 = GetU(x, 0);
  if (isNum(x0)) return num_squeeze(x);
  else if (isC32(x0)) return chr_squeeze(x);
  return FL_SET(x, fl_squoze);
}

B squeeze_deep(B x) {
  x = any_squeeze(x);
  if (TI(x,elType)!=el_B) return x;
  usz ia = a(x)->ia;
  usz i=0; HArr_p r = m_harrs(ia,&i);
  B* xp = arr_bptr(x);
  if (xp!=NULL) {
    while (i < ia) { r.a[i] = squeeze_deep(inc(xp[i])); i++; }
  } else {
    SGet(x);
    while (i < ia) { r.a[i] = squeeze_deep(Get(x,i)); i++; }
  }
  return any_squeeze(harr_fcd(r, x));
}

B bqn_merge(B x) {
  assert(isArr(x));
  usz xia = a(x)->ia;
  ur xr = rnk(x);
  if (xia==0) {
    B xf = getFillE(x);
    if (isAtm(xf)) { dec(xf); return x; }
    i32 xfr = rnk(xf);
    B xff = getFillQ(xf);
    Arr* r = m_fillarrp(0);
    fillarr_setFill(r, xff);
    if (xr+xfr > UR_MAX) thrM(">: Result rank too large");
    usz* rsh = arr_shAlloc(r, xr+xfr);
    if (rsh) {
      memcpy       (rsh   , a(x)->sh,  xr *sizeof(usz));
      if(xfr)memcpy(rsh+xr, a(xf)->sh, xfr*sizeof(usz));
    }
    dec(x); dec(xf);
    return taga(r);
  }
  
  SGetU(x)
  B x0 = GetU(x, 0);
  usz* elSh = isArr(x0)? a(x0)->sh : NULL;
  ur elR = isArr(x0)? rnk(x0) : 0;
  usz elIA = isArr(x0)? a(x0)->ia : 1;
  B fill = getFillQ(x0);
  if (xr+elR > UR_MAX) thrM(">: Result rank too large");
  
  MAKE_MUT(r, xia*elIA);
  usz rp = 0;
  for (usz i = 0; i < xia; i++) {
    B c = GetU(x, i);
    if (isArr(c)? (elR!=rnk(c) || !eqShPrefix(elSh, a(c)->sh, elR)) : elR!=0) { mut_pfree(r, rp); thrF(">: Elements didn't have equal shapes (contained %H and %H)", x0, c); }
    if (isArr(c)) mut_copy(r, rp, c, 0, elIA);
    else mut_set(r, rp, c);
    if (!noFill(fill)) fill = fill_or(fill, getFillQ(c));
    rp+= elIA;
  }
  Arr* ra = mut_fp(r);
  usz* rsh = arr_shAlloc(ra, xr+elR);
  if (rsh) {
    memcpy         (rsh   , a(x)->sh, xr *sizeof(usz));
    if (elSh)memcpy(rsh+xr, elSh,     elR*sizeof(usz));
  }
  dec(x);
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

NOINLINE void printAllocStats() {
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

#ifdef DEBUG
  #ifdef OBJ_COUNTER
    #define PRINT_ID(X) printf("Object ID: "N64u"\n", (X)->uid)
  #else
    #define PRINT_ID(X)
  #endif
  NOINLINE Value* VALIDATEP(Value* x) {
    if (x->refc<=0 || (x->refc>>28) == 'a' || x->type==t_empty) {
      PRINT_ID(x);
      printf("bad refcount for type %d: %d\nattempting to print: ", x->type, x->refc); fflush(stdout);
      print(tag(x,OBJ_TAG)); putchar('\n'); fflush(stdout);
      err("");
    }
    if (TIv(x,isArr)) {
      Arr* a = (Arr*)x;
      if (prnk(x)<=1) assert(a->sh == &a->ia);
      else VALIDATE(tag(shObjP(x),OBJ_TAG));
    }
    return x;
  }
  NOINLINE B VALIDATE(B x) {
    if (!isVal(x)) return x;
    VALIDATEP(v(x));
    if(isArr(x)!=TI(x,isArr) && v(x)->type!=t_freed && v(x)->type!=t_harrPartial) {
      printf("bad array tag/type: type=%d, obj=%p\n", v(x)->type, (void*)x.u);
      PRINT_ID(v(x));
      print(x);
      err("\nk");
    }
    return x;
  }
  NOINLINE NORETURN void assert_fail(char* expr, char* file, int line, const char fn[]) {
    printf("%s:%d: %s: Assertion `%s` failed.\n", file, line, fn, expr);
    err("");
  }
#endif
