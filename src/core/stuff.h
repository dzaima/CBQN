// memory defs

static void* mm_alloc(usz sz, u8 type);
static void mm_free(Value* x);
static u64 mm_size(Value* x);
static void mm_visit(B x);
static void mm_visitP(void* x);
NORETURN void bqn_exit(i32 code);
u64 mm_heapUsed();
void printAllocStats();
void vm_pstLive();


#ifndef MAP_NORESERVE
 #define MAP_NORESERVE 0 // apparently needed for freebsd or something
#endif

// shape mess

typedef struct ShArr {
  struct Value;
  usz a[];
} ShArr;
static ShArr* shObj (B x) { return RFLD(a(x)->sh, ShArr, a); }
static ShArr* shObjP(Value* x) { return RFLD(((Arr*)x)->sh, ShArr, a); }
void decShR(Value* x);
static void decSh(Value* x) { if (RARE(prnk(x)>1)) decShR(x); }

// some array stuff

#define WRAP(X,IA,MSG) ({ i64 wV=(i64)(X); u64 iaW=(IA); if(RARE((u64)wV >= iaW)) { if(wV<0) wV+= iaW; if((u64)wV >= iaW) {MSG;} }; (usz)wV; })

static ShArr* m_shArr(ur r) {
  assert(r>1);
  return ((ShArr*)mm_alloc(fsizeof(ShArr, a, usz, r), t_shape));
}

static void arr_shVec(Arr* x, usz ia) {
  x->ia = ia;
  sprnk(x, 1);
  x->sh = &x->ia;
}
static usz* arr_shAllocR(Arr* x, ur r) { // allocates shape, sets rank
  sprnk(x,r);
  if (r>1) return x->sh = m_shArr(r)->a;
  x->sh = &x->ia;
  return 0;
}
static usz* arr_shAllocI(Arr* x, usz ia, ur r) { // allocates shape, sets ia,rank
  x->ia = ia;
  return arr_shAllocR(x, r);
}
static void arr_shSetI(Arr* x, usz ia, ur r, ShArr* sh) {
  sprnk(x,r);
  x->ia = ia;
  if (r>1) { x->sh = sh->a; ptr_inc(sh); }
  else     { x->sh = &x->ia; }
}
static void arr_shCopy(Arr* n, B o) { // copy shape,rank,ia from o to n
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
B append_fmt(B s, char* p, ...);
B make_fmt(char* p, ...);
#define AJOIN(X) s = vec_join(s,X) // consumes X
#define AOBJ(X) s = vec_add(s,X) // consumes X
#define ACHR(X) AOBJ(m_c32(X))
#define A8(X) AJOIN(m_str8l(X))
#define AU(X) AJOIN(fromUTF8l(X))
#define AFMT(...) s = append_fmt(s, __VA_ARGS__)

// function stuff

char* format_type(u8 u);
char* format_pf(u8 u);
char* format_pm1(u8 u);
char* format_pm2(u8 u);
bool isPureFn(B x); // doesn't consume
B bqn_merge(B x); // consumes
B bqn_squeeze(B x); // consumes
B def_getU(B x, usz n);
B def_fn_uc1(B t, B o,                B x);
B def_fn_ucw(B t, B o,           B w, B x);
B def_m1_uc1(B t, B o, B f,           B x);
B def_m1_ucw(B t, B o, B f,      B w, B x);
B def_m2_uc1(B t, B o, B f, B g,      B x);
B def_m2_ucw(B t, B o, B f, B g, B w, B x);
B def_decompose(B x);
void noop_visit(Value* x);

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




#ifdef USE_VALGRIND
  #include <valgrind/valgrind.h>
  #include <valgrind/memcheck.h>
  static void pst(char* msg) {
    VALGRIND_PRINTF_BACKTRACE("%s", msg);
  }
#endif

// call stuff

NORETURN B c1_invalid(B f,      B x);
NORETURN B c2_invalid(B f, B w, B x);
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
