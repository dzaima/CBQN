extern u64 allocB; // currently allocated number of bytes


// memory defs

static void* mm_alloc(usz sz, u8 type);
static void mm_free(Value* x);
static u64 mm_size(Value* x);
static void mm_visit(B x);
static void mm_visitP(void* x);
NORETURN void bqn_exit(i32 code);
u64 mm_heapUsed();
void printAllocStats();


#ifndef MAP_NORESERVE
 #define MAP_NORESERVE 0 // apparently needed for freebsd or something
#endif

// shape mess

typedef struct ShArr {
  struct Value;
  usz a[];
} ShArr;
static ShArr* shObj (B x) { return (ShArr*)((u64)a(x)->sh-offsetof(ShArr,a)); }
static ShArr* shObjP(Value* x) { return (ShArr*)((u64)((Arr*)x)->sh-offsetof(ShArr,a)); }
static void decSh(Value* x) { if (prnk(x)>1) ptr_dec(shObjP(x)); }

// some array stuff

#define WRAP(X,IA,MSG) ({ i64 wV=(i64)(X); u64 iaW=(IA); if(RARE((u64)wV >= iaW)) { if(wV<0) wV+= iaW; if((u64)wV >= iaW) {MSG;} }; (usz)wV; })

static ShArr* m_shArr(ur r) {
  assert(r>1);
  return ((ShArr*)mm_alloc(fsizeof(ShArr, a, usz, r), t_shape));
}

static void arr_shVec(B x, usz ia) {
  a(x)->ia = ia;
  srnk(x, 1);
  a(x)->sh = &a(x)->ia;
}
static usz* arr_shAllocR(B x, ur r) { // allocates shape, sets rank
  srnk(x,r);
  if (r>1) return a(x)->sh = m_shArr(r)->a;
  a(x)->sh = &a(x)->ia;
  return 0;
}
static usz* arr_shAllocI(B x, usz ia, ur r) { // allocates shape, sets ia,rank
  a(x)->ia = ia;
  return arr_shAllocR(x, r);
}
static void arr_shSetI(B x, usz ia, ur r, ShArr* sh) {
  srnk(x,r);
  a(x)->ia = ia;
  if (r>1) { a(x)->sh = sh->a; ptr_inc(sh); }
  else     { a(x)->sh = &a(x)->ia; }
}
static void arr_shCopy(B n, B o) { // copy shape,rank,ia from o to n
  assert(isArr(o));
  a(n)->ia = a(o)->ia;
  ur r = srnk(n,rnk(o));
  if (r<=1) {
    a(n)->sh = &a(n)->ia;
  } else {
    ptr_inc(shObj(o));
    a(n)->sh = a(o)->sh;
  }
}

static void arrP_shVec(Arr* x, usz ia) {
  x->ia = ia;
  sprnk(x, 1);
  x->sh = &x->ia;
}
static usz* arrP_shAllocR(Arr* x, ur r) { // allocates shape, sets rank
  sprnk(x,r);
  if (r>1) return x->sh = m_shArr(r)->a;
  x->sh = &x->ia;
  return 0;
}
static usz* arrP_shAllocI(Arr* x, usz ia, ur r) { // allocates shape, sets ia,rank
  x->ia = ia;
  return arrP_shAllocR(x, r);
}
static void arrP_shSetI(Arr* x, usz ia, ur r, ShArr* sh) {
  sprnk(x,r);
  x->ia = ia;
  if (r>1) { x->sh = sh->a; ptr_inc(sh); }
  else     { x->sh = &x->ia; }
}
static void arrP_shCopy(Arr* n, B o) { // copy shape,rank,ia from o to n
  assert(isArr(o));
  n->ia = a(o)->ia;
  ur r = sprnk(n,rnk(o));
  if (r<=1) {
    n->sh = &n->ia;
  } else {
    ptr_inc(shObj(o));
    n->sh = a(o)->sh;
  }
}

static usz arr_csz(B x) {
  ur xr = rnk(x);
  if (xr<=1) return 1;
  usz* sh = a(x)->sh;
  usz r = 1;
  for (i32 i = 1; i < xr; i++) r*= sh[i];
  return r;
}
static bool eqShPrefix(usz* w, usz* x, ur len) {
  return memcmp(w, x, len*sizeof(usz))==0;
}
static bool eqShape(B w, B x) { assert(isArr(w)); assert(isArr(x));
  ur wr = rnk(w); usz* wsh = a(w)->sh;
  ur xr = rnk(x); usz* xsh = a(x)->sh;
  if (wr!=xr) return false;
  if (wsh==xsh) return true;
  return eqShPrefix(wsh, xsh, wr);
}


static B m_v1(B a               ); // consumes all
static B m_v2(B a, B b          ); // consumes all
static B m_v3(B a, B b, B c     ); // consumes all
static B m_v4(B a, B b, B c, B d); // consumes all
static B vec_join(B w, B x);
static B vec_add(B w, B x);
static bool isNumEl(u8 elt) { return elt==el_i32 | elt==el_f64; }

// string stuff

B m_str8l(char* s);
B fromUTF8l(char* x);
#define AJOIN(X) s = vec_join(s,X) // consumes X
#define AOBJ(X) s = vec_add(s,X) // consumes X
#define ACHR(X) AOBJ(m_c32(X))
#define A8(X) AJOIN(m_str8l(X))
#define AU(X) AJOIN(fromUTF8l(X))
#define AFMT(...) s = append_fmt(s, __VA_ARGS__)
NOINLINE B append_fmt(B s, char* p, ...);

// function stuff

char* format_pf(u8 u);
char* format_pm1(u8 u);
char* format_pm2(u8 u);
bool isPureFn(B x); // doesn't consume
B bqn_merge(B x); // consumes
B bqn_squeeze(B x); // consumes
static void noop_visit(Value* x) { }
static B def_getU(B x, usz n) { return x; }

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
static B def_fn_uc1(B t, B o,                B x) { return rtUnder_c1(o, t,    x); }
static B def_fn_ucw(B t, B o,           B w, B x) { return rtUnder_cw(o, t, w, x); }
static B def_m1_uc1(B t, B o, B f,           B x) { B t2 = m1_d(inc(t),inc(f)       ); B r = rtUnder_c1(o, t2,    x); dec(t2); return r; }
static B def_m1_ucw(B t, B o, B f,      B w, B x) { B t2 = m1_d(inc(t),inc(f)       ); B r = rtUnder_cw(o, t2, w, x); dec(t2); return r; }
static B def_m2_uc1(B t, B o, B f, B g,      B x) { B t2 = m2_d(inc(t),inc(f),inc(g)); B r = rtUnder_c1(o, t2,    x); dec(t2); return r; }
static B def_m2_ucw(B t, B o, B f, B g, B w, B x) { B t2 = m2_d(inc(t),inc(f),inc(g)); B r = rtUnder_cw(o, t2, w, x); dec(t2); return r; }
static B def_decompose(B x) { return m_v2(m_i32(isCallable(x)? 0 : -1),x); }

#define CMP(W,X) ({ AUTO wt = (W); AUTO xt = (X); (wt>xt?1:0)-(wt<xt?1:0); })
NOINLINE i32 compareR(B w, B x);
static i32 compare(B w, B x) { // doesn't consume; -1 if w<x, 1 if w>x, 0 if w≡x; 0==compare(NaN,NaN)
  if (isNum(w) & isNum(x)) return CMP(o2fu(w), o2fu(x));
  if (isC32(w) & isC32(x)) return CMP(o2cu(w), o2cu(x));
  return compareR(w, x);
}
#undef CMP

NOINLINE bool atomEqualR(B w, B x);
static bool atomEqual(B w, B x) { // doesn't consume (not that that matters really currently)
  if(isF64(w)&isF64(x)) return w.f==x.f;
  if (w.u==x.u) return true;
  if (!isVal(w) | !isVal(x)) return false;
  return atomEqualR(w, x);
}



#ifdef DEBUG
  static NOINLINE Value* VALIDATEP(Value* x) {
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
  static NOINLINE B VALIDATE(B x) {
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
  static void pst(char* msg) {
    VALGRIND_PRINTF_BACKTRACE("%s", msg);
  }
#endif

// call stuff

static NOINLINE B c1_invalid(B f,      B x) { thrM("This function can't be called monadically"); }
static NOINLINE B c2_invalid(B f, B w, B x) { thrM("This function can't be called dyadically"); }
static B md_c1(B t,      B x) { thrM("Cannot call a modifier"); }
static B md_c2(B t, B w, B x) { thrM("Cannot call a modifier"); }
static B arr_c1(B t,      B x) { return inc(t); }
static B arr_c2(B t, B w, B x) { return inc(t); }
static BB2B c1fn(B f) {
  if (isFun(f)) return c(Fun,f)->c1;
  if (isMd(f)) return md_c1;
  return arr_c1;
}
static BBB2B c2fn(B f) {
  if (isFun(f)) return c(Fun,f)->c2;
  if (isMd(f)) return md_c2;
  return arr_c2;
}

// alloc stuff

#ifdef ALLOC_STAT
  extern u64* ctr_a;
  extern u64* ctr_f;
  extern u64 actrc;
  extern u64 talloc;
  #ifdef ALLOC_SIZES
    extern u32** actrs;
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


extern _Thread_local i64 comp_currEnvPos;
extern _Thread_local B comp_currPath;
extern _Thread_local B comp_currArgs;
extern _Thread_local B comp_currSrc;
