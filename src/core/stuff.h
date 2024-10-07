// memory defs
#ifndef MAP_NORESERVE
 #define MAP_NORESERVE 0 // apparently needed for freebsd or something
#endif

static void storeu_u64(void* p, u64 v) { memcpy(p, &v, 8); }  static u64 loadu_u64(void* p) { u64 v; memcpy(&v, p, 8); return v; }
static void storeu_u32(void* p, u32 v) { memcpy(p, &v, 4); }  static u32 loadu_u32(void* p) { u32 v; memcpy(&v, p, 4); return v; }
static void storeu_u16(void* p, u16 v) { memcpy(p, &v, 2); }  static u16 loadu_u16(void* p) { u16 v; memcpy(&v, p, 2); return v; }
#define ptr_roundUp(P, N) ({ AUTO p_ = (P); u64 n_ = (N); TOPTR(typeof(*p_), (ptr2u64(p_)+n_-1) & ~(n_-1)); })
#define ptr_roundUpToEl(P) ({ AUTO p2_ = (P); ptr_roundUp(p2_, _Alignof(typeof(*p2_))); })

void print_allocStats(void);
void vm_pstLive(void);

typedef struct CustomObj {
  struct Value;
  V2v visit;
  V2v freeO;
} CustomObj;
void* m_customObj(u64 size, V2v visit, V2v freeO);

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

typedef void (*M_CopyF)(void*, usz, B, usz, usz);
typedef void (*M_FillF)(void*, usz, B, usz);
extern INIT_GLOBAL M_CopyF copyFns[el_MAX];
extern INIT_GLOBAL M_FillF fillFns[el_MAX];
#if SINGELI_SIMD
  typedef void (*copy_fn)(void*, void*, u64, void*);
  extern INIT_GLOBAL copy_fn tcopy_all[];
  #define COPY_TO_FROM(RP, RE, XP, XE, LEN) ({ \
    u8 re_ = (RE); \
    u8 xe_ = (XE); \
    assert(re_ < el_B && xe_ < el_B); \
    void* xp_ = (XP); \
    tcopy_all[re_*8+xe_](RP, xp_, LEN, GUARANTEED(xe_!=el_bit)? ARBITRARY(void*) : (u8*)xp_ - offsetof(TyArr,a)); \
  })
#else
  typedef void (*basic_copy_fn)(void*, void*, u64);
  extern INIT_GLOBAL basic_copy_fn basic_copy_all[];
  #define COPY_TO_FROM(RP, RE, XP, XE, LEN) ({ \
    u8 re_ = (RE); \
    u8 xe_ = (XE); \
    assert(re_ < el_B && xe_ < el_B); \
    basic_copy_all[re_*8+xe_](RP, XP, LEN); \
  })
#endif
#define COPY_TO(WHERE, ELT, MS, X, XS, LEN) copyFns[ELT](WHERE, MS, X, XS, LEN)
#define FILL_TO(WHERE, ELT, MS, X, LEN) fillFns[ELT](WHERE, MS, X, LEN)

#define TYARR_SZ(T,IA) fsizeof(TyArr, a, T##Atom, (u64)(IA))
#define TYARR_SZ2(T,IA) TYARR_SZ(T,IA)
#define TYARR_SZW(W,IA) (offsetof(TyArr, a) + (W)*(IA))

#define WRAP(X,IA,MSG) ({ i64 wV=(i64)(X); u64 iaW=(IA); if(RARE((u64)wV >= iaW)) { if(wV<0) wV+= iaW; if((u64)wV >= iaW) {MSG;} }; (usz)wV; })

static void tyarrv_freeP(Arr* x) { assert(PRNK(x)<=1 && IS_DIRECT_TYARR(PTY(x))); mm_free((Value*)x); }
static void tyarrv_free(B x) { tyarrv_freeP(a(x)); }

static inline void* m_arrUnchecked(u64 sz, u8 type, u64 ia) {
  Arr* r = mm_alloc(sz, type);
  r->ia = ia;
  return r;
}
SHOULD_INLINE void arr_check_size(u64 sz, u8 type, u64 ia);
SHOULD_INLINE void* m_arr(u64 sz, u8 type, u64 ia) {
  arr_check_size(sz, type, ia);
  return m_arrUnchecked(sz, type, ia);
}
static ShArr* m_shArr(ur r) {
  assert(r>1);
  return ((ShArr*)mm_alloc(fsizeof(ShArr, a, usz, r), t_shape));
}

FORCE_INLINE Arr* arr_rnk01(Arr* x, ur xr) {
  assert(xr<=1);
  SPRNK(x, xr);
  x->sh = &x->ia;
  return x;
}
static Arr* arr_shAtm(Arr* x) { return arr_rnk01(x, 0); }
static Arr* arr_shVec(Arr* x) { return arr_rnk01(x, 1); }

static usz* arr_shAlloc(Arr* x, ur r) { // sets rank, allocates & returns shape (or null if r<2); assumes x has rank≤1 (which will be the case for new allocations)
  assert(PRNK(x)<=1);
  if (r<=1) {
    arr_rnk01(x, r);
    return NULL;
  } else {
    usz* sh = x->sh = m_shArr(r)->a; // if m_shArr fails, the assumed rank≤1 guarantees the uninitialized x->sh won't break
    SPRNK(x,r);
    assert(sh!=NULL);
    return sh;
  }
}
static Arr* arr_shSetI(Arr* x, ur r, ShArr* sh) { // set rank and assign and increment shape if needed
  assert(PRNK(x)<=1);
  SPRNK(x,r);
  if (r>1) x->sh = ptr_inc(sh)->a;
  else     x->sh = &x->ia;
  return x;
}
static Arr* arr_shSetUO(Arr* x, ur r, ShArr* sh) { // set rank, and consume & assign shape if r>1
  assert(PRNK(x)<=1);
  SPRNK(x,r);
  if (r>1) x->sh = sh->a;
  else     x->sh = &x->ia;
  return x;
}
static Arr* arr_shSetUG(Arr* x, ur r, ShArr* sh) { // arr_shSetUO but guaranteed r>1, i.e. always consumes sh
  assert(r>1 && PRNK(x)<=1);
  SPRNK(x,r);
  x->sh = sh->a;
  return x;
}
static Arr* arr_shCopyUnchecked(Arr* n, B o) { // copy shape & rank from o to n
  assert(PRNK(n)<=1);
  ur r = SPRNK(n,RNK(o));
  if (r<=1) {
    n->sh = &n->ia;
  } else {
    usz* sh = SH(o);
    ptr_inc(shObjS(sh));
    n->sh = sh;
  }
  return n;
}
static Arr* arr_shCopy(Arr* n, B o) { // copy shape & rank from o to n; verifies that ia of both match
  assert(isArr(o) && IA(o)==n->ia);
  return arr_shCopyUnchecked(n, o);
}
static Arr* arr_shErase(Arr* x, ur r) { // replace x's shape with rank 0 or 1
  assert(r<=1);
  u8 xr = PRNK(x);
  if (xr!=r) {
    usz* prevsh = x->sh;
    arr_rnk01(x, r);
    if (xr>1) decShObj(shObjS(prevsh));
  }
  return x;
}
static Arr* arr_shReplace(Arr* x, ur r, ShArr* sh) { // replace x's shape with a new one; assumes r>1, but PRNK(x) can be anything
  assert(r>1);
  usz* prevsh = x->sh;
  u8 xr = PRNK(x);
  SPRNK(x, r);
  x->sh = sh->a;
  if (xr>1) decShObj(shObjS(prevsh));
  return x;
}
static void shcpy(usz* dst, usz* src, ux len) {
  PLAINLOOP for (ux i = 0; i < len; i++) dst[i] = src[i];
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
  PLAINLOOP for (i32 i = 0; i < len; i++) if (w[i]!=x[i]) return false;
  return true;
}
static bool ptr_eqShape(usz* wsh, ur wr, usz* xsh, ur xr) {
  if (wr != xr) return false;
  return eqShPart(wsh, xsh, wr);
}
static bool eqShape(B w, B x) { assert(isArr(w) && isArr(x)); return ptr_eqShape(SH(w), RNK(w), SH(x), RNK(x)); }

B bit_sel(B b, B e0, B e1); // consumes b; b must be bitarr; b⊏e0‿e1


Arr* allZeroes(usz ia); // ia⥊0 with undefined shape; always produces new array
Arr* allOnes(usz ia); // ia⥊1 with undefined shape; always produces new array

Arr* reshape_one(usz nia, B x); // nia⥊<x with undefined shape, with fl_asc, fl_dsc, and fl_squoze set; consumes x
B i64EachDec(i64 v, B x); // v¨ x; consumes x

B bit_negate(B x); // consumes; always produces new array
void bit_negatePtr(u64* rp, u64* xp, usz count); // count is number of u64-s
B widenBitArr(B x, ur axis); // consumes x, assumes bitarr; returns some array with cell size padded to the nearest of 8,16,32,64 if ≤64 bits, or a multiple of 64 bits otherwise
B narrowWidenedBitArr(B x, ur axis, ur cr, usz* csh); // consumes x.val; undoes widenBitArr, overriding shape past axis to cr↑csh

void bitnarrow(void* rp, ux rcsz, void* xp, ux xcsz, ux cam);
void bitwiden(void* rp, ux rcsz, void* xp, ux xcsz, ux cam);

Arr* customizeShape(B x); // consumes; returns new array with unset shape
Arr* cpyWithShape(B x); // consumes; returns new array with the same shape as x (SH(x) will be dangling, PSH(result) must be used to access it)
Arr* emptyArr(B x, ur xr); // doesn't consume; returns an empty array with the same fill as x; if xr>1, shape must be set
NOINLINE Arr* emptyWithFill(B fill); // consumes; returns new array with unset shape and the specified fill

B m_vec1(B a);      // complete fills
B m_vec2(B a, B b); // incomplete fills

// random stuff

#define addOn(V,X) ({ AUTO v_ = &(V); __builtin_add_overflow(*v_, X, v_); })
#define mulOn(V,X) ({ AUTO v_ = &(V); __builtin_mul_overflow(*v_, X, v_); })

static usz uszMul(usz a, usz b) {
  if (mulOn(a, b)) thrM("Size too large");
  return a;
}

static u8 selfElType_i32(i32 i) {
  return i==(i8)i? (i==(i&1)? el_bit : el_i8) : (i==(i16)i? el_i16 : el_i32);
}
static u8 selfElType_c32(u32 c) {
  return LIKELY(c<=255)? el_c8 : c<=65535? el_c16 : el_c32;
}
static u8 selfElType(B x) { // guaranteed to fit fill
  if (isF64(x)) {
    if (!q_i32(x)) return el_f64;
    return selfElType_i32(o2iG(x));
  }
  if (isC32(x)) return selfElType_c32(o2cG(x));
  return el_B;
}
static bool elChr(u8 x) { return x>=el_c8 && x<=el_c32; }
static bool elNum(u8 x) { return x<=el_f64; }
static bool elInt(u8 x) { return x<=el_i32; }

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

#define C1(F,  X) F##_c1(m_f64(0),  X)
#define C2(F,W,X) F##_c2(m_f64(0),W,X)

char* type_repr(u8 u);
char* pfn_repr(u8 u);
char* pm1_repr(u8 u);
char* pm2_repr(u8 u);
char* eltype_repr(u8 u);
char* genericDesc(B x); // doesn't consume
bool isPureFn(B x); // doesn't consume
bool isStr(B x); // doesn't consume; returns if x is a rank 1 array of characters (includes any empty array)
B bqn_merge(B x, u32 type); // consumes

B any_squeeze(B x); // consumes; accepts any array, returns one with the smallest type (doesn't recurse!)
B squeeze_deep(B x); // consumes; accepts any object, returns an object with all parts necessary for equality checking & hashing squeezed; if this function errors due to OOM, the argument won't yet be consumed
B num_squeeze(B x); // consumes; see note below
B chr_squeeze(B x); // consumes; see note below
// Note that num_squeeze & chr_squeeze don't check for fl_squoze, and unconditionally set it. Thus, don't call it on an array if it could be squeezable by the opposite method.
// or, if you do want to, if TI(x,elType) isn't of the squeezed type, either remove fl_squoze or call the other squeeze function.
// The functions below can be used as direct replacements of (num|chr)_squeeze if the argument might already be squeezed.
static inline B num_squeezeChk(B x) { return FL_HAS(x,fl_squoze)? x : num_squeeze(x); }
static inline B chr_squeezeChk(B x) { return FL_HAS(x,fl_squoze)? x : chr_squeeze(x); }

B def_fn_uc1(B t,    B o,           B x);  B def_fn_ucw(B t,    B o,           B w, B x);
B def_m1_uc1(Md1* t, B o, B f,      B x);  B def_m1_ucw(Md1* t, B o, B f,      B w, B x);
B def_m2_uc1(Md2* t, B o, B f, B g, B x);  B def_m2_ucw(Md2* t, B o, B f, B g, B w, B x);
B def_fn_is(B t,      B x);
B def_fn_im(B t,      B x);  B def_m1_im(Md1D* d,      B x);  B def_m2_im(Md2D* d,      B x);
B def_fn_iw(B t, B w, B x);  B def_m1_iw(Md1D* d, B w, B x);  B def_m2_iw(Md2D* d, B w, B x);
B def_fn_ix(B t, B w, B x);  B def_m1_ix(Md1D* d, B w, B x);  B def_m2_ix(Md2D* d, B w, B x);
B def_decompose(B x);

void noop_visit(Value* x);
#if HEAP_VERIFY
  void arr_visit(Value* x);
  #define VISIT_SHAPE(X) ({ if (PRNK(X)>1) mm_visitP(shObjP(X)); })
#else
  #define arr_visit noop_visit
  #define VISIT_SHAPE(X)
#endif

#define ICMP(W,X) ({ AUTO wt = (W); AUTO xt = (X); (wt>xt?1:0)-(wt<xt?1:0); })
SHOULD_INLINE i32 compareFloat(f64 w, f64 x) {
  if (RARE(w!=w || x!=x)) return (w!=w) - (x!=x);
  #if __x86_64__
    return (w>x) - !(w>=x); // slightly better codegen from being able to reuse the same compare instruction
  #else
    return (w>x) - (w<x);
  #endif
}
NOINLINE i32 compareF(B w, B x);
static i32 compare(B w, B x) { // doesn't consume; -1 if w<x, 1 if w>x, 0 if w≡x
  if (isNum(w) & isNum(x)) return compareFloat(o2fG(w), o2fG(x));
  if (isC32(w) & isC32(x)) return ICMP(o2cG(w), o2cG(x));
  return compareF(w, x);
}

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



#if USE_VALGRIND
  #include "../utils/valgrind.h"
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
static FC1 c1fn(B f) {
  if (isFun(f)) return c(Fun,f)->c1;
  if (isMd(f)) return md_c1;
  return arr_c1;
}
static FC2 c2fn(B f) {
  if (isFun(f)) return c(Fun,f)->c2;
  if (isMd(f)) return md_c2;
  return arr_c2;
}

// alloc stuff
#if ALLOC_STAT
  extern GLOBAL u64* ctr_a;
  extern GLOBAL u64* ctr_f;
  extern GLOBAL u64 actrc;
  extern GLOBAL u64 talloc;
  #if ALLOC_SIZES
    extern GLOBAL u32** actrs;
  #endif
#endif

#ifdef OOM_TEST
  extern GLOBAL i64 oomTestLeft;
  NOINLINE NORETURN void thrOOMTest(void);
#endif

#ifdef GC_EVERY_NTH_ALLOC
  extern GLOBAL u64 gc_depth;
  extern GLOBAL u64 nth_alloc;
#endif
FORCE_INLINE void preAlloc(usz sz, u8 type) {
  #ifdef OOM_TEST
    if (--oomTestLeft==0) thrOOMTest();
  #endif
  #ifdef GC_EVERY_NTH_ALLOC
    #if GC_EVERY_NTH_ALLOC<=1
      if (gc_depth==0) gc_forceGC(false);
    #else
      if (gc_depth==0 && --nth_alloc == 0) {
        gc_forceGC(false);
        nth_alloc = GC_EVERY_NTH_ALLOC;
      }
    #endif
  #endif
  #if ALLOC_STAT
    if (!ctr_a) {
      #if ALLOC_SIZES
        actrs = malloc(sizeof(u32*)*actrc);
        for (i32 i = 0; i < actrc; i++) actrs[i] = calloc(t_COUNT, sizeof(u32));
      #endif
      ctr_a = calloc(t_COUNT, sizeof(u64));
      ctr_f = calloc(t_COUNT, sizeof(u64));
    }
    assert(type<t_COUNT);
    #if ALLOC_SIZES
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
#define FINISH_OVERALLOC(P, S, E) tailVerifyReinit(P, S, E)
NOINLINE void reinit_portion(Arr* a, usz s, usz e);
#else
#define FINISH_OVERALLOC(P, S, E)
#define reinit_portion(...)
#endif
#define FINISH_OVERALLOC_A(A, S, L) FINISH_OVERALLOC(a(A), offsetof(TyArr,a)+(S), offsetof(TyArr,a)+(S)+(L));
FORCE_INLINE void preFree(Value* x, bool mmx) {
  #if ALLOC_STAT
    ctr_f[x->type]++;
  #endif
  #if VERIFY_TAIL
    if (!mmx) tailVerifyFree(x);
  #endif
  #if DEBUG
    if (x->type==t_empty) fatal("double-free");
    // u32 undef;
    // x->refc = undef;
    x->refc = -1431655000;
  #endif
  // x->refc = 0x61616161;
}
