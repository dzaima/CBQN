#include "h.h"
// a bunch of random things that don't really belong in any other file


#include <sys/mman.h>
#include <unistd.h>

#ifndef MAP_NORESERVE
 #define MAP_NORESERVE 0 // apparently needed for freebsd or something
#endif


void empty_free(Value* x) { err("FREEING EMPTY\n"); }
void builtin_free(Value* x) { err("FREEING BUILTIN\n"); }
void def_free(Value* x) { }
void def_visit(Value* x) { printf("(no visit for %d=%s)\n", x->type, format_type(x->type)); }
void noop_visit(Value* x) { }
void freed_visit(Value* x) {
  #ifndef CATCH_ERRORS
  err("visiting t_freed\n");
  #endif
}
void def_print(B x) { printf("(%d=%s)", v(x)->type, format_type(v(x)->type)); }
B    def_identity(B f) { return bi_N; }
B    def_get (B x, usz n) { return inc(x); }
B    def_getU(B x, usz n) { return x; }
B    def_m1_d(B m, B f     ) { thrM("cannot derive this"); }
B    def_m2_d(B m, B f, B g) { thrM("cannot derive this"); }
B    def_slice(B x, usz s) { thrM("cannot slice non-array!"); }
bool def_canStore(B x) { return false; }

B m_c32arrv(u32** p, usz ia);
B m_str8l(char* s);
B m_str32c(u32 c) {
  u32* rp; B r = m_c32arrv(&rp, 1);
  rp[0] = c;
  return r;
}
B fromUTF8(char* x, i64 len);
B fromUTF8l(char* x);
#define A8(X) s = vec_join(s,m_str8l(X))
#define AU(X) s = vec_join(s,fromUTF8l(X))
#define AC(X) s = vec_join(s,m_str32c(X))
#define AFMT(...) s = append_fmt(s, __VA_ARGS__)
NOINLINE B append_fmt(B s, char* p, ...) {
  va_list a;
  va_start(a, p);
  char buf[30];
  char c;
  char* lp = p;
  while ((c = *p++) != 0) {
    if (c!='%') continue;
    if (lp!=p-1) s = vec_join(s, fromUTF8(lp, p-1-lp));
    switch(c = *p++) { default: printf("Unknown format character '%c'", c); UD;
      case 'R': { // TODO proper
        B b = va_arg(a, B);
        if (isNum(b)) {
          AFMT("%f", o2f(b));
        } else { assert(isArr(b) && rnk(b)==1);
          s = vec_join(s, inc(b));
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
          if (c=='i') snprintf(buf, 30, mode==1?  "%u" :  "%x", va_arg(a, u32));
          else        snprintf(buf, 30, mode==1? "%lu" : "%lx", va_arg(a, u64));
        } else {
          if (c=='i') {
            i32 v = va_arg(a, i32);
            if (v<0) AU("¯");
            snprintf(buf, 30, "%lu", (u64)(v<0?-v:v));
          } else { assert(c=='l');
            i64 v = va_arg(a, i64);
            if (v<0) AU("¯");
            if (v==I64_MIN) snprintf(buf, 30, "9223372036854775808");
            else snprintf(buf, 30, "%lu", (u64)(v<0?-v:v));
          }
        }
        A8(buf);
        break;
      }
      case 's': {
        usz v = va_arg(a, usz);
        snprintf(buf, 30, sizeof(usz)==4? "%u" : "%lu", v);
        A8(buf);
        break;
      }
      case 'p': {
        snprintf(buf, 30, "%p", va_arg(a, void*));
        A8(buf);
        break;
      }
      case 'f': {
        f64 f = va_arg(a, f64);
        if (f<0) {
          AU("¯");
          f=-f;
        }
        snprintf(buf, 30, "%g", f);
        A8(buf);
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
  if (lp!=p) s = vec_join(s, fromUTF8(lp, p-lp));
  va_end(a);
  return s;
}

B rt_under, bi_before;
B rtUnder_c1(B f, B g, B x) { // consumes x
  B fn = m2_d(inc(rt_under), inc(f), inc(g));
  B r = c1(fn, x);
  dec(fn);
  return r;
}
B rtUnder_cw(B f, B g, B w, B x) { // consumes w,x
  B fn = m2_d(inc(rt_under), inc(f), m2_d(inc(bi_before), w, inc(g)));
  B r = c1(fn, x);
  dec(fn);
  return r;
}


B bi_before;
B def_fn_uc1(B t, B o,                B x) { return rtUnder_c1(o, t,    x); }
B def_fn_ucw(B t, B o,           B w, B x) { return rtUnder_cw(o, t, w, x); }
B def_m1_uc1(B t, B o, B f,           B x) { B t2 = m1_d(inc(t),inc(f)       ); B r = rtUnder_c1(o, t2,    x); dec(t2); return r; }
B def_m1_ucw(B t, B o, B f,      B w, B x) { B t2 = m1_d(inc(t),inc(f)       ); B r = rtUnder_cw(o, t2, w, x); dec(t2); return r; }
B def_m2_uc1(B t, B o, B f, B g,      B x) { B t2 = m2_d(inc(t),inc(f),inc(g)); B r = rtUnder_c1(o, t2,    x); dec(t2); return r; }
B def_m2_ucw(B t, B o, B f, B g, B w, B x) { B t2 = m2_d(inc(t),inc(f),inc(g)); B r = rtUnder_cw(o, t2, w, x); dec(t2); return r; }


void arr_print(B x) { // should accept refc=0 arguments for debugging purposes
  ur r = rnk(x);
  BS2B xgetU = TI(x).getU;
  usz ia = a(x)->ia;
  if (r!=1) {
    if (r==0) {
      printf("<");
      print(xgetU(x,0));
      return;
    }
    usz* sh = a(x)->sh;
    for (i32 i = 0; i < r; i++) {
      if(i==0)printf("%d",sh[i]);
      else printf("‿%d",sh[i]);
    }
    printf("⥊");
  } else if (ia>0) {
    for (usz i = 0; i < ia; i++) {
      B c = xgetU(x,i);
      if (!isC32(c) || (u32)c.u=='\n') goto reg;
    }
    printf("\"");
    for (usz i = 0; i < ia; i++) printUTF8((u32)xgetU(x,i).u); // c32, no need to decrement
    printf("\"");
    return;
  }
  reg:;
  printf("⟨");
  for (usz i = 0; i < ia; i++) {
    if (i!=0) printf(", ");
    print(xgetU(x,i));
  }
  printf("⟩");
}

void print(B x) {
  if (isF64(x)) {
    printf("%g", x.f);
  } else if (isC32(x)) {
    if ((u32)x.u>=32) { printf("'"); printUTF8((u32)x.u); printf("'"); }
    else if((u32)x.u>15) printf("\\x%x", (u32)x.u);
    else printf("\\x0%x", (u32)x.u);
  } else if (isI32(x)) {
    printf("%d", (i32)x.u);
  } else if (isVal(x)) {
    #ifdef DEBUG
    if (isVal(x) && (v(x)->type==t_freed || v(x)->type==t_empty)) {
      u8 t = v(x)->type;
      v(x)->type = v(x)->flags;
      printf(t==t_freed?"FREED:":"EMPTY:");
      TI(x).print(x);
      v(x)->type = t;
      return;
    }
    #endif
    TI(x).print(x);
  }
  else if (isVar(x)) printf("(var d=%d i=%d)", (u16)(x.u>>32), (i32)x.u);
  else if (x.u==bi_N.u) printf("·");
  else if (x.u==bi_optOut.u) printf("(value optimized out)");
  else if (x.u==bi_noVar.u) printf("(unset variable placeholder)");
  else if (x.u==bi_badHdr.u) printf("(bad header note)");
  else if (x.u==bi_noFill.u) printf("(no fill placeholder)");
  else printf("(todo tag %lx)", x.u>>48);
}

void printRaw(B x) {
  if (isAtm(x)) {
    if (isF64(x)) printf("%g", x.f);
    else if (isC32(x)) printUTF8((u32)x.u);
    else thrM("bad printRaw argument: atom arguments should be either numerical or characters");
  } else {
    usz ia = a(x)->ia;
    BS2B xgetU = TI(x).getU;
    for (usz i = 0; i < ia; i++) {
      B c = xgetU(x,i);
      #ifndef CATCH_ERRORS
      if (c.u==0 || noFill(c)) { printf(" "); continue; }
      #endif
      if (!isC32(c)) thrM("bad printRaw argument: expected all character items");
      printUTF8((u32)c.u);
    }
  }
}

B def_decompose(B x) { return m_v2(m_i32(isCallable(x)? 0 : -1),x); }
bool atomEqual(B w, B x) { // doesn't consume (not that that matters really currently)
  if(isF64(w)&isF64(x)) return w.f==x.f;
  if (w.u==x.u) return true;
  if (!isVal(w) | !isVal(x)) return false;
  if (v(w)->type!=v(x)->type) return false;
  B2B dcf = TI(w).decompose;
  if (dcf == def_decompose) return false;
  B wd=dcf(inc(w)); B* wdp = harr_ptr(wd);
  B xd=dcf(inc(x)); B* xdp = harr_ptr(xd);
  if (o2i(wdp[0])<=1) { dec(wd);dec(xd); return false; }
  usz wia = a(wd)->ia;
  if (wia!=a(xd)->ia) { dec(wd);dec(xd); return false; }
  for (i32 i = 0; i<wia; i++) if(!equal(wdp[i], xdp[i]))
                      { dec(wd);dec(xd); return false; }
                        dec(wd);dec(xd); return true;
}
bool equal(B w, B x) { // doesn't consume
  bool wa = isAtm(w);
  bool xa = isAtm(x);
  if (wa!=xa) return false;
  if (wa) return atomEqual(w, x);
  if (!eqShape(w,x)) return false;
  usz ia = a(x)->ia;
  BS2B xgetU = TI(x).getU;
  BS2B wgetU = TI(w).getU;
  for (usz i = 0; i < ia; i++) if(!equal(wgetU(w,i),xgetU(x,i))) return false;
  return true;
}

#define CMP(W,X) ({ AUTO wt = (W); AUTO xt = (X); (wt>xt?1:0)-(wt<xt?1:0); })
i32 compare(B w, B x) {
  if (isNum(w) & isNum(x)) return CMP(o2fu(w), o2fu(x));
  if (isC32(w) & isC32(x)) return CMP(o2cu(w), o2cu(x));
  if (isNum(w) & isC32(x)) return -1;
  if (isC32(w) & isNum(x)) return  1;
  if (isAtm(w) & isAtm(x)) thrM("Invalid comparison");
  bool wa=isAtm(w); usz wia; ur wr; usz* wsh; BS2B wgetU;
  bool xa=isAtm(x); usz xia; ur xr; usz* xsh; BS2B xgetU;
  if(wa) { wia=1; wr=0; wsh=NULL; wgetU=def_getU; } else { wia=a(w)->ia; wr=rnk(w); wsh=a(w)->sh; wgetU=TI(w).getU; }
  if(xa) { xia=1; xr=0; xsh=NULL; xgetU=def_getU; } else { xia=a(x)->ia; xr=rnk(x); xsh=a(x)->sh; xgetU=TI(x).getU; }
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
  for (int i = 0; i < rm; i++) {
    int c = compare(wgetU(w,i), xgetU(x,i));
    if (c!=0) return c;
  }
  return rc;
}
#undef CMP




bool eqShPrefix(usz* w, usz* x, ur len) {
  return memcmp(w, x, len*sizeof(usz))==0;
}
bool eqShape(B w, B x) { assert(isArr(w)); assert(isArr(x));
  ur wr = rnk(w); usz* wsh = a(w)->sh;
  ur xr = rnk(x); usz* xsh = a(x)->sh;
  if (wr!=xr) return false;
  if (wsh==xsh) return true;
  return eqShPrefix(wsh, xsh, wr);
}

usz arr_csz(B x) {
  ur xr = rnk(x);
  if (xr<=1) return 1;
  usz* sh = a(x)->sh;
  usz r = 1;
  for (i32 i = 1; i < xr; i++) r*= sh[i];
  return r;
}

u8 fillElType(B x) {
  if (isNum(x)) return el_i32;
  if (isC32(x)) return el_c32;
  return el_B;
}
u8 selfElType(B x) {
  if (isF64(x)) return q_i32(x)? el_i32 : el_f64;
  if (isC32(x)) return el_c32;
  return el_B;
}

bool isNumEl(u8 elt) { return elt==el_i32 | elt==el_f64; }



#ifdef DEBUG
  Value* VALIDATEP(Value* x) {
    if (x->refc<=0 || (x->refc>>28) == 'a' || x->type==t_empty) {
      printf("bad refcount for type %d: %d\nattempting to print: ", x->type, x->refc); fflush(stdout);
      print(tag(x,OBJ_TAG)); putchar('\n'); fflush(stdout);
      err("");
    }
    if (ti[x->type].isArr) {
      Arr* a = (Arr*)x;
      if (prnk(x)<=1) assert(a->sh == &a->ia);
      else VALIDATE(tag(shObjP(x),OBJ_TAG));
    }
    return x;
  }
  B VALIDATE(B x) {
    if (!isVal(x)) return x;
    VALIDATEP(v(x));
    if(isArr(x)!=TI(x).isArr && v(x)->type!=t_freed && v(x)->type!=t_harrPartial) {
      printf("bad array tag/type: type=%d, obj=%p\n", v(x)->type, (void*)x.u);
      print(x);
      err("\nk");
    }
    return x;
  }
#endif

#ifdef USE_VALGRIND
  #include <valgrind/valgrind.h>
  #include <valgrind/memcheck.h>
  void pst(char* msg) {
    VALGRIND_PRINTF_BACKTRACE("%s", msg);
  }
#endif

static inline void hdr_init() {
  for (i32 i = 0; i < t_COUNT; i++) {
    ti[i].free  = def_free;
    ti[i].visit = def_visit;
    ti[i].get   = def_get;
    ti[i].getU  = def_getU;
    ti[i].print = def_print;
    ti[i].m1_d  = def_m1_d;
    ti[i].m2_d  = def_m2_d;
    ti[i].isArr = false;
    ti[i].arrD1 = false;
    ti[i].elType    = el_B;
    ti[i].identity  = def_identity;
    ti[i].decompose = def_decompose;
    ti[i].slice     = def_slice;
    ti[i].canStore  = def_canStore;
    ti[i].fn_uc1 = def_fn_uc1;
    ti[i].fn_ucw = def_fn_ucw;
    ti[i].m1_uc1 = def_m1_uc1;
    ti[i].m1_ucw = def_m1_ucw;
    ti[i].m2_uc1 = def_m2_uc1;
    ti[i].m2_ucw = def_m2_ucw;
  }
  ti[t_empty].free = empty_free;
  ti[t_freed].free = def_free;
  ti[t_freed].visit = freed_visit;
  ti[t_shape].visit = noop_visit;
  ti[t_funBI].visit = ti[t_md1BI].visit = ti[t_md2BI].visit = noop_visit;
  ti[t_funBI].free  = ti[t_md1BI].free  = ti[t_md2BI].free  = builtin_free;
  bi_N = tag(0, TAG_TAG);
  bi_noVar   = tag(1, TAG_TAG);
  bi_badHdr  = tag(2, TAG_TAG);
  bi_optOut  = tag(3, TAG_TAG);
  bi_noFill  = tag(5, TAG_TAG);
  assert((MD1_TAG>>1) == (MD2_TAG>>1)); // just to be sure it isn't changed incorrectly, `isMd` depends on this
}

static NOINLINE B c1_invalid(B f,      B x) { thrM("This function can't be called monadically"); }
static NOINLINE B c2_invalid(B f, B w, B x) { thrM("This function can't be called dyadically"); }
static B md_c1(B t,      B x) { thrM("Cannot call a modifier"); }
static B md_c2(B t, B w, B x) { thrM("Cannot call a modifier"); }
static B arr_c1(B t,      B x) { return inc(t); }
static B arr_c2(B t, B w, B x) { return inc(t); }
BB2B c1fn(B f) {
  if (isFun(f)) return c(Fun,f)->c1;
  if (isMd(f)) return md_c1;
  return arr_c1;
}
BBB2B c2fn(B f) {
  if (isFun(f)) return c(Fun,f)->c2;
  if (isMd(f)) return md_c2;
  return arr_c2;
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

static inline void onAlloc(usz sz, u8 type) {
  #ifdef ALLOC_STAT
    if (!ctr_a) {
      #ifdef ALLOC_SIZES
        actrs = malloc(sizeof(u32*)*actrc);
        for (i32 i = 0; i < actrc; i++) actrs[i] = calloc(t_COUNT, sizeof(u32));
      #endif
      ctr_a = calloc(t_COUNT, sizeof(u64));
      ctr_f = calloc(t_COUNT, sizeof(u64));
    }
    assert(type<t_COUNT);
    #ifdef ALLOC_SIZES
      actrs[(sz+3)/4>=actrc? actrc-1 : (sz+3)/4][type]++;
    #endif
    ctr_a[type]++;
    talloc+= sz;
  #endif
}
static inline void onFree(Value* x) {
  #ifdef ALLOC_STAT
    ctr_f[x->type]++;
  #endif
  #ifdef DEBUG
    if (x->type==t_empty) err("double-free");
    // u32 undef;
    // x->refc = undef;
    x->refc = -1431655000;
  #endif
  // x->refc = 0x61616161;
}


void printAllocStats() {
  #ifdef ALLOC_STAT
    printf("total ever allocated: %lu\n", talloc);
    printf("allocated heap size:  %ld\n", mm_heapAllocated());
    printf("used heap size:       %ld\n", mm_heapUsed());
    ctr_a[t_harr]+= ctr_a[t_harrPartial];
    ctr_a[t_harrPartial] = 0;
    printf("ctrA←"); for (i64 i = 0; i < t_COUNT; i++) { if(i)printf("‿"); printf("%lu", ctr_a[i]); } printf("\n");
    printf("ctrF←"); for (i64 i = 0; i < t_COUNT; i++) { if(i)printf("‿"); printf("%lu", ctr_f[i]); } printf("\n");
    printf("names←⟨"); for (i64 i = 0; i < t_COUNT; i++) { if(i)printf(","); printf("\"%s\"", format_type(i)); } printf("⟩\n");
    u64 leakedCount = 0;
    for (i64 i = 0; i < t_COUNT; i++) leakedCount+= ctr_a[i]-ctr_f[i];
    printf("leaked object count: %ld\n", leakedCount);
    #ifdef ALLOC_SIZES
      for(i64 i = 0; i < actrc; i++) {
        u32* c = actrs[i];
        bool any = false;
        for (i64 j = 0; j < t_COUNT; j++) if (c[j]) any=true;
        if (any) {
          printf("%ld", i*4);
          for (i64 k = 0; k < t_COUNT; k++) printf("‿%u", c[k]);
          printf("\n");
        }
      }
    #endif
  #endif
}

_Thread_local B comp_currPath;
_Thread_local B comp_currArgs;
#define FOR_INIT(F) F(hdr) F(harr) F(fillarr) F(i32arr) F(c32arr) F(f64arr) F(hash) F(fns) F(sfns) F(arith) F(grade) F(md1) F(md2) F(sysfn) F(derv) F(comp) F(rtPerf) F(ns) F(load)
#define F(X) static inline void X##_init();
FOR_INIT(F)
#undef F
void cbqn_init() {
  #define F(X) X##_init();
   FOR_INIT(F)
  #undef F
}
#undef FOR_INIT
