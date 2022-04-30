// memory defs

NORETURN void bqn_exit(i32 code);
u64 mm_heapUsed(void);
void print_allocStats(void);
void vm_pstLive(void);


#ifndef MAP_NORESERVE
 #define MAP_NORESERVE 0 // apparently needed for freebsd or something
#endif

typedef struct CustomObj {
  struct Value;
  V2v visit;
  V2v freeO;
} CustomObj;
void* customObj(u64 size, V2v visit, V2v freeO);

// shape mess

typedef struct ShArr {
  struct Value;
  usz a[];
} ShArr;
static ShArr* shObjS(usz* x) { return RFLD(x, ShArr, a); }
static ShArr* shObj (B x) { return RFLD(a(x)->sh, ShArr, a); }
static ShArr* shObjP(Value* x) { return RFLD(((Arr*)x)->sh, ShArr, a); }
static void decSh(Value* x) { if (RARE(prnk(x)>1)) tptr_dec(shObjP(x), mm_free); }

// some array stuff

#define TYARR_SZ(T,IA) fsizeof(TyArr, a, T##Atom, IA)
#define TYARR_SZ2(T,IA) TYARR_SZ(T,IA)
#define TYARR_SZW(W,IA) (offsetof(TyArr, a) + (W)*(IA))

#define WRAP(X,IA,MSG) ({ i64 wV=(i64)(X); u64 iaW=(IA); if(RARE((u64)wV >= iaW)) { if(wV<0) wV+= iaW; if((u64)wV >= iaW) {MSG;} }; (usz)wV; })

static inline void* m_arr(u64 sz, u8 type, usz ia) {
  Arr* r = mm_alloc(sz, type);
  r->ia = ia;
  return r;
}
static ShArr* m_shArr(ur r) {
  assert(r>1);
  return ((ShArr*)mm_alloc(fsizeof(ShArr, a, usz, r), t_shape));
}

static void arr_shVec(Arr* x) {
  sprnk(x, 1);
  x->sh = &x->ia;
}
static usz* arr_shAlloc(Arr* x, ur r) { // sets rank, allocates & returns shape (or null if r<2)
  sprnk(x,r);
  if (r>1) return x->sh = m_shArr(r)->a;
  x->sh = &x->ia;
  return NULL;
}
static void arr_shSetI(Arr* x, ur r, ShArr* sh) { // set rank and assign and increment shape if needed
  sprnk(x,r);
  if (r>1) { x->sh = ptr_inc(sh)->a; }
  else     { x->sh = &x->ia; }
}
static void arr_shSetU(Arr* x, ur r, ShArr* sh) { // set rank and assign shape
  sprnk(x,r);
  if (r>1) { x->sh = sh->a;  }
  else     { x->sh = &x->ia; }
}
static void arr_shCopy(Arr* n, B o) { // copy shape & rank from o to n
  assert(isArr(o));
  assert(a(o)->ia==n->ia);
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

B bit_sel(B b, B e0, bool h0, B e1, bool h1); // consumes b; h0/h1 represent whether the corresponding element _might_ be in the result (can be true if unknown)
Arr* allZeroes(usz ia);
Arr* allOnes(usz ia);
B bit_negate(B x); // consumes

static B m_hVec1(B a               ); // consumes all
static B m_hVec2(B a, B b          ); // consumes all
static B m_hVec3(B a, B b, B c     ); // consumes all
static B m_hVec4(B a, B b, B c, B d); // consumes all
B m_vec1(B a);      // complete fills
B m_vec2(B a, B b); // incomplete fills

// random stuff

#define addOn(V,X) ({ AUTO v_ = &(V); __builtin_add_overflow(*v_, X, v_); })
#define mulOn(V,X) ({ AUTO v_ = &(V); __builtin_mul_overflow(*v_, X, v_); })

static usz uszMul(usz a, usz b) {
  if (mulOn(a, b)) thrM("Size too large");
  return a;
}

static u8 selfElType(B x) { // guaranteed to fit fill
  if (isF64(x)) return q_i8(x)? (q_bit(x)? el_bit : el_i8) : (q_i16(x)? el_i16 : q_i32(x)? el_i32 : el_f64);
  if (isC32(x)) return LIKELY(q_c8(x))? el_c8 : q_c16(x)? el_c16 : el_c32;
  return el_B;
}
static bool elChr(u8 x) {
  return x>=el_c8 && x<=el_c32;
}
static bool elNum(u8 x) {
  return x<=el_f64;
}

// string stuff

i32 num_fmt(char buf[30], f64 x);
#define NUM_FMT_BUF(N,X) char N[30]; num_fmt(N, X);
B append_fmt(B s, char* p, ...);
B make_fmt(char* p, ...);
void print_fmt(char* p, ...);
#define AJOIN(X) s = vec_join(s,X) // consumes X
#define AOBJ(X) s = vec_add(s,X) // consumes X
#define ACHR(X) AOBJ(m_c32(X))
#define A8(X) AJOIN(m_str8l(X))
#define AU(X) AJOIN(fromUTF8l(X))
#define AFMT(...) s = append_fmt(s, __VA_ARGS__)

// function stuff

char* type_repr(u8 u);
char* pfn_repr(u8 u);
char* pm1_repr(u8 u);
char* pm2_repr(u8 u);
char* eltype_repr(u8 u);
bool isPureFn(B x); // doesn't consume
B bqn_merge(B x); // consumes

B any_squeeze(B x); // consumes; accepts any array, returns one with the smallest type (doesn't recurse!)
B squeeze_deep(B x); // consumes; accepts any object, returns an object with all parts necessary for equality checking & hashing squeezed
B num_squeeze(B x); // consumes; see note below
B chr_squeeze(B x); // consumes; see note below
// Note that num_squeeze & chr_squeeze don't check for fl_squoze, and unconditionally set it. Thus, don't call it on an array if it could be squeezable by the opposite method.
// or, if you do want to, if TI(x,elType) isn't of the squeezed type, either remove fl_squoze or call the other squeeze function.
// The functions below can be used as direct replacements of (num|chr)_squeeze if the argument might already be squeezed.
static inline B num_squeezeChk(B x) { return FL_HAS(x,fl_squoze)? x : num_squeeze(x); }
static inline B chr_squeezeChk(B x) { return FL_HAS(x,fl_squoze)? x : chr_squeeze(x); }

B def_fn_uc1(B t, B o,                B x);
B def_fn_ucw(B t, B o,           B w, B x);
B def_m1_uc1(Md1* t, B o, B f,           B x);
B def_m1_ucw(Md1* t, B o, B f,      B w, B x);
B def_m2_uc1(Md2* t, B o, B f, B g,      B x);
B def_m2_ucw(Md2* t, B o, B f, B g, B w, B x);
B def_fn_im(B t,      B x);
B def_fn_is(B t,      B x);
B def_fn_iw(B t, B w, B x);
B def_fn_ix(B t, B w, B x);

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

NORETURN B c1_bad(B f,      B x);
NORETURN B c2_bad(B f, B w, B x);
NORETURN B m1c1_bad(Md1D* d,      B x);
NORETURN B m1c2_bad(Md1D* d, B w, B x);
NORETURN B m2c1_bad(Md2D* d,      B x);
NORETURN B m2c2_bad(Md2D* d, B w, B x);

B md_c1(B t,      B x);
B md_c2(B t, B w, B x);
B arr_c1(B t,      B x);
B arr_c2(B t, B w, B x);
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

extern i64 comp_currEnvPos;
extern B comp_currPath;
extern B comp_currArgs;
extern B comp_currSrc;
