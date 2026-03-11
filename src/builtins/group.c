// Group and Group Indices (⊔)

// Group Indices: calls 𝕩⊔↕𝕩 for rank-1 flat 𝕩, otherwise self-hosted

// Group: native code for rank-1 𝕨 only, optimizations for integers
// SHOULD squeeze 𝕨
// All statistics computed in the initial pass that finds ⌈´𝕨
// If 𝕨 is boolean, compute from 𝕨¬⊸/𝕩 and 𝕨/𝕩
//   Boolean 𝕨 with any sort flag: binary search, take+drop
// COULD handle small-range 𝕨 with equals-replicate
// If 𝕨 is sorted up (flag or ∧´1↓»⊸<𝕨), count with Singeli and make slices
// If +´»⊸≠𝕨 is small, process in chunks as a separate case
//   If chunks never share indices, get each result element as a slice
// If +´𝕨<¯1 is large, filter out ¯1s.
//   COULD recompute statistics, may have enabled chunked or sorted code
// Remaining cases copy cells from 𝕩 individually
//   Converts 𝕨 to i32, COULD handle smaller types
//   CPU-sized cells handled quickly, 1-bit with bitp_get/set
//   SHOULD use memcpy and bit_cpy for other sizes
//   TRIED separating neg>0 and neg==0 loops, no effect

#include "../core.h"
#include "../utils/talloc.h"
#include "../utils/calls.h"
#include "../utils/mut.h"

#if SINGELI
  #define SINGELI_FILE group
  #include "../utils/includeSingeli.h"
#endif
#if SINGELI_SIMD
  // From slash.c
  extern usz (**const si_count_sorted)(u8*, usz*, usz*, void*, usz);
#endif

extern B shape_c1(B, B);
extern B ud_c1(B, B);
extern B ne_c2(B, B, B);
extern B slash_c1(B, B);
extern B slash_c2(B, B, B);
extern B select_c2(B, B, B);
extern B take_c2(B, B, B);
extern B drop_c2(B, B, B);

static Arr* arr_shChangeLen(Arr* a, ur r, usz* xsh, usz len) {
  assert(r > 1);
  usz* sh = a->sh = m_shArr(r)->a;
  SPRNK(a,r);
  sh[0] = len;
  shcpy(sh+1, xsh+1, r-1);
  return a;
}
static B m_shChangeLen(u8 xt, ur xr, usz* xsh, usz l, usz cw, usz csz) {
  return taga(arr_shChangeLen(m_arr(offsetof(TyArr, a)+l*cw, xt, l*csz), xr, xsh, l));
}
static void allocGroups(B* rp, usz ria, B z, u8 xt, ur xr, usz* xsh, usz* len, usz width, usz csz) {
  if (xr==1) for (usz j = 0; j < ria; j++) { usz l=len[j]; if (!l) rp[j] = incG(z); else m_tyarrv(rp+j, width, l, xt); }
  else       for (usz j = 0; j < ria; j++) { usz l=len[j]; rp[j] = !l ? incG(z) : m_shChangeLen(xt, xr, xsh, l, width, csz); }
}
static Arr* m_bitarr_nop(usz ia) { return m_arr(BITARR_SZ(ia), t_bitarr, ia); }
static void allocBitGroups(B* rp, usz ria, B z, ur xr, usz* xsh, usz* len, usz width) {
  if (xr==1) for (usz j = 0; j < ria; j++) { usz l=len[j]; rp[j] = !l ? incG(z) : taga(arr_shVec(m_bitarr_nop(l))); }
  else       for (usz j = 0; j < ria; j++) { usz l=len[j]; rp[j] = !l ? incG(z) : taga(arr_shChangeLen(m_bitarr_nop(l*width), xr, xsh, l)); }
}
static void allocEmptyGroups(B* rp, usz ria, B z, B x, ur xr, usz* xsh, usz* len) { // csz==0
  for (usz j = 0; j < ria; j++) { usz l=len[j]; rp[j] = !l ? incG(z) : taga(arr_shChangeLen(emptyArr(x, xr), xr, xsh, l)); }
}

// Integer list w
static B group_simple(B w, B x, ur xr, usz wia, usz xn, usz* xsh, u8 we) {
  i64 ria = 0;
  bool bad = false, sort = true;
  usz neg = 0, change = 0;
  void *wp0 = tyany_ptr(w);
  #if SINGELI
    #define ACCUM(T) \
      u8 bad_t, sort_t;                             \
      si_group_statistics_##T(wp0, xn, &bad_t, &neg, &sort_t, &change, &max); \
      bad = bad_t; sort = sort_t;
  #else
    #define ACCUM(T) \
      T prev = -1;                                  \
      for (usz i = 0; i < xn; i++) {                \
        T n = wp[i];                                \
        if (n>max) max = n;                         \
        bad |= n < -1;                              \
        neg += n == -1;                             \
        sort &= prev <= n;                          \
        change += prev != n;                        \
        prev = n;                                   \
      }
  #endif
  #if SINGELI_SIMD
    // We'll branch off before neg/change are used except neg==xn
    #define SORT_STATS neg = max==-1? xn : 0;
  #else
    #define SORT_STATS \
      while (neg<xn && wp[neg]==-1) neg++;          \
      change = 1 + max-wp[neg]; /* max possible */
  #endif
  #define CASE(T) case el_##T: { \
    T max = -1; T* wp = (T*)wp0;                    \
    if (FL_HAS(w,fl_asc) && xn>0) {                 \
      bad = wp[0] < -1;                             \
      max = wp[xn-1];                               \
      SORT_STATS                                    \
    } else { ACCUM(T) }                             \
    if (wia>xn) { ria=wp[xn]; bad|=ria<-1; }        \
    i64 m=(i64)max+1; if (m>ria) ria=m;             \
    break; }
  switch (we) { default:UD;
    CASE(i8) CASE(i16) CASE(i32)
    // Boolean w is special-cased before we would check sort or change
    case el_bit:
      if (xn==0) ria = wia? bitp_get(wp0,0) : 0;
      else {
        sort = FL_HAS(w,fl_dsc|fl_asc); // add dsc since it's easy to handle
        ria = sort? bitp_get(wp0, FL_HAS(w,fl_dsc)? 0 : xn-1)
                  : bit_has(wp0,xn,1);
        ria++;
      }
      break;
  }
  #undef CASE
  #undef ACCUM
  #undef SORT_STATS
  if (bad) thrM("𝕨⊔𝕩: 𝕨 can't contain elements less than ¯1");
  if (ria > (i64)(USZ_MAX)) thrOOM();
  
  Arr* r = m_fillarr0p(ria);
  B* rp = fillarrv_ptr(r);
  
  B xf = getFillR(x);
  Arr* rf = emptyWithFill(xf);
  if (xr==1) arr_shVec(rf); else arr_shChangeLen(rf, xr, xsh, 0);
  
  B z = taga(rf);
  fillarr_setFill(r, z);
  
  if (ria <= 1) {
    if (ria == 0) goto dec_ret; // Needed so wia>0
    if (neg == 0) { rp[0]=incG(x); goto dec_ret; }
    // else, 𝕨 is a mix of 0 and ¯1 (and maybe trailing 1)
  }
  if (we==el_bit) {
    assert(ria == 2);
    if (sort) {
      bool dn = FL_HAS(w,fl_dsc);
      u64 l = (dn? bit_boundary_dn : bit_boundary_up)(wp0, xn);
      decG(w);
      rp[0] = C2(take, m_f64(l), incG(x));
      rp[1] = C2(drop, m_f64(l), x);
      if (RARE(dn)) { B t=rp[0]; rp[0]=rp[1]; rp[1]=t; }
    } else {
      if (wia>xn) w = C2(take, m_f64(xn), w);
      rp[1] = C2(slash, incG(w), incG(x));
      rp[0] = C2(slash, bit_negate(w), x);
    }
    return taga(r);
  }
  // Needed to make sure wia>0 for ip[wia-1] below
  if (neg==xn) {
    FILL_TO(rp, el_B, 0, z, ria);
    goto dec_ret;
  }
  
  #if SINGELI_SIMD
  if (sort) {
    usz os = xn/128 + 1; // Possible overflow entries, plus sentinel
    usz nc = 1+ria;      // Number of counts, includes -1
    usz oa = 2*os*sizeof(usz);
    TALLOC(u8, buf, oa + nc);
    u8* lenm = buf + oa+1;                // Length mod 128
    usz* ov = (usz*)buf; usz* oc = ov+os; // Overflow values and counts
    memset(lenm-1, 0, nc);
    usz on = si_count_sorted[we-el_i8](lenm, ov, oc, wp0, xn);
    decG(w);
    ov[on] = ria;
    usz nx = 0; // Number of slices taken from x, for refcounting
    incByG(z,ria); incByG(x,ria);
    BSS2A slice = TI(x,slice);
    #define GROUP_SLICES(L, SHAPE) \
      for (usz i=lenm[-1], j=0, oi=0, e=ov[oi]; j<ria; j++) { \
        usz l = lenm[j];                                      \
        while (RARE(j==e)) { l+=oc[oi++]; e=ov[oi]; }         \
        if (!l) rp[j] = z;                                    \
        else {                                                \
          Arr* c=slice(x,i,L); SHAPE; rp[j]=taga(c);          \
          i+=L; nx++;                                         \
        }                                                     \
      }
    if (xr == 1) {
      GROUP_SLICES(l, arr_shVec(c))
    } else {
      usz csz = arr_csz(x);
      GROUP_SLICES(l*csz, arr_shChangeLen(c,xr,xsh,l))
    }
    #undef GROUP_SLICES
    incByG(z,-(i64)nx); incByG(x,-(i64)(ria-nx));
    TFREE(buf); decG(x);
    return taga(r);
  }
  #endif
  
  TALLOC(usz, pos, 2*ria+1); usz* len = pos+ria+1;
  memset(pos, 0, sizeof(usz)*(2*ria+1));
  
  bool notB = TI(x,elType) != el_B;
  u8 xt = arrNewType(TY(x));
  u8 xl = arrTypeBitsLog(TY(x));
  bool bits = xl == 0;
  u64 width = bits ? 1 : 1<<(xl-3); // cell width in bits if bits==1, bytes otherwise
  usz csz = 1;
  if (RARE(xr>1)) {
    width *= csz = arr_csz(x);
    usz cs = csz | (csz==0);
    xl += CTZ(cs);
    if (bits && xl>=3) { bits=0; width>>=3; }
    if ((cs & (cs-1)) || xl>7) xl = 7;
  }
  
  // Few changes in 𝕨: move in chunks
  if (xn>64 && notB && change<(xn*(width?width:1))/32) {
    u64* mp; B m = m_bitarrv(&mp, xn);
    u8* wp0 = tyany_ptr(w);
    we = TI(w,elType);
    CMP_AA_IMM(ne, we, mp, wp0-elWidth(we), wp0, xn);
    bitp_set(mp, 0, -1!=o2fG(IGetU(w,0)));
    
    B ind = C1(slash, m);
    w = C2(select, incG(ind), w);
    ind = toI32Any(ind);
    w = toI32Any(w);
    wia = IA(ind);
    
    i32* ip = i32any_ptr(ind);
    i32* wp = i32any_ptr(w);
    usz i0 = ip[0];
    for (usz i=0; i<wia-1; i++) ip[i] = ip[i+1]-ip[i];
    ip[wia-1] = xn-ip[wia-1];
    usz dup = 0; // Becomes nonzero if any group isn't a simple slice
    for (usz i = 0; i < wia; i++) {
      usz* p=len+wp[i]; usz l=*p; dup|=l; *p=l+ip[i];
    }
    
    void* xp = tyany_ptr(x);
    
    #define GROUP_CHUNKED(CPY) \
      for (u64 i=0, k=i0*width; i<wia; i++) {     \
        u64 k0 = k;                               \
        u64 l = ip[i]*width; k += l;              \
        i32 n = wp[i]; if (n<0) continue;         \
        CPY(tyarr_ptr(rp[n]), pos[n], xp, k0, l); \
        pos[n] += l;                              \
      }
    if (csz==0) {
      allocEmptyGroups(rp, ria, z, x, xr, xsh, len);
    } else if (bits) {
      allocBitGroups(rp, ria, z, xr, xsh, len, width);
      GROUP_CHUNKED(bit_cpyN)
    } else if (dup!=0) {
      allocGroups(rp, ria, z, xt, xr, xsh, len, width, csz);
      GROUP_CHUNKED(MEM_CPY)
    } else {
      // Every non-empty result element is a slice
      incByG(z,ria); for (usz j=0; j<ria; j++) rp[j]=z;
      incByG(x,ria); usz nx = 0;
      BSS2A slice = TI(x,slice);
      #define GROUP_CH_SLICES(CSZ, SHAPE) \
        for (u64 i=0, k=i0*CSZ; i<wia; i++) {          \
          u64 k0 = k;                                  \
          u64 l = ip[i], lc = l*CSZ; k += lc;          \
          i32 n = wp[i]; if (n<0) continue; nx++;      \
          Arr* c=slice(x,k0,lc); SHAPE; rp[n]=taga(c); \
        }
      if (xr==1) { GROUP_CH_SLICES(1,   arr_shVec(c)) }
      else       { GROUP_CH_SLICES(csz, arr_shChangeLen(c,xr,xsh,l)) }
      #undef GROUP_CH_SLICES
      incByG(z,-(i64)nx); incByG(x,-(i64)(ria-nx));
    }
    #undef GROUP_CHUNKED
    decG(ind);
    goto done;
  }
  
  // Many ¯1s: filter out, then continue
  if (xn>32 && neg > (bits? 0 : xn/16)) {
    if (wia>xn) w = C2(take, m_f64(xn), w);
    B m = C2(ne, m_f64(-1), incG(w));
    w = C2(slash, incG(m), w);
    x = C2(slash, m, x);
    xsh = SH(x);
    xn = *xsh;
    neg = 0;
  }
  w = toI32Any(w);
  i32* wp = i32any_ptr(w);
  for (usz i = 0; i < xn; i++) len[wp[i]]++; // overallocation makes this safe after n<-1 check
  
  u8 xk = xl - 3;
  if (csz==0) { // Empty cells, no movement needed
    allocEmptyGroups(rp, ria, z, x, xr, xsh, len);
  #if !SINGELI_SIMD // Dead code, handled above
  } else if (notB && sort) { // Sorted 𝕨, that is, partition 𝕩
    void* xp = tyany_ptr(x);
    u64 i=neg*width;
    #define GROUP_SORT(CPY, ALLOC) \
      for (usz j=0; j<ria; j++) {            \
        usz l = len[j];                      \
        if (!l) { rp[j]=incG(z); continue; } \
        ALLOC;                               \
        u64 lw = l*width;                    \
        CPY(tyarr_ptr(rp[j]), 0, xp, i, lw); \
        i += lw;                             \
      }
    if (!bits) {
      if (xr==1) GROUP_SORT(MEM_CPY, m_tyarrv(rp+j, width, l, xt))
      else       GROUP_SORT(MEM_CPY, rp[j] = m_shChangeLen(xt, xr, xsh, l, width, csz))
    } else {
      if (xr==1) GROUP_SORT(bit_cpyN, rp[j] = taga(arr_shVec(m_bitarr_nop(l))))
      else       GROUP_SORT(bit_cpyN, rp[j] = taga(arr_shChangeLen(m_bitarr_nop(l*width), xr, xsh, l)))
    }
    #undef GROUP_SORT
  #endif
  } else if (notB && xk <= 3) { // Cells of 𝕩 are CPU-sized
    void* xp = tyany_ptr(x);
    allocGroups(rp, ria, z, xt, xr, xsh, len, width, csz);
    switch(xk) { default: UD;
      case 0: for (usz i = 0; i < xn; i++) { i32 n = wp[i]; if (n>=0) ((u8* )tyarr_ptr(rp[n]))[pos[n]++] = ((u8* )xp)[i]; } break;
      case 1: for (usz i = 0; i < xn; i++) { i32 n = wp[i]; if (n>=0) ((u16*)tyarr_ptr(rp[n]))[pos[n]++] = ((u16*)xp)[i]; } break;
      case 2: for (usz i = 0; i < xn; i++) { i32 n = wp[i]; if (n>=0) ((u32*)tyarr_ptr(rp[n]))[pos[n]++] = ((u32*)xp)[i]; } break;
      case 3: for (usz i = 0; i < xn; i++) { i32 n = wp[i]; if (n>=0) ((u64*)tyarr_ptr(rp[n]))[pos[n]++] = ((u64*)xp)[i]; } break;
    }
  } else if (xl == 0) { // 1-bit cells
    u64* xp = bitany_ptr(x);
    allocBitGroups(rp, ria, z, xr, xsh, len, width);
    for (usz i = 0; i < xn; i++) {
      bool b = bitp_get(xp,i); i32 n = wp[i];
      if (n>=0) bitp_set(bitarr_ptr(rp[n]), pos[n]++, b);
    }
  } else { // Generic case
    for (usz i = 0; i < ria; i++) {
      usz l = len[i];
      Arr* c = m_fillarrp(l*csz);
      c->ia = 0;
      fillarr_setFill(c, inc(xf));
      NOGC_E; // ia=0 means that this is a "safe" array
      if (xr==1) arr_shVec(c); else arr_shChangeLen(c, xr, xsh, l);
      rp[i] = taga(c);
    }
    if (csz==0) goto done;
    SLOW2("𝕨⊔𝕩", w, x);
    SGet(x)
    NOGC_S; // ia==0 of the elements means they're in a sort of invalid state; though it should be fine as `x` references all the items that may be in them, so this NOGC isn't strictly necessary
    if (csz == 1) {
      for (usz i = 0; i < xn; i++) {
        i32 n = wp[i];
        if (n>=0) fillarrv_ptr(a(rp[n]))[pos[n]++] = Get(x, i);
      }
    } else {
      for (usz i = 0; i < xn; i++) {
        i32 n = wp[i];
        if (n<0) continue;
        usz p = (pos[n]++)*csz;
        B* rnp = fillarrv_ptr(a(rp[n])) + p;
        for (usz j = 0; j < csz; j++) rnp[j] = Get(x, i*csz + j);
      }
    }
    for (usz i = 0; i < ria; i++) a(rp[i])->ia = len[i]*csz;
    NOGC_E;
  }
  
  done:
  TFREE(pos);
  dec_ret:
  decG(w); decG(x);
  return taga(r);
}

extern GLOBAL B rt_group;
B group_c2(B t, B w, B x) {
  if (isAtm(x)) thrM("𝕨⊔𝕩: 𝕩 must be an array");
  ur xr = RNK(x), wr;
  if (isArr(w) && (wr=RNK(w))<=xr) {
    u8 we = TI(w,elType);
    if (!elInt(we)) w = squeeze_numTry(w, &we, SQ_INT);
    if (elInt(we)) {
      elint:;
      usz wia = IA(w);
      usz* xsh = SH(x);
      usz xn;
      if (wr == 1) {
        xn = *xsh;
        if (wia-(u64)xn > 1) thrF("𝕨⊔𝕩: ≠𝕨 must be either ≠𝕩 or one bigger (%s≡≠𝕨, %s≡≠𝕩)", wia, xn);
      } else {
        usz* wsh = SH(w);
        if (!eqShPart(wsh, xsh, wr)) thrF("𝕨⊔𝕩: Expected 𝕨's shape to be a prefix of 𝕩's (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x);
        w = C1(shape, w);
        xn = wia;
        // Combine first wr axes of x
        xr = 1 + xr-wr;
        ShArr* zsh ONLY_GCC(=0);
        if (xr != 1) {
          zsh = m_shArr(xr);
          zsh->a[0] = xn; shcpy(zsh->a+1, xsh+wr, xr-1);
        }
        Arr* z = TI(x,slice)(x, 0, IA(x));
        if (xr==1) arr_shVec(z); else arr_shSetUG(z, xr, zsh);
        x = taga(z);
        xsh = SH(x);
      }
      return group_simple(w, x, xr, wia, xn, xsh, we);
    }
    if (!elNum(we) && wr==1 && IA(w)==1) { // Check for ⟨int⟩⊔𝕩
      B w0 = IGetU(w,0);
      if (!isArr(w0) || !((wr=RNK(w0))<=xr)) goto base;
      incG(w0); we = TI(w0,elType);
      if (!elInt(we)) w0 = squeeze_numTry(w0, &we, SQ_INT);
      if (!elInt(we)) { decG(w0); goto base; }
      decG(w); w = w0;
      goto elint;
    }
  }
  base:
  return c2rt(group, w, x);
}
B group_c1(B t, B x) {
  if (isArr(x) && RNK(x)==1) {
    u8 we = TI(x,elType);
    if (!elInt(we)) x = squeeze_numTry(x, &we, SQ_INT);
    if (!elInt(we)) goto base;
    B range = C1(ud, m_f64(IA(x)));
    return C2(group, x, range);
  }
  base:
  return c1rt(group, x);
}
