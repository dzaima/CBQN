// Dyadic search functions: Member Of (∊), Index of (⊐), Progressive Index of (⊒)

// 𝕨⊐unit or unit∊𝕩: scalar loop with early-exit
//   SHOULD use simd
//   SHOULD unify implementations
// 𝕩⊒unit or 𝕨⊒𝕩 where 1≥≠𝕩: defer to 𝕨⊐𝕩

// Both arguments with rank≥1:
//   High-rank inputs:
//     Convert to a (lower-rank) typed integer array if cells are ≤62 bits
//     COULD have special hashing for equal type >64 bit cells, skipping squeezing
//     COULD try conditionally squeezing ahead-of-time, and not squeezing in bqn_hash
//   p⊐n & n∊p with short p & long n: n⊸=¨ p
//     bitarr⊐𝕩: more special arithmetic
//     SHOULD have impls for long p & short n
//   ≤16-bit elements: lookup tables
//   Character elements: reinterpret as integer elements
//   Otherwise, generic hashtable
//     SHOULD handle up to 64 bit cells via proper typed hash tables
//   SHOULD have fast path when cell sizes or element types doesn't match
//   SHOULD properly handle ¯0

#include "../core.h"
#include "../utils/hash.h"
#include "../utils/talloc.h"
#include "../utils/calls.h"

#define C2i(F, W, X) C2(F, m_i32(W), X)
extern B eq_c2(B,B,B);
extern B ne_c2(B,B,B);
extern B or_c2(B,B,B);
extern B add_c2(B,B,B);
extern B sub_c2(B,B,B);
extern B mul_c2(B,B,B);

static u64 elRange(u8 eltype) { return 1ull<<(1<<elWidthLogBits(eltype)); }

#define TABLE(IN, FOR, TY, INIT, SET) \
  usz it = elRange(IN##e);   /* Range of writes        */                    \
  usz ft = elRange(FOR##e);  /* Range of lookups       */                    \
  usz t = it>ft? it : ft;    /* Table allocation width */                    \
  TALLOC(TY, tab0, t); TY* tab = tab0 + t/2;                                 \
  usz m=IN##ia, n=FOR##ia;                                                   \
  void* ip = tyany_ptr(IN);                                                  \
  void* fp = tyany_ptr(FOR);                                                 \
  /* Initialize */                                                           \
  if (IN.u != FOR.u) {                                                       \
    if (FOR##e==el_i16 && n<ft/(64/sizeof(TY)))                              \
         { for (usz i=0; i<n; i++) tab[((i16*)fp)[i]]=INIT; }                \
    else { TY* to=tab-(ft/2-(ft==2)); for (i64 i=0; i<ft; i++) to[i]=INIT; } \
  }                                                                          \
  /* Set */                                                                  \
  if (IN##e==el_i8) { for (usz i=m; i--;    ) tab[((i8 *)ip)[i]]=SET; }      \
  else              { for (usz i=m; i--;    ) tab[((i16*)ip)[i]]=SET; }      \
  decG(IN);                                                                  \
  /* Lookup */                                                               \
  if (FOR##e==el_bit) {                                                      \
    r = bit_sel(FOR, m_i32(tab[0]), m_i32(tab[1]));                          \
  } else {                                                                   \
    TY* rp; r = m_##TY##arrc(&rp, FOR);                                      \
    if (FOR##e==el_i8){ for (usz i=0; i<n; i++) rp[i]=tab[((i8 *)fp)[i]]; }  \
    else              { for (usz i=0; i<n; i++) rp[i]=tab[((i16*)fp)[i]]; }  \
    decG(FOR);                                                               \
  }                                                                          \
  TFREE(tab0);

typedef struct { B n, p; } B2;
static NOINLINE B toIntCell(B x, ux csz0, ur co) {
  assert(TI(x,elType)!=el_B);
  usz ria = shProd(SH(x), 0, co);
  ShArr* rsh;
  if (co>1) { rsh=m_shArr(co); shcpy(rsh->a,SH(x),co); }
  B r0 = widenBitArr(x, co);
  usz csz = shProd(SH(r0),co,RNK(r0)) << elWidthLogBits(TI(r0,elType));
  u8 t;
  if      (csz==8)  t = t_i8slice;
  else if (csz==16) t = t_i16slice;
  else if (csz==32) t = t_i32slice;
  else if (csz==64) t = t_f64slice;
  else UD;
  TySlice* r = m_arr(sizeof(TySlice), t, ria);
  r->p = a(r0);
  r->a = tyany_ptr(r0);
  if (co>=1) arr_shSetU((Arr*)r, co, rsh);
  else arr_shVec((Arr*)r);
  return taga(r);
}
static NOINLINE B cpyToElLog(B x, u8 xe, u8 lb) {
  switch(lb) { default: UD;
    case 0: return taga(cpyBitArr(x));
    case 3: return taga(elNum(xe)? cpyI8Arr(x) : cpyC8Arr(x));
    case 4: return taga(elNum(xe)? cpyI16Arr(x) : cpyC16Arr(x));
    case 5: return taga(elNum(xe)? cpyI32Arr(x) : cpyC32Arr(x));
    case 6: return taga(cpyF64Arr(x));
  }
}
static NOINLINE B2 splitCells(B n, B p, u8 mode) { // 0:∊ 1:⊐ 2:⊒
  #define SYMB (mode==0? "∊" : mode==1? "⊐" : "⊒")
  #define ARG_N (mode? "𝕩" : "𝕨")
  #define ARG_P (mode? "𝕨" : "𝕩")
  if (isAtm(p) || RNK(p)==0) thrF("%U: %U cannot have rank 0", SYMB, ARG_P);
  ur pr = RNK(p);
  if (isAtm(n)) n = m_atomUnit(n);
  ur nr = RNK(n);
  if (nr < pr-1) thrF("%U: Rank of %U must be at least the cell rank of %U (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", SYMB, ARG_N, ARG_P, mode?p:n, mode?n:p);
  ur pcr = pr-1;
  ur nco = nr-pcr;
  if (nco>0 && eqShPart(SH(n)+nco, SH(p)+1, pcr)) {
    u8 ne = TI(n,elType);
    u8 pe = TI(p,elType);
    if (ne<el_B && pe<el_B && elNum(ne)==elNum(pe)) {
      usz csz = arr_csz(p);
      u8 neb = elWidthLogBits(ne);
      u8 peb = elWidthLogBits(pe);
      u8 meb = neb>peb? neb : peb;
      ux rb = csz<<meb;
      if (rb!=0 && rb<=62) {
        if (n.u == p.u) { decG(p); n=toIntCell(n,rb,1); return (B2){.n=n, .p=incG(n)}; }
        if      (neb!=meb) n = cpyToElLog(n, ne, meb);
        else if (peb!=meb) p = cpyToElLog(p, pe, meb);
        return (B2){.n=toIntCell(n,rb,nco), .p=toIntCell(p,rb,1)};
      }
    }
  }
  return (B2){.n=toKCells(n,nco), .p=toCells(p)};
  
  #undef ARG_N
  #undef ARG_P
  #undef SYMB
}

static B reduceI32Width(B r, usz count) {
  return count<=I8_MAX? taga(cpyI8Arr(r)) : count<=I16_MAX? taga(cpyI16Arr(r)) : r;
}

B indexOf_c2(B t, B w, B x) {
  if (RARE(!isArr(w) || RNK(w)!=1)) {
    B2 t = splitCells(x, w, 1);
    w = t.p;
    x = t.n;
  }
  if (!isArr(x) || RNK(x)==0) {
    usz wia = IA(w);
    B el = isArr(x)? IGetU(x,0) : x;
    i32 res = wia;
    u8 we = TI(w,elType);
    if (we<el_B) {
      void* wp = tyany_ptr(w);
      u8 v8; u16 v16; u32 v32; f64 v64f;
      switch(we) { default: UD;
        case el_bit: if (!q_bit(el)) goto notfound; res = bit_find(wp,wia,o2bG(el)); goto checked;
        case el_i8:  if (!q_i8 (el)) goto notfound;  v8  = ( u8)( i8)o2iG(el); goto chk8;
        case el_i16: if (!q_i16(el)) goto notfound;  v16 = (u16)(i16)o2iG(el); goto chk16;
        case el_i32: if (!q_i32(el)) goto notfound;  v32 = (u32)(i32)o2iG(el); goto chk32;
        case el_f64: if (!q_f64(el)) goto notfound;  v64f=           o2fG(el); goto chk64f;
        case el_c8:  if (!q_c8 (el)) goto notfound;  v8  = ( u8)     o2cG(el); goto chk8;
        case el_c16: if (!q_c16(el)) goto notfound;  v16 = (u16)     o2cG(el); goto chk16;
        case el_c32: if (!q_c32(el)) goto notfound;  v32 = (u32)     o2cG(el); goto chk32;
      }
      
      chk8:   for (usz i = 0; i < wia; i++) if ((( u8*)wp)[i]== v8 ) { res=i; break; } goto checked;
      chk16:  for (usz i = 0; i < wia; i++) if (((u16*)wp)[i]==v16 ) { res=i; break; } goto checked;
      chk32:  for (usz i = 0; i < wia; i++) if (((u32*)wp)[i]==v32 ) { res=i; break; } goto checked;
      chk64f: for (usz i = 0; i < wia; i++) if (((f64*)wp)[i]==v64f) { res=i; break; } goto checked;
    } else {
      SGetU(w)
      for (usz i = 0; i < wia; i++) if (equal(GetU(w,i), el)) { res = i; goto checked; }
    }
    
    checked:; notfound:;
    decG(w); dec(x);
    B r = m_vec1(m_f64(res));
    arr_shAtm(a(r)); // replaces shape
    return r;
  } else {
    u8 we = TI(w,elType); usz wia = IA(w);
    u8 xe = TI(x,elType); usz xia = IA(x);
    if (wia == 0) { B r=taga(arr_shCopy(allZeroes(xia), x)); decG(w); decG(x); return r; }
    
    if (elNum(we) && elNum(xe)) { tyEls:
      if (we==el_bit) {
        u64* wp = bitarr_ptr(w);
        u64 w0 = 1 & wp[0];
        u64 i = bit_find(wp, wia, !w0); decG(w);
        if (i!=wia) incG(x);
        B r =                         C2i(mul, wia  , C2i(ne,  w0, x)) ;
        return i==wia? r : C2(sub, r, C2i(mul, wia-i, C2i(eq, !w0, x)));
      }
      
      if (wia<=(we<=el_i16?4:16) && xia>16) {
        SGetU(w);
        #define XEQ(I) C2(ne, GetU(w,I), incG(x))
        B r = XEQ(wia-1);
        for (usz i=wia-1; i--; ) r = C2(mul, XEQ(i), C2i(add, 1, r));
        #undef XEQ
        decG(w); decG(x); return r;
      }
      
      if (xia+wia>20 && we<=el_i16 && xe<=el_i16) {
        B r;
        TABLE(w, x, i32, wia, i)
        return reduceI32Width(r, wia);
      }
    } else if (elChr(we) && elChr(xe)) { we-= el_c8-el_i8; xe-= el_c8-el_i8; goto tyEls; }
    
    i32* rp; B r = m_i32arrc(&rp, x);
    H_b2i* map = m_b2i(64);
    SGetU(x)
    SGetU(w)
    for (usz i = 0; i < wia; i++) {
      bool had; u64 p = mk_b2i(&map, GetU(w,i), &had);
      if (!had) map->a[p].val = i;
    }
    for (usz i = 0; i < xia; i++) rp[i] = getD_b2i(map, GetU(x,i), wia);
    free_b2i(map); decG(w); decG(x);
    return reduceI32Width(r, wia);
  }
}

B enclosed_0, enclosed_1;
B memberOf_c2(B t, B w, B x) {
  if (isAtm(x) || RNK(x)!=1) {
    B2 t = splitCells(w, x, false);
    w = t.n;
    x = t.p;
  }
  if (isAtm(w)) goto single;
  
  ur wr = RNK(w);
  if (wr>0) goto many;
  
  B w0 = IGet(w, 0);
  dec(w);
  w = w0;
  goto single;
  
  B r;
  single: {
    usz xia = IA(x);
    SGetU(x)
    for (usz i = 0; i < xia; i++) if (equal(GetU(x, i), w)) { r = incG(enclosed_1); goto dec_wx; }
    r = incG(enclosed_0);
    dec_wx:; dec(w);
    goto dec_x;
  }
  
  
  many: {
    u8 we = TI(w,elType); usz wia = IA(w);
    u8 xe = TI(x,elType); usz xia = IA(x);
    if (xia == 0) { r=taga(arr_shCopy(allZeroes(wia), w)); decG(w); goto dec_x; }
    
    if (elNum(we) && elNum(xe)) { tyEls:
      #define WEQ(V) C2(eq, incG(w), V)
      if (xe==el_bit) {
        u64* xp = bitarr_ptr(x);
        u64 x0 = 1 & xp[0];
        r = WEQ(m_usz(x0));
        if (bit_has(xp, xia, !x0)) r = C2(or, r, WEQ(m_usz(!x0)));
        decG(w); goto dec_x;
      }
      
      if (xia<=(xe==el_i16?8:16) && wia>16) {
        SGetU(x);
        r = WEQ(GetU(x,0));
        for (usz i=1; i<xia; i++) r = C2(or, r, WEQ(GetU(x,i)));
        decG(w); goto dec_x;
      }
      #undef WEQ
      
      if (xia+wia>20 && we<=el_i16 && xe<=el_i16) {
        B r;
        TABLE(x, w, i8, 0, 1)
        return taga(cpyBitArr(r));
      }
    } else if (elChr(we) && elChr(xe)) { we-= el_c8-el_i8; xe-= el_c8-el_i8; goto tyEls; }
    
    H_Sb* set = m_Sb(64);
    SGetU(x) SGetU(w)
    bool had;
    for (usz i = 0; i < xia; i++) mk_Sb(&set, GetU(x,i), &had);
    u64* rp; r = m_bitarrc(&rp, w);
    for (usz i = 0; i < wia; i++) bitp_set(rp, i, has_Sb(set, GetU(w,i)));
    free_Sb(set); decG(w);
    goto dec_x;
  }
  
  dec_x:;
  decG(x);
  return r;
}

B count_c2(B t, B w, B x) {
  if (RARE(!isArr(w) || RNK(w)!=1)) {
    B2 t = splitCells(x, w, 2);
    w = t.p;
    x = t.n;
  }
  
  if (!isArr(x) || IA(x)<=1) return indexOf_c2(m_f64(0), w, x);
  u8 we = TI(w,elType); usz wia = IA(w);
  u8 xe = TI(x,elType); usz xia = IA(x);
  i32* rp; B r = m_i32arrc(&rp, x);
  TALLOC(usz, wnext, wia+1);
  wnext[wia] = wia;
  if (we<=el_i16 && xe<=el_i16) {
    if (we==el_bit) { w = toI8Any(w); we = TI(w,elType); }
    if (xe==el_bit) { x = toI8Any(x); xe = TI(x,elType); }
    el8or16:;
    usz it = elRange(we);    // Range of writes
    usz ft = elRange(xe);    // Range of lookups
    usz t = it>ft? it : ft;  // Table allocation width
    TALLOC(i32, tab0, t); i32* tab = tab0 + t/2;
    usz m=wia, n=xia;
    void* ip = tyany_ptr(w);
    void* fp = tyany_ptr(x);
    // Initialize
    if (xe==el_i16 && n<ft/(64/sizeof(i32)))
         { for (usz i=0; i<n; i++) tab[((i16*)fp)[i]]=wia; }
    else { for (i64 i=0; i<ft; i++) tab[i-ft/2]=wia; }
    // Set
    #define SET(T) for (usz i=m; i--; ) { i32* p=tab+((T*)ip)[i]; wnext[i]=*p; *p=i; }
    if (we==el_i8) { SET(i8) } else { SET(i16) }
    #undef SET
    // Lookup
    #define GET(T) for (usz i=0; i<n; i++) { i32* p=tab+((T*)fp)[i]; *p=wnext[rp[i]=*p]; }
    if (xe==el_i8) { GET(i8) } else { GET(i16) }
    #undef GET
    TFREE(tab0);
  } else if (we>=el_c8 && we<=el_c16 && xe>=el_c8 && xe<=el_c16) {
    we-= el_c8-el_i8; xe-= el_c8-el_i8;
    goto el8or16;
  } else {
    H_b2i* map = m_b2i(64);
    SGetU(x)
    SGetU(w)
    for (usz i = wia; i--; ) {
      bool had; u64 p = mk_b2i(&map, GetU(w,i), &had);
      wnext[i] = had ? map->a[p].val : wia;
      map->a[p].val = i;
    }
    for (usz i = 0; i < xia; i++) {
      bool had; u64 p = getQ_b2i(map, GetU(x,i), &had);
      usz j = wia;
      if (had) { j = map->a[p].val; map->a[p].val = wnext[j]; }
      rp[i] = j;
    }
    free_b2i(map);
  }
  TFREE(wnext); decG(w); decG(x);
  return reduceI32Width(r, wia);
}


void search_init(void) {
  { u64* p; Arr* a=m_bitarrp(&p, 1); arr_shAtm(a); *p= 0;    gc_add(enclosed_0=taga(a)); }
  { u64* p; Arr* a=m_bitarrp(&p, 1); arr_shAtm(a); *p=~0ULL; gc_add(enclosed_1=taga(a)); }
}
