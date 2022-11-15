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
static ShArr* shObj (B x) { return RFLD(SH(x), ShArr, a); }
static ShArr* shObjP(Value* x) { return RFLD(PSH((Arr*)x), ShArr, a); }
static void decShObj(ShArr* x) { tptr_dec(x, mm_free); }
static void decSh(Value* x) { if (RARE(PRNK(x)>1)) decShObj(shObjP(x)); }

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

static Arr* arr_shVec(Arr* x) {
  SPRNK(x, 1);
  x->sh = &x->ia;
  return x;
}
static usz* arr_shAlloc(Arr* x, ur r) { // sets rank, allocates & returns shape (or null if r<2); assumes x has rank≤1 (which will be the case for new allocations)
  assert(PRNK(x)<=1);
  if (r>1) {
    usz* sh = x->sh = m_shArr(r)->a; // if m_shArr fails, the assumed rank≤1 guarantees the uninitialized x->sh won't break
    SPRNK(x,r);
    return sh;
  }
  SPRNK(x,r);
  x->sh = &x->ia;
  return NULL;
}
static void arr_shSetI(Arr* x, ur r, ShArr* sh) { // set rank and assign and increment shape if needed
  SPRNK(x,r);
  if (r>1) { x->sh = ptr_inc(sh)->a; }
  else     { x->sh = &x->ia; }
}
static void arr_shSetU(Arr* x, ur r, ShArr* sh) { // set rank and assign shape
  SPRNK(x,r);
  if (r>1) { x->sh = sh->a;  }
  else     { x->sh = &x->ia; }
}
static Arr* arr_shCopyUnchecked(Arr* n, B o) {
  ur r = SPRNK(n,RNK(o));
  if (r<=1) {
    n->sh = &n->ia;
  } else {
    ptr_inc(shObj(o));
    n->sh = SH(o);
  }
  return n;
}
static Arr* arr_shCopy(Arr* n, B o) { // copy shape & rank from o to n
  assert(isArr(o));
  assert(IA(o)==n->ia);
  return arr_shCopyUnchecked(n, o);
}
static void shcpy(usz* dst, usz* src, size_t len) {
  // memcpy(dst, src, len*sizeof(usz));
  PLAINLOOP for (size_t i = 0; i < len; i++) dst[i] = src[i];
}

static usz shProd(usz* sh, usz s, usz e) {
  usz r = 1;
  PLAINLOOP for (i32 i = s; i < e; i++) r*= sh[i];
  return r;
}
static usz arr_csz(B x) {
  ur xr = RNK(x);
  if (xr<=1) return 1;
  return shProd(SH(x), 1, xr);
}
static bool eqShPart(usz* w, usz* x, usz len) {
  // return memcmp(w, x, len*sizeof(usz))==0;
  PLAINLOOP for (i32 i = 0; i < len; i++) if (w[i]!=x[i]) return false;
  return true;
}
static bool eqShape(B w, B x) { assert(isArr(w)); assert(isArr(x));
  ur wr = RNK(w); usz* wsh = SH(w);
  ur xr = RNK(x); usz* xsh = SH(x);
  if (wr!=xr) return false;
  if (wsh==xsh) return true;
  return eqShPart(wsh, xsh, wr);
}

B bit_sel(B b, B e0, B e1); // consumes b; b must be bitarr; b⊏e0‿e1
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
static bool elInt(u8 x) {
  return x<=el_i32;
}

// string stuff

B vec_addN(B w, B x); // consumes both; fills may be wrong
B vec_join(B w, B x); // consumes both
i32 num_fmt(char buf[30], f64 x);
#define NUM_FMT_BUF(N,X) char N[30]; num_fmt(N, X);
B append_fmt(B s, char* p, ...);
B make_fmt(char* p, ...);
void print_fmt(char* p, ...);
void fprint_fmt(FILE* f, char* p, ...);
#define AJOIN(X) s = vec_join(s,X) // consumes X
#define AOBJ(X) s = vec_addN(s,X) // consumes X
#define ACHR(X) AOBJ(m_c32(X))
#define A8(X) AJOIN(m_c8vec_0(X))
#define AU(X) AJOIN(utf8Decode0(X))
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
B squeeze_deep(B x); // consumes; accepts any object, returns an object with all parts necessary for equality checking & hashing squeezed; if this function errors due to OOM, the argument won't yet be consumed
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
NOINLINE i32 compareF(B w, B x);
static i32 compare(B w, B x) { // doesn't consume; -1 if w<x, 1 if w>x, 0 if w≡x; 0==compare(NaN,NaN)
  if (isNum(w) & isNum(x)) return CMP(o2fG(w), o2fG(x));
  if (isC32(w) & isC32(x)) return CMP(o2cG(w), o2cG(x));
  return compareF(w, x);
}
#undef CMP

NOINLINE bool atomEqualF(B w, B x);
static bool atomEqual(B w, B x) { // doesn't consume
  if(isF64(w)&isF64(x)) return w.f==x.f;
  if (w.u==x.u) return true;
  if (!isVal(w) | !isVal(x)) return false;
  return atomEqualF(w, x);
}

NOINLINE usz depthF(B x);
static usz depth(B x) { // doesn't consume
  if (isAtm(x)) return 0;
  if (TI(x,arrD1)) return 1;
  return depthF(x);
}



#ifdef USE_VALGRIND
  #include "../utils/valgrind.h"
  #include <valgrind/valgrind.h>
  static void pst(char* msg) {
    VALGRIND_PRINTF_BACKTRACE("%s", msg);
  }
#else
  #define vg_def_p(X, L)
  #define vg_undef_p(X, L)
  #define vg_def_v(X) (X)
  #define vg_undef_v(X) (X)
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

#ifdef OOM_TEST
  extern i64 oomTestLeft;
  NOINLINE NORETURN void thrOOMTest(void);
#endif

FORCE_INLINE void preAlloc(usz sz, u8 type) {
  #ifdef OOM_TEST
    if (--oomTestLeft==0) thrOOMTest();
  #endif
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
#if VERIFY_TAIL
void tailVerifyAlloc(void* ptr, u64 origSz, i64 logAlloc, u8 type);
void tailVerifyFree(void* ptr);
void tailVerifyReinit(void* ptr, u64 s, u64 e);
#define REINIT_TAIL(P, S, E) tailVerifyReinit(P, S, E)
#else
#define REINIT_TAIL(P, S, E)
#endif
#define REINIT_TAIL_A(A, S, L) REINIT_TAIL(a(A), offsetof(TyArr,a)+(S), offsetof(TyArr,a)+(S)+(L));
FORCE_INLINE void preFree(Value* x, bool mmx) {
  #ifdef ALLOC_STAT
    ctr_f[x->type]++;
  #endif
  #if VERIFY_TAIL
    if (!mmx) tailVerifyFree(x);
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
