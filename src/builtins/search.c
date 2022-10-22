#include "../core.h"
#include "../utils/hash.h"
#include "../utils/mut.h"
#include "../utils/talloc.h"

#define C2(F,X,W) F##_c2(m_f64(0),X,W)
#define C2i(F, W, X) C2(F, m_i32(W), X)
extern B eq_c2(B,B,B);
extern B ne_c2(B,B,B);
extern B or_c2(B,B,B);
extern B sub_c2(B,B,B);
extern B mul_c2(B,B,B);

#define TABLE(IN, FOR, TY, INIT, SET) \
  usz it = 1<<(1<<elWidthLogBits(IN##e));  /* Range of writes        */     \
  usz ft = 1<<(1<<elWidthLogBits(FOR##e)); /* Range of lookups       */     \
  usz t = it>ft? it : ft;                  /* Table allocation width */     \
  TALLOC(TY, tab0, t); TY* tab = tab0 + t/2;                                \
  usz m=IN##ia, n=FOR##ia;                                                  \
  void* ip = tyany_ptr(IN);                                                 \
  void* fp = tyany_ptr(FOR);                                                \
  /* Initialize */                                                          \
  if (FOR##e==el_i16 && n<ft/(64/sizeof(TY)))                               \
       { for (usz i=0; i<n; i++) tab[((i16*)fp)[i]]=INIT; }                 \
  else { TY* to=tab-(ft/2-(ft==2)); for (i64 i=0; i<ft; i++) to[i]=INIT; }  \
  /* Set */                                                                 \
  if (IN##e==el_i8) { for (usz i=m; i--;    ) tab[((i8 *)ip)[i]]=SET; }     \
  else              { for (usz i=m; i--;    ) tab[((i16*)ip)[i]]=SET; }     \
  decG(IN);                                                                 \
  /* Lookup */                                                              \
  if (FOR##e==el_bit) {                                                     \
    r = bit_sel(FOR, m_i32(tab[0]), m_i32(tab[1]));                         \
  } else {                                                                  \
    TY* rp; r = m_##TY##arrc(&rp, FOR);                                     \
    if (FOR##e==el_i8){ for (usz i=0; i<n; i++) rp[i]=tab[((i8 *)fp)[i]]; } \
    else              { for (usz i=0; i<n; i++) rp[i]=tab[((i16*)fp)[i]]; } \
    decG(FOR);                                                              \
  }                                                                         \
  TFREE(tab0);

extern B rt_indexOf;
B indexOf_c2(B t, B w, B x) {
  if (!isArr(w) || RNK(w)==0) thrM("⊐: 𝕨 must have rank at least 1");
  if (RNK(w)==1) {
    if (!isArr(x) || RNK(x)==0) {
      usz wia = IA(w);
      B el = isArr(x)? IGetU(x,0) : x;
      i32 res = wia;
      if (TI(w,elType)==el_i32) {
        if (q_i32(el)) {
          i32* wp = i32any_ptr(w);
          i32 v = o2iG(el);
          for (usz i = 0; i < wia; i++) {
            if (wp[i] == v) { res = i; break; }
          }
        }
      } else {
        SGetU(w)
        for (usz i = 0; i < wia; i++) {
          if (equal(GetU(w,i), el)) { res = i; break; }
        }
      }
      decG(w); dec(x);
      i32* rp; Arr* r = m_i32arrp(&rp, 1);
      arr_shAlloc(r, 0);
      rp[0] = res;
      return taga(r);
    } else {
      u8 we = TI(w,elType); usz wia = IA(w);
      u8 xe = TI(x,elType); usz xia = IA(x);
      if (we==el_bit) {
        u64* wp = bitarr_ptr(w);
        u64 w0 = 1 & wp[0];
        u64 i = bit_find(wp, wia, !w0); decG(w);
        B r =                         C2i(mul, wia  , C2i(ne,  w0, inc(x)));
        return i==wia? r : C2(sub, r, C2i(mul, wia-i, C2i(eq, !w0, x)));
      }
      // TODO O(wia×xia) for small wia or xia
      if (xia+wia>20 && we<=el_i16 && xe<=el_i16) {
        B r;
        TABLE(w, x, i32, wia, i)
        return r;
      }
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
      return wia<=I8_MAX? taga(cpyI8Arr(r)) : wia<=I16_MAX? taga(cpyI16Arr(r)) : r;
    }
  }
  return c2(rt_indexOf, w, x);
}

B enclosed_0;
B enclosed_1;
extern B rt_memberOf;
B memberOf_c2(B t, B w, B x) {
  if (isAtm(x) || RNK(x)!=1) goto bad;
  if (isAtm(w)) goto single;
  ur wr = RNK(w);
  if (wr==0) {
    B w0 = IGet(w, 0);
    dec(w);
    w = w0;
    goto single;
  }
  if (wr==1) goto many;
  goto bad;
  
  bad: return c2(rt_memberOf, w, x);
  
  B r;
  single: {
    usz xia = IA(x);
    SGetU(x)
    for (usz i = 0; i < xia; i++) if (equal(GetU(x, i), w)) { r = inc(enclosed_1); goto dec_wx; }
    r = inc(enclosed_0);
    dec_wx:; dec(w);
    goto dec_x;
  }
  
  
  many: {
    u8 we = TI(w,elType); usz wia = IA(w);
    u8 xe = TI(x,elType); usz xia = IA(x);
    if (xia == 0) { Arr* ba=allZeroes(wia); arr_shVec(ba); r=taga(ba); decG(w); goto dec_x; }
    #define WEQ(V) C2(eq, inc(w), V)
    if (xe==el_bit) {
      u64* xp = bitarr_ptr(x);
      u64 x0 = 1 & xp[0];
      r = WEQ(m_usz(x0));
      if (bit_has(xp, xia, !x0)) r = C2(or, r, WEQ(m_usz(!x0)));
      decG(w); goto dec_x;
    }
    if (xia<=16 && wia>16 && we<el_B && xe<el_B) {
      SGetU(x);
      r = WEQ(GetU(x,0));
      for (usz i=1; i<xia; i++) r = C2(or, r, WEQ(GetU(x,i)));
      decG(w); goto dec_x;
    }
    #undef WEQ
    // TODO O(wia×xia) for small wia
    if (xia+wia>20 && we<=el_i16 && xe<=el_i16) {
      B r;
      TABLE(x, w, i8, 0, 1)
      return num_squeeze(r);
    }
    H_Sb* set = m_Sb(64);
    SGetU(x) SGetU(w)
    bool had;
    for (usz i = 0; i < xia; i++) mk_Sb(&set, GetU(x,i), &had);
    u64* rp; r = m_bitarrv(&rp, wia);
    for (usz i = 0; i < wia; i++) bitp_set(rp, i, has_Sb(set, GetU(w,i)));
    free_Sb(set); decG(w);
    goto dec_x;
  }
  
  dec_x:;
  decG(x);
  return r;
}

extern B rt_count;
B count_c2(B t, B w, B x) {
  if (!isArr(w) || RNK(w)==0) thrM("⊒: 𝕨 must have rank at least 1");
  if (RNK(w)==1) {
    if (!isArr(x) || IA(x)<=1) return indexOf_c2(m_f64(0), w, x);
    usz wia = IA(w);
    usz xia = IA(x);
    i32* rp; B r = m_i32arrc(&rp, x);
    TALLOC(usz, wnext, wia+1);
    H_b2i* map = m_b2i(64);
    SGetU(x)
    SGetU(w)
    wnext[wia] = wia;
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
    TFREE(wnext); free_b2i(map); decG(w); decG(x);
    return wia<=I8_MAX? taga(cpyI8Arr(r)) : wia<=I16_MAX? taga(cpyI16Arr(r)) : r;
  }
  return c2(rt_count, w, x);
}


void search_gcFn() {
  mm_visit(enclosed_0);
  mm_visit(enclosed_1);
}

void search_init() {
  gc_addFn(search_gcFn);
  { u64* p; Arr* a=m_bitarrp(&p, 1); arr_shAlloc(a,0); *p= 0;    enclosed_0=taga(a); }
  { u64* p; Arr* a=m_bitarrp(&p, 1); arr_shAlloc(a,0); *p=~0ULL; enclosed_1=taga(a); }
}
