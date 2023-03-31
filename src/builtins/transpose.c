// Transpose and Reorder Axes (⍉)

// Transpose
// One length-2 axis: dedicated code
//   Boolean: pdep for height 2; pext for width 2
//     SHOULD use a generic implementation if BMI2 not present
// SHOULD optimize other short lengths with pdep/pext and shuffles
// Boolean 𝕩: convert to integer
//   SHOULD have bit matrix transpose kernel
// CPU sizes: native or SIMD code
//   Large SIMD kernels used when they fit, overlapping for odd sizes
//     i8: 16×16; i16: 16×8; i32: 8×8; f64: 4×4
//   COULD use half-width or smaller kernels to improve odd sizes
//   Scalar transpose or loop used for overhang of 1

// Reorder Axes
// If 𝕨 indicates the identity permutation, return 𝕩
// Simplify: remove length-1 axes; coalesce adjacent and trailing axes
// Empty result or trivial reordering: reshape 𝕩
// Large cells: slow outer loop plus mut_copy
// CPU-sized cells, large last 𝕩 and result axes: strided 2D transposes
// Otherwise, generate indices and select with +⌜ and ⊏
//   SHOULD generate for a cell and virtualize the rest to save space
// COULD decompose axis permutations to use 2D transpose when possible
// COULD convert boolean to integer for some axis reorderings
// SHOULD have a small-subarray transposer using one or a few shuffles

//  ⍉⁼𝕩: data movement of ⍉ with different shape logic
// 𝕨⍉⁼𝕩: compute inverse 𝕨, length 1+⌈´𝕨
// Under Transpose supports invertible cases
//   SHOULD implement Under with duplicate axes, maybe as Under Select
// ⍉˘𝕩 and k⍉˘𝕩 for number k: convert to 0‿a⍉𝕩
//   SHOULD convert ⍉ with rank to a Reorder Axes call
// COULD implement fast ⍉⍟n

#include "../core.h"
#include "../utils/each.h"
#include "../utils/talloc.h"
#include "../builtins.h"
#include "../utils/calls.h"

#ifdef __BMI2__
  #include <immintrin.h>
  #if USE_VALGRIND
    #define _pdep_u64 vg_pdep_u64
  #endif
#endif

#define DECL_BASE(T) \
  static NOINLINE void transpose_##T(void* rv, void* xv, u64 bw, u64 bh, u64 w, u64 h) {     \
    T* rp=rv; T* xp=xv;                                                                      \
    PLAINLOOP for(usz y=0;y<bh;y++) NOVECTORIZE for(usz x=0;x<bw;x++) rp[x*h+y] = xp[y*w+x]; \
  }
DECL_BASE(i8) DECL_BASE(i16) DECL_BASE(i32) DECL_BASE(i64)
#undef DECL_BASE

typedef void (*TranspFn)(void*,void*,u64,u64,u64,u64);
#if SINGELI
  #define transposeFns simd_transpose
  #define SINGELI_FILE transpose
  #include "../utils/includeSingeli.h"
#else
  static TranspFn transposeFns[] = {
    transpose_i8, transpose_i16, transpose_i32, transpose_i64
  };
#endif


static void transpose_move(void* rv, void* xv, u8 xe, usz w, usz h) {
  assert(xe!=el_bit); assert(xe!=el_B);
  transposeFns[elWidthLogBits(xe)-3](rv, xv, w, h, w, h);
}
// Return an array with data from x transposed as though it's shape h,w
// Shape of result needs to be set afterwards!
static Arr* transpose_noshape(B* px, usz ia, usz w, usz h) {
  B x = *px;
  u8 xe = TI(x,elType);
  Arr* r;
  if (xe==el_B) {
    B xf = getFillR(x);
    B* xp = TO_BPTR(x);
    
    HArr_p p = m_harrUv(ia); // Debug build complains with harrUp
    transpose_move(p.a, xp, el_f64, w, h);
    for (usz xi=0; xi<ia; xi++) inc(p.a[xi]); // TODO don't inc when there's a method of freeing a HArr without freeing its elements
    NOGC_E;
    
    r=a(qWithFill(p.b, xf));
  } else if (xe==el_bit) {
    #ifdef __BMI2__
    if (h==2) {
      u32* x0 = (u32*)bitarr_ptr(x);
      u64* rp; r=m_bitarrp(&rp, ia);
      Arr* x1o = TI(x,slice)(inc(x),w,w);
      u32* x1 = (u32*) ((TyArr*)x1o)->a;
      for (usz i=0; i<BIT_N(ia); i++) rp[i] = _pdep_u64(x0[i], 0x5555555555555555) | _pdep_u64(x1[i], 0xAAAAAAAAAAAAAAAA);
      mm_free((Value*)x1o);
    } else if (w==2) {
      u64* xp = bitarr_ptr(x);
      u64* r0; r=m_bitarrp(&r0, ia);
      TALLOC(u64, r1, BIT_N(h));
      for (usz i=0; i<BIT_N(ia); i++) {
        u64 v = xp[i];
        ((u32*)r0)[i] = _pext_u64(v, 0x5555555555555555);
        ((u32*)r1)[i] = _pext_u64(v, 0xAAAAAAAAAAAAAAAA);
      }
      bit_cpy(r0, h, r1, 0, h);
      TFREE(r1);
    } else
    #endif
    {
      *px = x = taga(cpyI8Arr(x)); xe=el_i8;
      void* rv = m_tyarrp(&r,elWidth(xe),ia,el2t(xe));
      void* xv = tyany_ptr(x);
      transpose_move(rv, xv, xe, w, h);
      r = (Arr*)cpyBitArr(taga(r));
    }
  } else {
    void* rv = m_tyarrp(&r,elWidth(xe),ia,el2t(xe));
    void* xv = tyany_ptr(x);
    transpose_move(rv, xv, xe, w, h);
  }
  return r;
}

B transp_c1(B t, B x) {
  if (RARE(isAtm(x))) return m_atomUnit(x);
  ur xr = RNK(x);
  if (xr<=1) return x;
  
  usz ia = IA(x);
  usz* xsh = SH(x);
  usz h = xsh[0];
  if (ia==0 || h==1 || h==ia /*w==1*/) {
    Arr* r = cpyWithShape(x);
    ShArr* sh = m_shArr(xr);
    shcpy(sh->a, xsh+1, xr-1);
    sh->a[xr-1] = h;
    arr_shReplace(r, xr, sh);
    return taga(r);
  }
  usz w = xsh[1] * shProd(xsh, 2, xr);
  
  Arr* r = transpose_noshape(&x, ia, w, h);
  
  usz* rsh = arr_shAlloc(r, xr);
  if (xr==2) rsh[0] = w; else shcpy(rsh, SH(x)+1, xr-1);
  rsh[xr-1] = h;
  decG(x); return taga(r);
}

B mul_c2(B,B,B);
B ud_c1(B,B);
B tbl_c2(Md1D*,B,B);
B select_c2(B,B,B);

static void shSet(Arr* ra, ur rr, ShArr* sh) {
  if (RARE(rr <= 1)) arr_shVec(ra);
  else arr_shSetU(ra, rr, sh);
}

B transp_c2(B t, B w, B x) {
  usz wia=1;
  if (isArr(w)) {
    if (RNK(w)>1) thrM("⍉: 𝕨 must have rank at most 1");
    wia = IA(w);
    if (wia==0) { decG(w); return isArr(x)? x : m_atomUnit(x); }
  }
  ur xr;
  if (isAtm(x) || (xr=RNK(x))<wia) thrM("⍉: Length of 𝕨 must be at most rank of 𝕩");

  // Axis permutation
  TALLOC(u8, alloc, xr*(sizeof(ur) + 3*sizeof(usz))); // ur* p, usz* rsh, usz* st, usz* ri
  ur* p = (ur*)alloc;
  if (isAtm(w)) {
    usz a=o2s(w);
    if (a>=xr) thrF("⍉: Axis %s does not exist (%i≡=𝕩)", a, xr);
    if (a==xr-1) { TFREE(alloc); return C1(transp, x); }
    p[0] = a;
  } else {
    SGetU(w)
    for (usz i=0; i<wia; i++) {
      usz a=o2s(GetU(w, i));
      if (a>=xr) thrF("⍉: Axis %s does not exist (%i≡=𝕩)", a, xr);
      p[i] = a;
    }
    decG(w);
  }

  B r;

  // Compute shape for the given axes
  usz* xsh = SH(x);
  usz* rsh = (usz*)(p + xr); // Length xr
  usz dup = 0, max = 0, id = 0;
  usz no_sh = -(usz)1;
  for (usz j=0; j<xr; j++) rsh[j] = no_sh;
  for (usz i=0; i<wia; i++) {
    ur j=p[i];
    usz xl=xsh[i], l=rsh[j];
    dup += l!=no_sh;
    id += i==j;
    max = j>max? j : max;
    if (xl<l) rsh[j]=xl;
  }
  if (id == wia) { r = x; goto ret; }

  // Fill in remaining axes and check for missing ones
  ur rr = xr-dup;
  if (max >= rr) thrF("⍉: Skipped result axis");
  if (wia<xr) for (usz j=0, i=wia; j<rr; j++) if (rsh[j]==no_sh) {
    p[i] = j;
    rsh[j] = xsh[i];
    i++;
  }
  // Create shape object, saving unprocessed result shape
  ShArr* sh;
  if (LIKELY(rr > 1)) { // Not all duplicates
    sh = m_shArr(rr);
    shcpy(sh->a, rsh, rr);
  }

  // Empty result
  if (IA(x) == 0) {
    Arr* ra = m_fillarrpEmpty(getFillQ(x));
    shSet(ra, rr, sh);
    decG(x);
    r = taga(ra); goto ret;
  }

  // Add up stride for each axis
  ur na = max + 1;    // Number of result axes that moved
  usz* st = rsh + xr; // Length na
  for (usz j=0; j<na; j++) st[j] = 0;
  usz csz = shProd(xsh, na+dup, xr);
  for (usz i=na+dup, c=csz; i--; ) { st[p[i]]+=c; c*=xsh[i]; }

  // Simplify axis structure
  // p is unused now; work only on csz, rsh, and st
  usz *lp = &csz; usz sz = csz;
  usz na0=na; usz* rsh0=rsh; usz* st0=st; rsh+=na0; st+=na0; na=0;
  for (usz i=na0; i--; ) {
    usz l = rsh0[i]; if (l==1) continue;                    // Ignore
    usz s = st0[i]; if (s==sz) { *lp*=l; sz*=l; continue; } // Combine with lower
    na++; *--rsh=l; *--st=s; lp=rsh; sz=l*s;
  }
  // Turned out trivial
  if (na == 0) {
    Arr* ra = TI(x,slice)(x, 0, csz);
    shSet(ra, rr, sh);
    r = taga(ra); goto ret;
  }

  u8 xe = TI(x,elType);
  #define AXIS_LOOP(N_AX, I_INC, DO_INNER) \
    ur a0 = N_AX - 1;                                           \
    usz* ri = st+na; for (usz i=0; i<a0; i++) ri[i]=0;          \
    usz l = rsh[a0];                                            \
    for (usz i=0, j0=0;;) {                                     \
      /* Hardcode one innermost loop: assume N_AX>=1 */         \
      ur a = a0;                                                \
      usz str = st[a];                                          \
      for (usz k=0; k<l; k++) {                                 \
        usz j=j0+k*str; DO_INNER;                               \
        i += I_INC;                                             \
      }                                                         \
      if (i == ria) break;                                      \
      /* Update result index starting with last axis finished */\
      while (1) {                                               \
        str = st[--a];                                          \
        j0 += str;                                              \
        if (LIKELY(++ri[a] < rsh[a])) break;                    \
        ri[a] = 0;                                              \
        j0 -= rsh[a] * str;                                     \
      }                                                         \
    }
  u8 xlw = elWidthLogBits(xe);
  if (csz >= (32*8) >> xlw) { // cell >= 32 bytes
    usz ria = csz * shProd(rsh, 0, na);
    MAKE_MUT_INIT(rm, ria, xe); MUTG_INIT(rm);
    AXIS_LOOP(na, csz, mut_copyG(rm, i, x, j, csz));
    Arr* ra = mut_fp(rm);
    shSet(ra, rr, sh);
    r = withFill(taga(ra), getFillQ(x));
    decG(x); goto ret;
  }
  #undef AXIS_LOOP
  if ((csz & (csz-1))==0 && csz<=64>>xlw && csz<<xlw>=8  // CPU-sized cells
      && xe!=el_B && na>=2) {
    // If some result axis has stride 1 (guaranteed if dup==0), then it
    // corresponds to the last argument axis and we have a strided
    // transpose swapping that with the last result axis
    usz rai = na-1;
    usz xai=rai; while (st[--xai]!=1) if (xai==0) goto skip_2d;
    if (rsh[xai]*rsh[rai] < (256*8) >> xlw) goto skip_2d;
    TranspFn tran = transposeFns[CTZ(csz<<xlw)-3];
    usz rf = shProd(rsh, 0, na);
    Arr* ra;
    u8* rp = m_tyarrlbp(&ra,xlw,rf*csz,el2t(xe));
    u8* xp = tyany_ptr(x);
    usz w = rsh[xai]; usz ws = st[rai];
    usz h = rsh[rai]; usz hs = shProd(rsh, xai+1, rai) * h;
    if (na == 2) {
      tran(rp, xp, w, h, ws, hs);
    } else {
      csz = (csz<<xlw) / 8; // Convert to bytes
      usz i_skip = (w-1)*hs*csz;
      usz end = rf*csz - i_skip;
      ur a0 = na - 1;
      if      (xlw<3) for (usz i=0; i<na; i++) st[i] >>= 3-xlw;
      else if (xlw>3) for (usz i=0; i<na; i++) st[i] <<= xlw-3;
      usz* ri = st+na; for (usz i=0; i<a0; i++) ri[i]=0;
      for (usz i=0, j=0;;) {
        tran(rp+i, xp+j, w, h, ws, hs);
        i += h*csz;
        if (i == end) break;
        for (ur a = a0;;) {
          if (--a == xai) { assert(a!=0); i+=i_skip; --a; }
          usz str = st[a];
          j += str;
          if (LIKELY(++ri[a] < rsh[a])) break;
          ri[a] = 0;
          j -= rsh[a] * str;
        }
      }
    }
    shSet(ra, rr, sh);
    r = taga(ra);
    decG(x); goto ret;
  }
  skip_2d:;

  // Reshape x for selection
  ShArr* zsh = m_shArr(2);
  zsh->a[0] = IA(x)/csz;
  zsh->a[1] = csz;
  Arr* z = TI(x,slice)(x, 0, IA(x));
  arr_shSetU(z, 2, zsh);
  x = taga(z);
  // (+⌜´st×⟜↕¨rsh)⊏⥊𝕩
  B ind = bi_N;
  for (ur k=na; k--; ) {
    B v = C2(mul, m_f64(st[k]/csz), C1(ud, m_f64(rsh[k])));
    if (q_N(ind)) ind = v;
    else ind = M1C2(tbl, add, v, ind);
  }
  r = C2(select, ind, x);
  Arr* ra = cpyWithShape(r); r = taga(ra);
  if (rr>1) arr_shReplace(ra, rr, sh);
  else { decSh((Value*)ra); arr_shVec(ra); }

  ret:;
  TFREE(alloc);
  return r;
}


B transp_im(B t, B x) {
  if (isAtm(x)) thrM("⍉⁼: 𝕩 must not be an atom");
  ur xr = RNK(x);
  if (xr<=1) return x;
  
  usz ia = IA(x);
  usz* xsh = SH(x);
  usz w = xsh[xr-1];
  if (ia==0 || w==1 || w==ia /*h==1*/) {
    Arr* r = cpyWithShape(x);
    ShArr* sh = m_shArr(xr);
    sh->a[0] = w;
    shcpy(sh->a+1, xsh, xr-1);
    arr_shReplace(r, xr, sh);
    return taga(r);
  }
  usz h = xsh[0] * shProd(xsh, 1, xr-1);
  
  Arr* r = transpose_noshape(&x, ia, w, h);
  
  usz* rsh = arr_shAlloc(r, xr);
  rsh[0] = w;
  if (xr==2) rsh[1] = h; else shcpy(rsh+1, SH(x), xr-1);
  decG(x); return taga(r);
}

B transp_uc1(B t, B o, B x) {
  return transp_im(m_f64(0), c1(o,  transp_c1(t, x)));
}

// Consumes w; return bi_N if w contained duplicates
static B invert_transp_w(B w, ur xr) {
  if (isAtm(w)) {
    if (xr<1) thrM("⍉⁼: Length of 𝕨 must be at most rank of 𝕩");
    usz a=o2s(w);
    if (a>=xr) thrF("⍉⁼: Axis %s does not exist (%i≡=𝕩)", a, xr);
    i32* wp; w = m_i32arrv(&wp, a);
    PLAINLOOP for (usz i=0; i<a; i++) wp[i] = i+1;
  } else {
    if (RNK(w)>1) thrM("⍉⁼: 𝕨 must have rank at most 1");
    usz wia = IA(w);
    if (wia==0) return w;
    if (xr<wia) thrM("⍉⁼: Length of 𝕨 must be at most rank of 𝕩");
    SGetU(w)
    TALLOC(ur, p, xr);
    for (usz i=0; i<xr; i++) p[i]=xr;
    usz max = 0;
    for (usz i=0; i<wia; i++) {
      usz a=o2s(GetU(w, i));
      if (a>=xr) thrF("⍉⁼: Axis %s does not exist (%i≡=𝕩)", a, xr);
      if (p[a]!=xr) { TFREE(p); decG(w); return bi_N; } // Handled by caller
      max = a>max? a : max;
      p[a] = i;
    }
    decG(w);
    usz n = max+1;
    i32* wp; w = m_i32arrv(&wp, n);
    for (usz i=0, j=wia; i<n; i++) wp[i] = p[i]<xr? p[i] : j++;
    TFREE(p);
  }
  return w;
}

B transp_ix(B t, B w, B x) {
  if (isAtm(x)) thrM("⍉⁼: 𝕩 must not be an atom");
  w = invert_transp_w(w, RNK(x));
  if (q_N(w)) thrM("⍉⁼: Duplicate axes");
  return C2(transp, w, x);
}

B transp_ucw(B t, B o, B w, B x) {
  B wi = invert_transp_w(inc(w), isAtm(x)? 0 : RNK(x));
  if (q_N(wi)) return def_fn_ucw(t, o, w, x); // Duplicate axes
  return C2(transp, wi, c1(o, C2(transp, w, x)));
}

void transp_init(void) {
  c(BFn,bi_transp)->im = transp_im;
  c(BFn,bi_transp)->ix = transp_ix;
  c(BFn,bi_transp)->uc1 = transp_uc1;
  c(BFn,bi_transp)->ucw = transp_ucw;
}
