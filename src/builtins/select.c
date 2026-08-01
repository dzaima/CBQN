// First Cell and Select (⊏)

// First Cell is just a slice, even if repeated (⊏⍟n𝕩, n>1)

// Select - 𝕨 ⊏ 𝕩
// Complications in Select mostly come from range checks and negative 𝕨
// Atom or enclosed atom 𝕨 and rank-1 𝕩: make new array
// Atom or enclosed atom 𝕨 and high-rank 𝕩: slice
// Empty 𝕨: no selection
// Float or generic 𝕨: attempt to squeeze, go generic cell size path if stays float
// High-rank 𝕩 & boolean 𝕨: either widens 𝕨 to i8, or goes generic cell path
//   SHOULD go a bit select path for small cells
//   SHOULD reshape for 1=≠𝕩
// Boolean 𝕩 (cell size = 1 bit):
//   𝕨 larger than 𝕩: convert 𝕩 to i8, select, convert back
//   Otherwise: select/shift bytes, reversed for fast writing
//     TRIED pext, doesn't seem faster (mask built with shifts anyway)
// 𝕩 with cell sizes of 1, 2, 4, or 8 bytes:
//   Small 𝕩 and i8 𝕨 with Singeli: use shuffles
//     COULD try to squeeze 𝕨 to i8 if small enough 𝕩
//   Boolean 𝕨: use bit_sel for blend or similar
//   Integer 𝕨 with Singeli: fused wrap, range-check, and gather
//     COULD try selecting from boolean with gather
//     COULD detect <Skylake where gather is slow
//   i32 𝕨: wrap, check, select one index at a time
//   i8 and i16 𝕨: separate range check in blocks to vectorize it
//     COULD wrap 𝕨 to a temp buffer
//     COULD copy 𝕩 to a buffer indexable directly by positive and negative indices
// Generic cell size 𝕩:
//   Computes a function that copies the necessary amount of bytes/bits
//   Specializes over i8/i16/i32 𝕨
// Nested 𝕨:
//   Recognizes a trailing element of a+↕b
//   Converts remaining indices to single select indices via +⌜
//   COULD have specialized select that skips OOB/negative checks

// Under Select - F⌾(i⊸⊏) 𝕩
// Specialized for rank-1 numeric 𝕩
// SHOULD apply to characters as well
// No longer needs to range-check but indices can be negative
//   COULD convert negative indices before selection
// Must check collisions if CHECK_VALID; uses a byte set
//   Sparse initialization if 𝕨 is much smaller than 𝕩
//   COULD call Mark Firsts (∊) for very short 𝕨 to avoid allocation

// Select Cells - inds⊸⊏⎉1 𝕩
// Squeeze indices if too wide for given 𝕩
// Single index: (also used for monadic ⊏˘ ⊣˝˘ ⊢˝˘)
//   Selecting a column of bits:
//     Row size <64: extract as with fold-cells
//   Selecting a column of 1, 2, 4, or 8-byte elements:
//     Short cells: pack vectors from 𝕩, or blend and permute
//     Long cells: dedicated scalar loop for each type
//   Otherwise, loop with mutable copy
// Boolean indices:
//   Short inds and short cells: Widen to i8
//   Otherwise: bitsel call per cell
//   SHOULD specialize wider input/output:
//     AVX2 extract 16 bits from 128: vbroadcasti128+vpshufb+vpmullw
//     AVX-512 extract 64 bits from 64: vpmultishiftqb/vpshufbitqmb
//     potentially better options via transposing to allow a shuffle to reorder multiple rows
// 1, 2, 4 or 8-byte data elements & short cells & short index list:
//   Split indices to available native shuffle width (e.g. 2‿1⊸⊏˘ n‿5⥊i16 → 2‿3‿0‿1⊸⊏˘ n‿10⥊i8)
//   Repeat indices if using ≤0.5x of shuffle width (e.g. 0‿0‿2⊸⊏˘ n‿3⥊i8 → 0‿0‿2‿3‿3‿5⊸⊏˘ n‿6⥊i8)
//   SHOULD disregard actual cell width if index range is small
//   COULD merge to wider elements if indices are in runs (e.g. 0‿1‿6‿7⊸⊏˘ n‿10⥊i16 → 0‿3⊸⊏˘ n‿5⥊i32)
//   COULD split into multiple index blocks
// 1-bit elements:
//   ≤8 bit cells in argument and result:
//     Widen cells to 8 bits, SIMD lookup table
//     SHOULD handle multidimensional cells by repeating indices, e.g. 0‿0‿2⊸⊏˘n‿3‿2⥊0
//   Else, inds⊸⊏⌾⍉ or generic path depending on heuristic
// Long inds / long cells:
//   Direct call to select function per cell
//     COULD have a more direct call that avoids overflow checking & wrapping
//   COULD generate full list of indices via arith
// 1-element cells: use (≠inds)/⥊x after checking ∧´inds∊0‿¯1
// Used for ⌽⎉1
// SHOULD use for /⎉k, ⌽⎉k, ↑⎉k, ↓⎉k, ↕⎉k, ⍉⎉k, probably more

#include "core.h"
#include "utils/talloc.h"
#include "utils/mut.h"
#include "utils/calls.h"
#include "builtins.h"

#if SINGELI
  #define SINGELI_FILE select
  #include "utils/includeSingeli.h"
  typedef bool (*SimdSelectFn)(void* w0, void* x0, void* r0, u64 wl, u64 xl);
  #define SIMD_SELECT(WE, XL) ({ AUTO we_=(WE); AUTO xl_=(XL); assert(we_>=el_i8 && we_<=el_i32 && xl_>=3 && xl_<=6); si_select_tab[4*(we_-el_i8)+xl_-3]; })
#endif

FORCE_INLINE B first_cell(ur dr, B x, ur xr, usz* xsh) {
  usz ia = shProd(xsh, dr, xr);
  Arr* r = TI(x,slice)(incG(x), 0, ia);
  usz* sh = arr_shAlloc(r, xr-dr);
  if (sh) shcpy(sh, xsh+dr, xr-dr);
  decG(x);
  return taga(r);
}
B select_c1(B t, B x) {
  if (isAtm(x)) thrM("⊏𝕩: 𝕩 cannot be an atom");
  ur xr = RNK(x);
  if (xr==0) thrM("⊏𝕩: 𝕩 cannot be rank 0");
  if (*SH(x)==0) thrF("⊏𝕩: 𝕩 shape cannot start with 0 (%H ≡ ≢𝕩)", x);
  return first_cell(1, x, xr, SH(x));
}
B select_powm(i64 am, B x) {
  assert(am > 1);
  if (isAtm(x)) thrM("⊏𝕩: 𝕩 cannot be an atom");
  ur xr = RNK(x);
  usz* xsh = SH(x);
  if (RARE(IA(x)==0)) {
    ur dr = IMIN(xr, am);
    for (ur i=0; i<dr; i++) if (xsh[i]==0) thrF("⊏𝕩: 𝕩 shape cannot start with 0 (%2H ≡ ≢𝕩)", xr-i, xsh+i);
  }
  if (xr < am) thrM("⊏𝕩: 𝕩 cannot be rank 0");
  return first_cell(am, x, xr, SH(x));
}

B select_c2(B t, B w, B x);
static NOINLINE NORETURN void select_properError(B w, B x) {
  select_c2(w, w, taga(cpyHArr(x)));
  fatal("select_properError");
}

static NOINLINE B select_list_cell(usz wi, B x) { // guarantees returning new array
  assert(isArr(x));
  B xf = getFillR(x);
  B xv = IGet(x, wi);
  B rb;
  if (numFill(xf) || chrFill(xf)) {
    rb = m_unit(xv);
  } else if (noFill(xf)) {
    rb = m_hunit(xv);
  } else {
    Arr* r = m_fillarrp(1);
    arr_shAtm(r);
    fillarrv_ptr(r)[0] = xv;
    fillarr_setFill(r, xf);
    NOGC_E;
    rb = taga(r);
  }
  decG(x);
  return rb;
}

static NOINLINE void select_depth2_bad(B w, B x) {
  usz wia = IA(w);
  if (IA(x)==0 && wia>0) {
    u8 we;
    w = squeeze_numTry(w, &we, SQ_NUM);
    if (elNum(we)) {
      thrF("𝕨⊏𝕩: Indexing out-of-bounds (%B∊𝕨, %s≡≠𝕩)", IGetU(w,0), *SH(x));
    }
  }
  if (RNK(w) > 1) thrF("𝕨⊏𝕩: Compound 𝕨 must have rank at most 1 (%H ≡ ≢𝕨)", w);
  SGetU(w)
  bool depth1 = depth(w)==1;
  for (ux i = 0; i < wia; i++) {
    B wc = GetU(w,i);
    if (depth1) {
      if (isAtm(wc) && !q_f64(wc)) thrF("𝕨⊏𝕩: 𝕨 must be an array of numbers or list of such (𝕨 contained %S)", genericDesc(wc));
    } else {
      if (isAtm(wc)) thrF("𝕨⊏𝕩: 𝕨 must be an array of numbers or list of such (𝕨 contained both an array and %S)", genericDesc(wc));
    }
  }
}
static NOINLINE NORETURN void select_depth2_bad_inds(B cw, ux axis, B x) {
  assert(axis < RNK(x) && isArr(cw));
  SGetU(cw)
  usz ia = IA(cw);
  ux len = SH(x)[axis];
  for (ux i = 0; i < ia; i++) {
    f64 c = o2fG(GetU(cw,i));
    i64 ci;
    if (!q_fi64o(&ci, c)) thrF("𝕨⊏𝕩: Bad index: %f along axis %z", c, axis);
    WRAP(ci, len, thrF("𝕨⊏𝕩: Indexing out-of-bounds along axis %z (%f ∊ %z⊑𝕨, %H≡≢𝕩)", axis, c, axis, x));
  }
  fatal("select_depth2_bad_inds should've errored");
}

B add_c2(B,B,B);
B mul_c2(B,B,B);
B lt_c2(B,B,B);
typedef struct {
  B inds; // array (rank≤1) of elNum arrays
  ux prod; // ×´⥊≠∘⥊¨inds
  ux left; // (=x) - ≠𝕨
  ux rank; // =w⊏x
  bool lastMaybeRange; // whether ¯1⊑r may be an a+↕b
} Depth2Inds;
Depth2Inds select_depth2_parse_inds(ux wia, B w, B x) { // consumes w; checks that w of w⊏x is valid, and, if so, returns a depth-2 array of number arrays (i.e. `squeeze_numTry¨ ((≠w)↑≢x) | w`)
  assert(isArr(w) && isArr(x));
  assert(IA(w)==wia && wia>0);
  if (RNK(w) > 1) { select_depth2_bad(w,x); fatal("should've errored"); }
  SGetU(w)
  if (wia > RNK(x)) { select_depth2_bad(w,x); thrF("𝕨⊏𝕩: Length of compound 𝕨 must be at most rank of 𝕩 (%s ≡ ≠𝕨, %H ≡ ≢𝕩)", wia, x); }
  
  Depth2Inds r;
  HArr_p inds = m_harr0v(wia);
  r.inds = inds.b;
  r.prod = 1;
  r.rank = r.left = RNK(x) - wia;
  usz* xsh = SH(x);
  i64 bounds[2];
  ux lastIA;
  for (ux i = 0; i < wia; i++) {
    B c = GetU(w, i);
    if (!isArr(c)) { select_depth2_bad(w,x); thrF("𝕨⊏𝕩: Elements of compound 𝕨 must be arrays (encountered %S)", genericDesc(c)); }
    r.rank+= RNK(c);
    
    u8 ce;
    c = squeeze_numTry(incG(c), &ce, SQ_NUM);
    lastIA = IA(c);
    r.prod*= lastIA;
    
    if (lastIA>0) {
      if (!elNum(ce)) { select_depth2_bad(w,x); thrM("𝕨⊏𝕩: Elements of compound 𝕨 must be arrays of numbers"); }
      
      if (!getRange_fns[ce](tyany_ptr(c), bounds, lastIA) || bounds[0] < -(i64)xsh[i] || bounds[1] >= xsh[i]) {
        select_depth2_bad(w,x);
        select_depth2_bad_inds(c, i, x);
      }
      if (bounds[0] < 0) {
        c = C2(add, c, C2(mul, m_f64(xsh[i]), C2(lt, incG(c), m_f64(0))));
      }
    }
    inds.a[i] = c;
  }
  decG(w);
  
  if (r.rank > UR_MAX) thrM("𝕨⊏𝕩: Result rank too large");
  r.lastMaybeRange = lastIA!=0 && (bounds[0]<0 || bounds[1]+1-bounds[0] == lastIA);
  return r;
}

B tbl_c2(Md1D* d, B w, B x);
B mul_c2(B, B, B);
B ud_c1(B, B);
B shape_c1(B, B);
typedef struct {
  B starts; // unspecified shape
  ux span, mul, add;
} Spans;
Spans select_depth2_inds(B w, B x, bool lastMaybeRange) { // doesn't consume; assumes w is a result of select_depth2_parse_inds; (⥊w⊏x) ≡ ⥊((mul×⥊starts)+⌜add+↕span)⊏x
  ur xr = RNK(x);
  usz wia = IA(w);
  assert(xr>0 && wia>=2 && wia<=xr);
  usz* xsh = SH(x);
  
  ux add = 0;
  ux span = shProd(xsh, wia, xr);
  ux mul = span;
  
  if (MAY_T(lastMaybeRange) && wia>=2) {
    B l = IGetU(w, wia-1);
    usz lia = IA(l);
    assert(lia!=0);
    if (HEURISTIC(lia <= 3)) goto lastNotRange; // TODO improve heuristic
    SGetU(l)
    usz l0 = o2sG(GetU(l,0));
    for (ux i = 1; i < lia; i++) if (o2sG(GetU(l,i)) != l0+i) goto lastNotRange; // TODO do in a less bad way
    add = l0*mul;
    span*= lia;
    mul*= xsh[wia-1];
    wia--;
  }
  lastNotRange:;
  
  SGet(w)
  B c = Get(w, 0);
  for (ux i = 1; i < wia; i++) {
    c = C2(mul, c, m_f64(xsh[i]));
    c = M1C2(tbl, add, c, Get(w, i));
  }
  return (Spans) {c, span, mul, add};
}

#define WRAP_SELECT_ONE(VAL, LEN, FMT, ARG) WRAP(VAL, LEN, thrF("𝕨⊏𝕩: Indexing out-of-bounds (" FMT "∊𝕨, %s≡≠𝕩)", ARG, LEN))

static NOINLINE B select_depth2_select(Spans ws, B x) { // consumes ws.starts
  B wst = ws.starts;
  ux span = ws.span;
  B r0;
  
  if (span != 1) {
    u8 xe = TI(x,elType);
    ux sia = IA(wst);
    UntaggedArr r = m_arrp_copyFill(x, sia*span);
    SGetU(wst)
    CFRes f = cf_get(span, a(x), 1);
    ux ro = 0;
    for (ux i = 0; i < sia; i++) {
      ux xi = ws.add + ws.mul*o2sG(GetU(wst,i));
      cf_call(f, r.data, ro, xi*f.mul);
      ro+= span*f.mul;
    }
    if (xe==el_B) NOGC_E;
    decG(wst);
    r0 = taga(r.obj);
  } else {
    if (ws.mul  != 1) wst = C2(mul, wst, m_f64(ws.mul));
    // if (span != 1) wst = M1C2(tbl, add, wst, C1(ud, m_f64(span)));
    if (ws.add  != 0) wst = C2(add, wst, m_f64(ws.add));
    r0 = C2(select, wst, C1(shape, incG(x)));
  }
  
  assert(reusable(r0));
  return r0;
}

static B select_depth2_impl(ux wia, B w, B x) { // wia<=1 only if invalid; or if x is empty, w may also be a number list
  Depth2Inds wi = select_depth2_parse_inds(wia, w, x);
  assert(wia>=2); // invalid cases thrown out above
  w = wi.inds;
  ux rr = wi.rank;
  
  B r0;
  if (wi.prod==0 || IA(x)==0) {
    r0 = taga(emptyArr(x, rr));
  } else {
    Spans ws = select_depth2_inds(w, x, wi.lastMaybeRange);
    r0 = select_depth2_select(ws, x);
  }
  
  if (rr >= 2) {
    ShArr* rsh = m_shArr(rr);
    arr_shReplace(a(r0), rr, rsh);
    
    usz* rshc = rsh->a;
    SGetU(w)
    for (ux i = 0; i < wia; i++) {
      B wc = GetU(w, i);
      ur wcr = RNK(wc);
      shcpy(rshc, SH(wc), wcr);
      rshc+= wcr;
    }
    shcpy(rshc, SH(x)+wia, wi.left);
    assert(rshc+wi.left == rsh->a+rr);
  } else {
    arr_shErase(a(r0), rr); // may re-write the rank of bi_emptyHVec/bi_emptyIVec/bi_emptyCVec/bi_emptySVec. ¯\_(ツ)_/¯
  }
  
  decG(w); decG(x);
  return r0;
}

B select_c2(B t, B w, B x) {
  if (isAtm(x)) thrM("𝕨⊏𝕩: 𝕩 cannot be an atom");
  ur xr = RNK(x);
  if (xr==0) thrM("𝕨⊏𝕩: 𝕩 cannot be a unit");
  if (isAtm(w)) {
    atomw:;
    usz wi = WRAP_SELECT_ONE(o2i64(w), *SH(x), "%R", w);
    if (xr==1) return select_list_cell(wi, x);
    usz csz = arr_csz(x);
    Arr* r = TI(x,slice)(incG(x), wi*csz, csz);
    usz* sh = arr_shAlloc(r, xr-1);
    if (sh) shcpy(sh, SH(x)+1, xr-1);
    decG(x);
    return taga(r);
  }
  
  usz wia = IA(w);
  Arr* r;
  ur wr = RNK(w);
  i32 rr = xr+wr-1; // only for depth-1 w
  if (wia <= 1) {
    if (wia == 0) {
      emptyRes:
      if (0 == *SH(x) && wr==1) {
        decG(w);
        return x;
      }
      r = emptyArr(x, rr);
      if (rr<=1) goto dec_ret;
      goto setsh;
    }
    B w0 = IGetU(w, 0);
    if (isAtm(w0)) {
      inc(w0);
      decG(w);
      w = w0;
      if (wr == 0) goto atomw;
      assert(rr >= 1);
      usz wi = WRAP_SELECT_ONE(o2i64(w), *SH(x), "%R", w);
      B r;
      usz* sh;
      if (xr == 1) {
        r = select_list_cell(wi, x);
        sh = arr_shAlloc(a(r), rr);
      } else {
        usz csz = arr_csz(x);
        Arr* ra = TI(x,slice)(incG(x), wi*csz, csz);
        sh = arr_shAlloc(ra, rr);
        if (sh) shcpy(sh+wr, SH(x)+1, xr-1);
        r = taga(ra);
        decG(x);
      }
      if (sh) PLAINLOOP for (ux i = 0; i < wr; i++) sh[i] = 1;
      return r;
    } else if (isArr(w0) && wr<=1) {
      // try to fast-path ⟨numarr⟩ ⊏ 𝕩; if not possible, 𝕨 is definitely erroneous
      inc(w0);
      decG(w);
      u8 w0e = TI(w0,elType);
      if (elNum(w0e)) return C2(select, w0, x);
      w0 = squeeze_numTry(w0, &w0e, SQ_MSGREQ(SQ_NUM));
      if (elNum(w0e)) return C2(select, w0, x);
      w = m_vec1(w0); // erroneous, speed doesn't matter
    }
    goto depth2;
  }
  
  B xf = getFillR(x);
  usz xn = *SH(x);
  if (xn==0) goto not_depth1_dec_xf; // empty x, non-empty w; error
  usz csz = arr_csz(x);
  u8 xl = cellWidthLog(x);
  usz ria = wia * csz;
  
  usz xia = IA(x);
  u8 xe = TI(x,elType);
  u8 we = TI(w,elType);
  
  
  #if SINGELI_AVX2 || SINGELI_NEON
    #define CPUSEL(W, NEXT) /*assumes 3≤xl≤6*/ \
      if (RARE(!SIMD_SELECT(we, xl)(wp, xp, rp, wia, xn))) select_properError(w, x);
    
  #else
    #define CASE(S, E)  case S: for (usz i=i0; i<i1; i++) ((E*)rp)[i] = ((E*)xp+off)[ip[i]]; break
    #define CASEW(S, E) case S: for (usz i=0; i<wia; i++) ((E*)rp)[i] = ((E*)xp)[WRAP_SELECT_ONE(wp[i], xn, "%i", wp[i])]; break
    #define CPUSEL(W, NEXT) /*assumes 3≤xl≤6*/ \
      if (sizeof(W) >= 4) {                           \
        switch(xl) { default:UD; CASEW(3,u8); CASEW(4,u16); CASEW(5,u32); CASEW(6,u64); } \
      } else {                                        \
        W* wt = NULL;                                 \
        for (usz bl=(1<<14)/sizeof(W), i0=0, i1=0; i0<wia; i0=i1) { \
          i1+=bl; if (i1>wia) i1=wia;                 \
          W min=wp[i0], max=min; for (usz i=i0+1; i<i1; i++) { W e=wp[i]; if (e>max) max=e; if (e<min) min=e; } \
          if (min<-(i64)xn) select_properError(w, x); \
          if (max>=(i64)xn) select_properError(w, x); \
          W* ip=wp; usz off=xn;                       \
          if (max>=0) { off=0; if (RARE(min<0)) {     \
            if (RARE(xn > (1ULL<<(sizeof(W)*8-1)))) { w=taga(NEXT(w)); mm_free((Value*)r); return C2(select, w, x); } \
            if (!wt) {wt=TALLOCP(W,i1-i0);} ip=wt-i0; \
            for (usz i=i0; i<i1; i++) { W e=wp[i]; ip[i]=e+((W)xn & (W)-(e<0)); } \
          } }                                         \
          switch(xl) { default:UD; CASE(3,u8); CASE(4,u16); CASE(5,u32); CASE(6,u64); } \
        }                                             \
        if (wt) TFREE(wt);                            \
      }
  #endif
    
  #if SINGELI_AVX2 || SINGELI_NEON
    bool bool_use_simd = we==el_i8 && xl==0 && xia<=128;
    
    #define BOOL_SPECIAL(W) \
      if (sizeof(W)==1 && bool_use_simd) { \
        if (RARE(!simd_select_bool128(wp, xp, rp, wia, xn))) select_properError(w, x); \
        goto setsh; \
      }
  #else
    bool bool_use_simd = 0;
    #define BOOL_SPECIAL(W)
  #endif
  
  if (!bool_use_simd && xe==el_bit && (csz&7)!=0 && HEURISTIC(xl==0? wia>=256 : wia>=4) && csz<128 && TI(w,arrD1)) {
    // test widen/narrow on bitarr input
    // ShArr* sh = RNK(x)==1? NULL : ptr_inc(shObj(x));
    // B t = C2(select, w, widenBitArr(x, 1));
    // B r = narrowWidenedBitArr(t, wr, xr-1, sh==NULL? &xn : sh->a+1);
    // if (sh!=NULL) ptr_dec(sh);
    // return r;
    if (csz==1) {
      if (wia/4>=xia) return taga(cpyBitArr(C2(select, w, taga(cpyI8Arr(x)))));
    } else if (HEURISTIC(csz>64? wia/2>=xn : wia>=xn/2)) {
      ShArr* sh = ptr_inc(shObj(x));
      B t = C2(select, w, widenBitArr(x, 1));
      B r = narrowWidenedBitArr(t, wr, xr-1, sh->a+1);
      ptr_dec(sh);
      return r;
    }
  }
  
  
  #define TYPE(W, NEXT) { W* wp = W##any_ptr(w);      \
    if (xl==0) { u64* xp=bitany_ptr(x);               \
      u64* rp; r = m_bitarrp(&rp, ria);               \
      BOOL_SPECIAL(W)                                 \
      u64 b=0;                                        \
      for (usz i = wia; ; ) {                         \
        i--;                                          \
        usz n = WRAP(wp[i], xn, select_properError(w, x)); \
        b = 2*b + ((((u8*)xp)[n/8] >> (n%8)) & 1);    \
        if (i%64 == 0) { rp[i/64]=b; if (!i) break; } \
      }                                               \
      goto setsh;                                     \
    }                                                 \
    if (xe!=el_B) {                                   \
      if (xl<3 || xl==7) goto generic_l;              \
      void* rp = m_tyarrlp(&r, xl-3, ria, arrNewType(TY(x))); \
      void* xp = tyany_ptr(x);                        \
      CPUSEL(W, NEXT)                                 \
      goto setsh;                                     \
    }                                                 \
    if (xl!=6) goto generic_l;                        \
    M_HARR(ra, wia); B* xp = arr_bptr(x);             \
    SLOWIF(xp==NULL) SLOW2("𝕨⊏𝕩", w, x);              \
    if (xp!=NULL) { for (usz i=0; i<wia; i++) HARR_ADD(ra, i, inc(xp[WRAP_SELECT_ONE(wp[i], xia, "%i", wp[i])])); } \
    else { SGet(x); for (usz i=0; i<wia; i++) HARR_ADD(ra, i, Get(x, WRAP_SELECT_ONE(wp[i], xia, "%i", wp[i]) )); } \
    r = a(withFill(HARR_FV(ra), xf)); goto setsh;     \
  }
  
  retry:
  switch (we) { default: UD;
    case el_bit: {
      if (xr!=1) {
        if (xe!=el_B && (csz<<elwBitLog(xe)) < 128) {
          dec(xf);
          return C2(select, taga(cpyI8Arr(w)), x);
        } else {
          goto generic_l;
        }
      }
      SGetU(x)
      B x0 = GetU(x, 0);
      B x1;
      if (xia<2) {
        u64* wp=bitany_ptr(w);
        usz i; for (i=0; i<wia/64; i++) if (wp[i]) break;
        if (i<wia/64 || bitp_l0(wp,wia)!=0) thrF("𝕨⊏𝕩: Indexing out-of-bounds (1∊𝕨, %s≡≠𝕩)", xn);
        x1 = x0;
      } else {
        x1 = GetU(x,1);
      }
      B r = bit_sel(w, x0, x1);
      decG(x);
      if (noFill(xf) && TI(r,elType)!=el_B) return taga(cpyHArr(r));
      return withFill(r, xf);
    }
    case el_i8:  TYPE(i8, cpyI16Arr)
    case el_i16: TYPE(i16,cpyI32Arr)
    case el_i32: TYPE(i32,cpyF64Arr)
    case el_f64: {
      if (MAY_T(FL_HAS(w, fl_squoze))) goto generic_l; // either has non-integers (i.e. error, thus don't care about speed) or very large (i.e. will hit memory bandwidth anyway)
      // else fallthrough - want to do integer 𝕨 if possible
    }
    case el_B: case el_c8: case el_c16: case el_c32: {
      w = squeeze_numTry(w, &we, SQ_MSGREQ(SQ_NUM));
      if (RANDOMIZE_HEURISTICS && we==el_f64) goto generic_l; // avoid infinite loop
      if (elNum(we)) goto retry;
      goto not_depth1_dec_xf; // erroneous input
    }
  }
  #undef CASE
  #undef CASEW
  
  not_depth1_dec_xf:;
  dec(xf);
  depth2: return select_depth2_impl(wia, w, x);
  
  generic_l: {
    if (xia==0) goto emptyRes;
    SLOW2("𝕨⊏𝕩", w, x);
    MAKE_MUT_INIT_WITHFILL(rm, ria, xe, xf);
    void* rp = rm->a;
    usz csz = arr_csz(x);
    CFRes f = cf_get(1, a(x), csz);
    ux scl = f.mul;
    void* wp = tyany_ptr(w);
    ux ri = 0, i = 0; // i is used for bad1 message
    switch(we) { default: UD;
      case el_bit:               for (; i<wia; i++) { ux c = bitp_get(wp,i);           if (c >= xn) { goto bad1; }   cf_call(f, rp, ri, c*scl); ri+= scl; }   break; // TODO something better
      case el_i8:  { i8*  w0=wp; for (i8*  wc=w0; wc<w0+wia; wc++) { usz c = WRAP(*wc, xn, { i=wc-w0; goto bad1; }); cf_call(f, rp, ri, c*scl); ri+= scl; } } break;
      case el_i16: { i16* w0=wp; for (i16* wc=w0; wc<w0+wia; wc++) { usz c = WRAP(*wc, xn, { i=wc-w0; goto bad1; }); cf_call(f, rp, ri, c*scl); ri+= scl; } } break;
      case el_i32: { i32* w0=wp; for (i32* wc=w0; wc<w0+wia; wc++) { usz c = WRAP(*wc, xn, { i=wc-w0; goto bad1; }); cf_call(f, rp, ri, c*scl); ri+= scl; } } break;
      case el_f64: { f64* w0=wp; for (f64* wc=w0; wc<w0+wia; wc++) {
        i64 cwi; if (!q_fi64o(&cwi, *wc)) { bad_wf64: i=wc-w0; goto bad1; }
        usz c = WRAP(cwi, xn, goto bad_wf64);
        cf_call(f, rp, ri, c*scl);
        ri+= scl;
      }} break;
    }
    r = mut_fp(rm);
    goto setsh;
    
    bad1:;
    mut_pfree(rm, i*csz);
    f64 badw = o2i64(IGetU(w,i));
    thrF("𝕨⊏𝕩: Indexing out-of-bounds (%f∊𝕨, %s≡≠𝕩)", badw, xn);
  }
  
  
  
  setsh:
  if (rr>1) {
    if (rr > UR_MAX) thrF("𝕨⊏𝕩: Result rank too large (%i≡=𝕨, %i≡=𝕩)", wr, xr);
    ShArr* sh = m_shArr(rr);
    shcpy(sh->a, SH(w), wr);
    shcpy(sh->a+wr, SH(x)+1, xr-1);
    arr_shSetUG(r, rr, sh);
  } else {
    arr_shVec(r);
  }
  
  dec_ret:;
  decG(w); decG(x); return taga(r);
}



B select_replace(u32 chr, B w, B x, B rep, usz wia, usz cam, usz csz) { // consumes all; (⥊rep)⌾(⥊w⊏cam‿csz⥊⊢) x; assumes csz>0, that w is a typed (elNum) list of valid indices (squeeze already attempted on el_f64), and that rep has the proper element count
  assert(csz > 0);
  #if CHECK_VALID
    TALLOC(bool, set, cam);
    bool sparse = wia < cam/64;
    if (!sparse) for (i64 i = 0; i < cam; i++) set[i] = false;
    #define SPARSE_INIT(WI) \
      if (sparse) for (usz i = 0; i < wia; i++) {                   \
        i64 cw = WI; if (RARE(cw<0)) cw+= (i64)cam; set[cw] = false; \
      }
    #define EQ(ITER,F) if (set[cw]) ITER if (F) thrF("𝔽⌾(a⊸%c): Incompatible result elements", chr); set[cw] = true;
    #define EQ1(F) EQ(,F)
    #define FREE_CHECK TFREE(set)
  #else
    #define SPARSE_INIT(GET)
    #define EQ(ITER,F)
    #define EQ1(F)
    #define FREE_CHECK
  #endif
  
  #define READ_W(N,I) i64 N = (i64)wp[I]; if (RARE(N<0)) N+= (i64)cam
  u8 we = TI(w,elType); assert(elNum(we) || wia==0);
  u8 xe = TI(x,elType);
  u8 re = el_or(xe, TI(rep,elType));
  // w = taga(cpyF64Arr(w)); we = el_f64; // test the float path
  DIRECTARR_COPY(r, re, x);
  B rb = r.obj;
  SLOWIF(!q_beq(rb,x) && cam>100 && wia<cam/50) SLOW2("⌾(𝕨⊸⊏)𝕩 or ⌾(𝕨⊸⊑)𝕩 because not reusable", w, x);
  
  if (we==el_f64) {
    f64* wp = f64any_ptr(w);
    SPARSE_INIT((i64)wp[i])
    if (csz==1) {
      SGet(rep)
      for (usz i = 0; i < wia; i++) {
        READ_W(cw, i);
        B cn = Get(rep, i);
        EQ1(!compatible(DIRECTARR_GETU(r, cw), cn));
        DIRECTARR_REPLACE(r, cw, cn);
      }
    } else {
      SGetU(rep)
      for (usz i = 0; i < wia; i++) {
        READ_W(cw, i);
        EQ(for (usz j = 0; j < csz; j++), !compatible(DIRECTARR_GETU(r, cw*csz + j), GetU(rep, i*csz + j)));
        DIRECTARR_REPLACE_RANGE(r, cw*csz, rep, i*csz, csz); // TODO use cf_*
      }
    }
    goto dec_ret_rb;
  }
  assert(elInt(we) || wia==0);
  
  w = toI32Any(w);
  i32* wp = i32any_ptr(w);
  SPARSE_INIT(wp[i])
  switch (re) { default: UD;
    case el_i8:  rep = toI8Any(rep);  goto do_u8;
    case el_c8:  rep = toC8Any(rep);  goto do_u8;
    case el_i16: rep = toI16Any(rep); goto do_u16;
    case el_c16: rep = toC16Any(rep); goto do_u16;
    case el_i32: rep = toI32Any(rep); goto do_u32;
    case el_c32: rep = toC32Any(rep); goto do_u32;
    case el_f64: rep = toF64Any(rep); goto do_f64;
    case el_bit: {
      assert(TI(rep,elType)==el_bit);
      u64* np = bitarr_ptr(rep);
      u64* rp = r.data;
      if (csz==1) {
        for (usz i = 0; i < wia; i++) {
          READ_W(cw, i);
          bool cn = bitp_get(np, i);
          EQ1(cn != bitp_get(rp, cw));
          bitp_set(rp, cw, cn);
        }
      } else {
        for (usz i = 0; i < wia; i++) {
          READ_W(cw, i);
          EQ(for (usz j = 0; j < csz; j++), bitp_get(np, i*csz + j) != bitp_get(rp, cw*csz + j));
          COPY_TO(rp, el_bit, cw*csz, rep, i*csz, csz);
        }
      }
      goto dec_ret_rb;
    }
    case el_B: {
      B* rp = r.data;
      if (csz==1) {
        SGet(rep)
        for (usz i = 0; i < wia; i++) {
          READ_W(cw, i);
          B cn = Get(rep, i);
          EQ1(!compatible(cn,rp[cw]));
          dec(rp[cw]);
          rp[cw] = cn;
        }
      } else {
        SGetU(rep)
        for (usz i = 0; i < wia; i++) {
          READ_W(cw, i);
          EQ(for (usz j = 0; j < csz; j++), !compatible(GetU(rep, i*csz + j), rp[cw*csz + j]));
          for (usz j = 0; j < csz; j++) dec(rp[cw*csz + j]);
          COPY_TO(rp, el_B, cw*csz, rep, i*csz, csz);
        }
      }
      goto dec_ret_rb;
    }
  }
  
  #define IMPL(T, COMPATIBLE) do {  \
    if (csz!=1) goto do_tycell;     \
    T* rp = r.data;                 \
    T* np = tyany_ptr(rep);         \
    for (usz i = 0; i < wia; i++) { \
      READ_W(cw, i);                \
      T cn = np[i];                 \
      EQ1(!COMPATIBLE(cn, rp[cw])); \
      rp[cw] = cn;                  \
    }                               \
    goto dec_ret_rb;                \
  } while(0)
  
  #define INT_EQ(A,B) ((A)==(B))
  do_u8:  IMPL(u8,  INT_EQ);
  do_u16: IMPL(u16, INT_EQ);
  do_u32: IMPL(u32, INT_EQ);
  do_f64: IMPL(f64, compatibleFloats);
  #undef INT_EQ
  #undef IMPL
  
  do_tycell:;
  ux cwidth = csz * elWidth(re);
  u8* rp = r.data;
  u8* np = tyany_ptr(rep);
  MatchFnObj eq = MATCHR_GET(re,re);
  for (usz i = 0; i < wia; i++) {
    READ_W(cw, i);
    EQ1(!MATCH_CALL(eq, rp + cw*cwidth, np + i*cwidth, csz));
    COPY_TO(rp, re, cw*csz, rep, i*csz, csz);
  }
  goto dec_ret_rb;
  
  
  
  dec_ret_rb:;
  decG(w); decG(rep);
  FREE_CHECK;
  return rb;
  
  #undef SPARSE_INIT
  #undef EQ
  #undef EQ1
  #undef FREE_CHECK
}

static void* m_arrv_same_t(B* r, B** rbp, usz ia, u8 ty, B src) {
  assert(isArr(src));
  u8 se = TIi(ty,elType);
  if (se==el_B) {
    B fill = getFillR(src);
    if (noFill(fill)) {
      HArr_p p = m_harrUv(ia);
      *rbp = p.a;
      *r = p.b;
    } else {
      Arr* ra = m_fillarrp(ia);
      fillarr_setFill(ra, fill);
      *rbp = fillarrv_ptr(ra);
      *r = taga(ra);
    }
    FILL_TO(*rbp, el_B, 0, m_f64(0), ia);
    NOGC_E;
    return *rbp;
  } else {
    return m_tyarrlbv(r, arrTypeBitsLog(ty), ia, arrNewType(ty));
  }
}
static void* m_arrv_same(B* r, B** rbp, usz ia, B src) { // makes a new array with same element type and fill as src, but new ia
  return m_arrv_same_t(r, rbp, ia, TY(src), src);
}

B slash_c2(B, B, B);
B select_cells_base(B inds, B x0, ux csz, ux cam);
extern void (*const si_select_cells_bit_lt64)(u64*,u64*,usz,usz,usz); // from fold.c (fold.singeli)
extern usz (*const si_select_cells_byte)(void*,void*,usz,usz,u8);

B select_cells_single(usz ind, B x, usz cam, usz l, usz csz) { // ⥊ ind ⊏˘ cam‿l‿csz ⥊ x
  Arr* ra;
  if (l==1) {
    ra = cpyWithShape(incG(x));
    arr_shErase(ra, 1);
  } else {
    u8 xe = TI(x,elType);
    u8 ewl= elwBitLog(xe);
    u8 xl = multWidthLog(csz, ewl);
    usz ria = cam*csz;
    if (xl>=7 || (xl<3 && xl>0)) { // generic case
      UntaggedArr r = m_arrp_copyFill(x, ria);
      CFRes f = cf_get(1, a(x), csz);
      usz xi = ind*f.mul, ri = 0;
      for (usz i = 0; i < cam; i++) {
        cf_call(f, r.data, ri, xi);
        xi+= l*f.mul;
        ri+= f.mul;
      }
      NOGC_E;
      ra = r.obj;
      return taga(arr_shVec(ra));
    } else if (xe==el_B) {
      assert(csz == 1);
      SGet(x)
      HArr_p rp = m_harrUv(ria);
      for (usz i = 0; i < cam; i++) rp.a[i] = Get(x, i*l+ind);
      NOGC_E; ra = (Arr*)rp.c;
      goto copyFill;
    } else {
      void* rp = m_tyarrlbp(&ra, ewl, ria, el2t(xe));
      void* xp = tyany_ptr(x);
      if (xl == 0) {
        #if SINGELI
        if (l < 64) si_select_cells_bit_lt64(xp, rp, cam, l, ind);
        else
        #endif
        for (usz i=0; i<cam; i++) bitp_set(rp, i, bitp_get(xp, i*l+ind));
      } else {
        usz i0 = 0;
        #if SINGELI
        i0 = si_select_cells_byte((u8*)xp + (ind<<(xl-3)), rp, cam, l, xl-3);
        #endif
        switch(xl) { default: UD;
          case 3: PLAINLOOP for (usz i=i0; i<cam; i++) ((u8* )rp)[i] = ((u8* )xp)[i*l+ind]; break;
          case 4: PLAINLOOP for (usz i=i0; i<cam; i++) ((u16*)rp)[i] = ((u16*)xp)[i*l+ind]; break;
          case 5: PLAINLOOP for (usz i=i0; i<cam; i++) ((u32*)rp)[i] = ((u32*)xp)[i*l+ind]; break;
          case 6: PLAINLOOP for (usz i=i0; i<cam; i++) ((f64*)rp)[i] = ((f64*)xp)[i*l+ind]; break;
        }
      }
    }
  }
  return taga(ra);
  
  copyFill:
  return withFill(taga(ra), getFillR(x));
}

#define CLZC(X) (64-(CLZ64((u64)(X))))

#ifdef SELECT_ROWS_PRINTF
  #undef SELECT_ROWS_PRINTF
  #define SELECT_ROWS_PRINTF(...) printf(__VA_ARGS__)
#else
  #define SELECT_ROWS_PRINTF(...)
#endif

#define INDS_BUF_MAX 64 // only need 32 bytes for AVX2 & 16 for NEON, but have more for past-the-end pointers and writes

static Arr* m_tyslice_el(void* data, u8 elt, ux ia) {
  assert(elt != el_bit);
  return m_tyslice(data, a(emptyIVec()), t_i8slice + elt-el_i8, ia);
}

B transp_c1(B t, B x);
B select_rows_direct(B x, ux csz, ux cam, void* inds, ux indn, u8 ie) { // ⥊ (indn↑inds As ie)⊸⊏˘ cam‿csz⥊x; if inds are valid and csz<=128, ie must be <=el_i8
  assert(csz!=0 && cam!=0 && indn!=0);
  assert(csz*cam == IA(x));
  assert(ie<=el_i32);
  
  MAYBE_UNUSED u8 inds_buf[INDS_BUF_MAX];
  bool generic_allowed = true; // whether required interpretation of x hasn't changed from its real one
  if (csz==1) { // TODO maybe move to select_rows_B and require csz>=2 here?
    i64 bounds[2];
    if (!getRange_fns[ie](inds, bounds, indn) || bounds[0]<-1 || bounds[1]>0) goto generic_any;
    return C2(slash, m_f64(indn), taga(arr_shVec(customizeShape(x))));
  }
  assert(csz>=2);
  
  ux ria = indn * cam;
  B r;
  B* rbp = NULL;
  u8* xp;
  u8 xe = TI(x,elType);
  u8 lb = arrTypeWidthLog(TY(x));
  
  if (xe==el_B) {
    if (B_IS_U64) goto generic_any;
    xp = (u8*) arr_bptr(x);
    if (xp == NULL) goto generic_any;
  } else {
    xp = tyany_ptr(x);
    if (xe == el_bit) {
      #if SINGELI_AVX2 || SINGELI_NEON
        if (indn<=8 && csz<=8) goto bit_ok;
      #endif
      if (HEURISTIC(csz<=128 ? indn<192 : indn*5 > csz)) {
        Arr* xs = TI(x,slice)(x, 0, IA(x));
        usz* xsh = arr_shAlloc(xs, 2);
        xsh[0] = cam; xsh[1] = csz;
        B tr = C1(transp, taga(xs));
        B warr;
        if (ie != el_bit) {
          warr = taga(arr_shVec(m_tyslice_el(inds, ie, indn)));
        } else {
          u64* wp; warr = m_bitarrv(&wp, indn);
          memcpy(wp, inds, (indn+7)>>3);
        }
        return C1(transp, C2(select, warr, tr));
      }
      goto generic_any;
      goto bit_ok; bit_ok:;
    }
  }
  
  MAYBE_UNUSED bool fast;
  ux xbump = csz<<lb;
  ux rbump = indn<<lb;
  i64 bounds[2];
  
  if (ie==el_bit) {
    // TODO path for xe==el_bit + long indn
    if (HEURISTIC_BOUNDED(csz>32 || indn>32 || indn>INDS_BUF_MAX, xe!=el_bit && (csz>8 || indn>8), indn<=32)) { // TODO properly tune
      u8* rp = m_arrv_same(&r, &rbp, ria, x);
      for (ux i = 0; i < cam; i++) {
        bitselFns[lb](rp, inds, loadu_u64(xp), loadu_u64(xp + (1<<lb)), indn);
        xp+= xbump;
        rp+= rbump;
      }
      goto decG_B_ret;
    } else {
      assert(inds_buf != inds);
      COPY_TO_FROM(inds_buf, el_i8, inds, el_bit, indn);
      inds = inds_buf;
      ie = el_i8;
      bounds[0] = 0;
      bounds[1] = 1; // might be an over-estimate, hopefully doesn't matter (and csz≥2 anyway)
      #if SINGELI
      fast = true;
      goto skip_bounds_check;
      #endif
    }
  }
  
  
  #if SINGELI
  assert(INDS_BUF_MAX_COPY == INDS_BUF_MAX);
  {
    fast = ie==el_i8;
    
    if (!getRange_fns[ie](inds, bounds, indn)) goto generic_int;
    if (bounds[1] >= (i64)csz) goto generic_int;
    if (bounds[0] < 0) {
      if (bounds[0] < -(i64)csz) goto generic_int;
      if (csz < 128 && indn < INDS_BUF_MAX) {
        assert(ie == el_i8);
        si_wrap_inds[0](inds, inds_buf, indn, csz);
        bounds[0] = 0;
        bounds[1] = csz-1;
        inds = inds_buf;
      } else {
        fast = false;
      }
    }
    skip_bounds_check:;
    assert(ie==el_i8 || csz>128);
    
    #if SINGELI_AVX2 || SINGELI_NEON
      if (fast) {
        generic_allowed = false;
        ux sh = select_rows_widen[lb](inds, inds_buf, bounds[1], indn); // TODO null element in table for guaranteed-zero
        if (sh!=0) {
          SELECT_ROWS_PRINTF("widening indices by factor of %d:\n", 1<<sh);
          SELECT_ROWS_PRINTF("  src: lb=%d, ie=%d, csz=%zu, indn=%zu\n", lb, ie, csz, indn);
          inds = inds_buf;
          lb-= sh;
          csz<<= sh;
          indn<<= sh;
          SELECT_ROWS_PRINTF("  dst: lb=%d, ie=%d, csz=%zu, indn=%zu\n", lb, ie, csz, indn);
        }
      }
    #endif
    
    #if SINGELI_AVX2 || SINGELI_NEON
      if (xe==el_bit) {
        assert(ie==el_i8 && csz<=8 && indn<=8 && csz>=2 && indn>=1);
        // TODO si_select_cells_bit_lt64 for indn==1
        static const u8 rep_lut[9] = {0,3,2,1,1,0,0,0,0};
        u8 exp = rep_lut[IMAX(csz, indn)];
        ux rindn = indn<<exp;
        ux rcsz = csz<<exp;
        assert(rcsz<=8 && rindn<=8);
        
        ux rcam = (cam + TAIL(ux,exp))>>exp;
        
        if (rcsz!=8) {
          u64* xp2;
          B x2 = m_bitarrv(&xp2, 8*cam);
          bitwiden(xp2, 8, xp, rcsz, cam);
          decG(x);
          x = x2;
          xp = (void*) xp2;
          SELECT_ROWS_PRINTF("8bit: widen %zu‿%zu → ⟨%zu,%zu→8⟩\n", cam, csz, rcam, rcsz);
        }
        
        if (exp!=0) {
          simd_repeat_inds(inds, inds_buf, indn, csz);
          inds = inds_buf;
        }
        
        u64* rp;
        ux ria0 = rindn!=8? 8*rcam : ria;
        r = m_bitarrv(&rp, ria0);
        SELECT_ROWS_PRINTF("8bit: indn=%zu rindn=%zu csz=%zu rcsz=%zu cam=%zu ria0=%zu rcam=%zu\n", indn, rindn, csz, rcsz, cam, ria0, rcam);
        si_select_rows_8bit(inds, rindn, xp, rp, (ria0+7)/8);
        
        if (rindn!=8) {
          SELECT_ROWS_PRINTF("8bit: narrow 8 → %zu<<%d\n", csz, exp);
          
          u64* rp2;
          B r2 = m_bitarrv(&rp2, 8*rcam);
          bitnarrow(rp2, rindn, rp, 8, rcam);
          tyarrv_free(r);
          r = r2;
          
          ux ria1 = IA(r);
          assert(ria <= ria1);
          FINISH_OVERALLOC(a(r), offsetof(TyArr,a) + (ria+7)/8, offsetof(TyArr,a) + (ria1+7)/8);
          a(r)->ia = ria;
        }
        
        goto decG_ret;
      }
    #endif
    
    u8* rp = m_arrv_same(&r, &rbp, ria, x);
    
    ux slow_cam = cam;
    #if SINGELI_AVX2 || SINGELI_NEON
    ux lnt = CLZC(csz-1); // ceil-log2 of number of elements in table
    
    if (fast && lnt < select_rows_tab_h) {
      u8 max_indn = select_rows_max_indn[lb];
      if (indn > max_indn) goto no_fast;
      u8 min_lnt = select_rows_min_logcsz[lb];
      ux used_lnt;
      SELECT_ROWS_PRINTF("csz: %zu/%d; inds: %d/%d\n", csz, 1<<min_lnt, (int)indn, max_indn);
      
      ux indn_real = indn;
      ux rep;
      if (indn*2 <= max_indn) {
        assert(max_indn<=32); // otherwise inds_buf hard-coded size may need to change
        rep = simd_repeat_inds(inds, inds_buf, indn, csz);
        indn_real = rep*indn;
        SELECT_ROWS_PRINTF("rep: %zu; inds: %zu→%zu; csz: %zu→%zu - raw repeat\n", rep, indn, indn*rep, csz, csz*rep);
        PLAINLOOP while (rep*indn > max_indn) rep--; // simd_repeat_inds over-estimates
        SELECT_ROWS_PRINTF("rep: %zu; inds: %zu→%zu; csz: %zu→%zu - valid inds\n", rep, indn, indn*rep, csz, csz*rep);
        
        used_lnt = min_lnt;
        ux fine_csz = 1ULL<<(min_lnt+1); // TODO have a proper per-element-type LUT of "target LUT size"
        if (csz < fine_csz) {
          ux cap = fine_csz / csz;
          if (rep > cap) rep = cap;
        } else rep = 1;
        
        ux new_lnt = CLZC(csz*rep-1);
        if (new_lnt > used_lnt) used_lnt = new_lnt;
        
        SELECT_ROWS_PRINTF("rep: %zu; inds: %zu→%zu; csz: %zu→%zu - valid table\n", rep, indn, indn*rep, csz, csz*rep);
        inds = inds_buf;
        
      } else {
        rep = 1;
        used_lnt = IMAX(lnt, min_lnt);
      }
      
      assert(indn*rep <= max_indn);
      AUTO fn = select_rows_tab[used_lnt*4 + lb];
      if (fn == null_fn) goto no_fast;
      ux done = fn(inds, xp, csz*rep, rp, indn*rep, rp + cam*rbump) * rep;
      ux left = cam - done;
      SELECT_ROWS_PRINTF("done_rows: %zu; left_rows: %zu; left_els: %zu; left_max: %zu\n", done, left, indn*left, indn_real);
      if (left) {
        xp+= done * xbump;
        rp+= done * rbump;
        if (left*csz <= 127) {
          assert(indn*left <= indn_real);
          bool ok = SIMD_SELECT(ie, lb+3)(inds, xp, rp, indn*left, I64_MAX); assert(ok);
        } else {
          slow_cam = left;
          goto no_fast;
        }
      }
      
      goto decG_B_ret;
    }
    no_fast:;
    #endif
    
    SimdSelectFn fn = SIMD_SELECT(ie, lb+3);
    for (ux i = 0; i < slow_cam; i++) {
      fn(inds, xp, rp, indn, csz);
      xp+= xbump;
      rp+= rbump;
    }
    goto decG_B_ret;
  }
  #else
    (void) bounds;
  #endif
  
  generic_any:;
  if (ie==el_bit) {
    u64* rp;
    B indo = m_bitarrv(&rp, indn);
    memcpy(rp, inds, (indn+7)>>3);
    assert(generic_allowed);
    return select_cells_base(indo, x, csz, cam);
  }
  goto generic_int;
  
  generic_int:;
  assert(ie!=el_bit && generic_allowed);
  B indo = taga(arr_shVec(m_tyslice_el(inds, ie, indn)));
  return select_cells_base(indo, x, csz, cam);
  
  decG_B_ret:;
  if (rbp != NULL) {
    incEach(rbp, ria); // TODO if only a few columns are selected, could incBy in a stride per selected column
  }
  decG_ret: MAYBE_UNUSED;
  decG(x);
  return r;
}

B select_rows_B(B x, ux csz, ux cam, B inds) { // consumes inds,x; ⥊ inds⊸⊏˘ cam‿csz⥊x; if inds isn't rank 1, result may or may not be high rank
  assert(csz*cam == IA(x));
  if (csz==0) goto generic;
  if (cam<=1) {
    if (cam==0) goto retEmpty;
    return C2(select, inds, taga(arr_shVec(TI(x,slice)(x, 0, IA(x)))));
  }
  
  ux in = IA(inds);
  B r;
  if (in == 0) { retEmpty:; r = taga(emptyVec(x)); goto decret_inds_x; }
  if (in == 1) {
    B w = IGetU(inds,0); if (!q_f64(w)) goto generic;
    r = select_cells_single(WRAP_SELECT_ONE(o2i64(w), csz, "%R", w), x, cam, csz, 1);
    goto decret_inds_x;
  }
  u8 ie = TI(inds,elType);
  if (csz<=2? ie!=el_bit : csz<=128? ie>el_i8 : !elInt(ie)) {
    inds = squeeze_numTry(inds, &ie, SQ_BEST);
    if (!elInt(ie)) goto generic;
  }
  void* ip = tyany_ptr(inds);
  
  r = select_rows_direct(x, csz, cam, (u8*)ip, in, ie);
  goto decret_inds;
  
  generic:;
  return select_cells_base(inds, x, csz, cam);
  decret_inds_x: decG(x);
  decret_inds: decG(inds);
  return r;
}



SHOULD_INLINE i64 i64get_i32(void* xp, ux i, bool* bad) {
  return ((i32*)xp)[i];
}
SHOULD_INLINE i64 i64get_f64(void* xp, ux i, bool* bad) {
  f64 f = ((f64*)xp)[i]; i64 fi;
  if (q_fi64o(&fi, f)) return fi;
  *bad = true;
  return 0;
}

SHOULD_INLINE bool select_each_impl(DirectArr r, u8 re, B c, ux xn, void* wp, usz wia, i64 (*getW)(void*, ux, bool*)) {
  #define GETW ({ bool bad=false; i64 wc = getW(wp, i, &bad); if (bad) goto bad; WRAP(wc, xn, goto bad); })
  u64 uval;
  switch (re) { default: UD;
    case el_bit:;
      bool cb = o2bG(c);
      if (cb) for (ux i = 0; i < wia; i++) bitp_set(r.data, GETW, true);
      else    for (ux i = 0; i < wia; i++) bitp_set(r.data, GETW, false);
      return true;
    case el_i8:  uval = o2iG(c); goto do_u8;
    case el_i16: uval = o2iG(c); goto do_u16;
    case el_i32: uval = o2iG(c); goto do_u32;
    case el_f64: uval = r_f64u(o2fG(c)); goto do_u64;
    case el_c8:  uval = o2cG(c); goto do_u8;
    case el_c16: uval = o2cG(c); goto do_u16;
    case el_c32: uval = o2cG(c); goto do_u32;
    case el_B:
      for (ux i = 0; i < wia; i++) {
        B* p = (B*)r.data + GETW;
        dec(*p);
        *p = c;
      }
      return true;
  }
  UD;
  
  do_u8:  for (ux i = 0; i < wia; i++) ((u8 *)r.data)[GETW] = uval; return true;
  do_u16: for (ux i = 0; i < wia; i++) ((u16*)r.data)[GETW] = uval; return true;
  do_u32: for (ux i = 0; i < wia; i++) ((u32*)r.data)[GETW] = uval; return true;
  do_u64: for (ux i = 0; i < wia; i++) ((u64*)r.data)[GETW] = uval; return true;
  #undef GETW
  
  bad: return false;
}

B select_ucw(B t, B o, B w, B x) {
  if (RARE(isAtm(x))) { def: return def_fn_ucw(t, o, w, x); }
  u8 we;
  if (isAtm(w)) {
    if (RARE(!q_f64(w))) goto def;
    w = m_unit(w);
    we = TI(w,elType);
    assert(elNum(we));
  } else {
    we = TI(w,elType);
    if (!elInt(we)) {
      w = squeeze_numTry(w, &we, SQ_MSGREQ(SQ_NUM));
      if (!elNum(we)) goto def;
    }
  }
  
  usz wia = IA(w);
  B rep;
  if (MAY_F(isArr(o) && RNK(x)>0)) {
    usz xn = *SH(x);
    i64 buf[2];
    if (wia!=0 && (!getRange_fns[we](tyany_ptr(w), buf, wia) || buf[0]<-(i64)xn || buf[1]>=xn)) {
      bad:
      C2(select, w, x);
      fatal("select_ucw expected to error");
    }
    rep = incG(o);
  } else if (MAY_F(isFun(o) && TY(o)==t_md1D && RNK(x)==1)) {
    Md1D* od = c(Md1D,o);
    if (PRTID(od->m1) != n_each) goto notConstEach;
    B c;
    if (!toConstant(od->f, &c)) goto notConstEach;
    
    u8 xe = TI(x,elType);
    u8 re = el_orSelf(xe, c);
    
    DirectArr r = toEltypeArr(x, re);
    if (isVal(c)) {
      if (wia==0) decG(c); // TODO could return x; fills?
      else incByG(c, wia-1);
    }
    
    usz xn = *SH(r.obj);
    bool ok;
    if (elInt(we)) {
      w = toI32Any(w); we = el_i32;
      i32* wp = i32any_ptr(w);
      ok = select_each_impl(r, re, c, xn, wp, wia, i64get_i32);
    } else {
      // annoying amount of code for el_f64 𝕨 vs ≤el_i32, but as el_f64 only should apply to ≥2⋆31-element arrays it shouldn't matter much
      assert(we==el_f64);
      f64* wp = f64any_ptr(w);
      ok = select_each_impl(r, re, c, xn, wp, wia, i64get_f64);
    }
    
    if (ok) {
      decG(w);
      return r.obj;
    } else {
      x = r.obj;
      goto bad;
    }
  } else {
    notConstEach:;
    rep = c1(o, C2(select, incG(w), incG(x)));
  }
  
  ur xr = RNK(x);
  ur wr = RNK(w);
  if (isAtm(rep) || xr+wr != RNK(rep)+1 || !eqShPart(SH(w),SH(rep),wr) || !eqShPart(SH(x)+1,SH(rep)+wr,xr-1)) {
    thrF("𝔽⌾(a⊸⊏)𝕩: 𝔽 must return an array with the same shape as its input (expected %0H, got %0H)", C2(select, w, x), rep);
  }
  
  usz csz = arr_csz(x);
  if (csz == 0) { decG(rep); decG(w); return x; }
  return select_replace(U'⊏', w, x, rep, wia, *SH(x), csz);
}
