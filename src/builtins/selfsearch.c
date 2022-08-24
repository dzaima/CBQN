#include "../core.h"
#include "../utils/hash.h"
#include "../utils/mut.h"
#include "../utils/talloc.h"

B memberOf_c1(B t, B x) {
  if (isAtm(x) || RNK(x)==0) thrM("∊: Argument cannot have rank 0");
  if (RNK(x)>1) x = toCells(x);
  usz xia = IA(x);
  if (xia==0) { decG(x); return emptyIVec(); }
  u8 xe = TI(x,elType);

  #define LOOKUP(T) \
    usz tn = 1<<T, n = xia;                                            \
    u##T* xp = (u##T*)i##T##any_ptr(x);                                \
    i8* rp; B r = m_i8arrv(&rp, n);                                    \
    TALLOC(u8, tab, tn);                                               \
    for (usz j=0; j<tn; j++) tab[j]=1;                                 \
    for (usz i=0; i<n;  i++) { u##T j=xp[i]; rp[i]=tab[j]; tab[j]=0; } \
    decG(x); TFREE(tab);                                               \
    return num_squeeze(r)
  if (xia>=16 && xe==el_i8) { LOOKUP(8); }
  if (xia>=256 && xe==el_i16) { LOOKUP(16); }
  #undef LOOKUP
  // Radix-assisted lookup
  if (xia>=256 && xe==el_i32) {
    usz rx = 256, tn = 1<<16; // Radix; table length
    usz n = xia;
    u32* v0 = (u32*)i32any_ptr(x);
    i8* r0; B r = m_i8arrv(&r0, n);

    TALLOC(u8, alloc, 9*n+(tn+2*rx*sizeof(usz)));
    // Allocations                    count radix hash deradix
    usz *c0 = (usz*)(alloc);   // rx    X-----------------X
    usz *c1 = (usz*)(c0+rx);   // rx    X-----------------X
    u8  *k0 = (u8 *)(c1+rx);   //  n    X-----------------X
    u8  *k1 = (u8 *)(k0+n);    //            --/----------X
    u32 *v1 = (u32*)(k1);      //  n         X-X
    u32 *v2 = (u32*)(v1+n);    //  n           X----------X
    u8  *r2 = (u8 *)(k1+n);
    u8  *r1 = (u8 *)(r2+n);
    u8  *tab= (u8 *)(v2+n);    // tn

    // Count keys
    for (usz j=0; j<2*rx; j++) c0[j] = 0;
    for (usz i=0; i<n; i++) { u32 v=v0[i]; c0[(u8)(v>>24)]++; c1[(u8)(v>>16)]++; }
    // Exclusive prefix sum
    usz s0=0, s1=0;
    for (usz j=0; j<rx; j++) {
      usz p0 = s0,  p1 = s1;
      s0 += c0[j];  s1 += c1[j];
      c0[j] = p0;   c1[j] = p1;
    }
    // Radix moves
    for (usz i=0; i<n; i++) { u32 v=v0[i]; u8 k=k0[i]=(u8)(v>>24); usz c=c0[k]++; v1[c]=v; }
    for (usz i=0; i<n; i++) { u32 v=v1[i]; u8 k=k1[i]=(u8)(v>>16); usz c=c1[k]++; v2[c]=v; }
    // Table lookup
    for (usz j=0; j<tn; j++) tab[j]=1;
    u32 t0=v2[0]>>16; usz e=0;
    for (usz i=0; i<n; i++) {
      u32 v=v2[i], tv=v>>16;
      // Clear table when top bytes change
      if (RARE(tv!=t0)) { for (; e<i; e++) tab[(u16)v2[e]]=1; t0=tv; }
      u32 j=(u16)v; r2[i]=tab[j]; tab[j]=0;
    }
    // Radix unmoves
    memmove(c0+1, c0, (2*rx-1)*sizeof(usz)); c0[0]=c1[0]=0;
    for (usz i=0; i<n; i++) { r1[i]=r2[c1[k1[i]]++]; }
    for (usz i=0; i<n; i++) { r0[i]=r1[c0[k0[i]]++]; }
    decG(x); TFREE(alloc);
    return num_squeeze(r);
  }
  
  u64* rp; B r = m_bitarrv(&rp, xia);
  H_Sb* set = m_Sb(64);
  SGetU(x)
  for (usz i = 0; i < xia; i++) bitp_set(rp, i, !ins_Sb(&set, GetU(x,i)));
  free_Sb(set); decG(x);
  return r;
}

B count_c1(B t, B x) {
  if (isAtm(x) || RNK(x)==0) thrM("⊒: Argument cannot have rank 0");
  if (RNK(x)>1) x = toCells(x);
  usz xia = IA(x);
  if (xia==0) { decG(x); return emptyIVec(); }
  u8 xe = TI(x,elType);

  #define LOOKUP(T) \
    usz tn = 1<<T, n = xia;                      \
    u##T* xp = (u##T*)i##T##any_ptr(x);          \
    i32* rp; B r = m_i32arrv(&rp, n);            \
    TALLOC(i32, tab, tn);                        \
    for (usz j=0; j<tn; j++) tab[j]=0;           \
    for (usz i=0; i<n;  i++) rp[i]=tab[xp[i]]++; \
    decG(x); TFREE(tab);                         \
    return r
  if (xia>=16 && xe==el_i8 && xia<=(usz)I32_MAX+1) { LOOKUP(8); }
  if (xia>=256 && xe==el_i16 && xia<=(usz)I32_MAX+1) { LOOKUP(16); }
  #undef LOOKUP

  i32* rp; B r = m_i32arrv(&rp, xia);
  H_b2i* map = m_b2i(64);
  SGetU(x)
  for (usz i = 0; i < xia; i++) {
    bool had; u64 p = mk_b2i(&map, GetU(x,i), &had);
    rp[i] = had? ++map->a[p].val : (map->a[p].val = 0);
  }
  decG(x); free_b2i(map);
  return r;
}

extern B rt_indexOf;
B indexOf_c1(B t, B x) {
  if (isAtm(x) || RNK(x)==0) thrM("⊐: 𝕩 cannot have rank 0");
  if (RNK(x)>1) x = toCells(x);
  usz xia = IA(x);
  if (xia==0) { decG(x); return emptyIVec(); }
  u8 xe = TI(x,elType);

  #define LOOKUP(T) \
    usz tn = 1<<T, n = xia;                      \
    u##T* xp = (u##T*)i##T##any_ptr(x);          \
    i32* rp; B r = m_i32arrv(&rp, n);            \
    TALLOC(i32, tab, tn);                        \
    for (usz j=0; j<tn; j++) tab[j]=n;           \
    i32 u=0;                                     \
    for (usz i=0; i<n;  i++) {                   \
      u##T j=xp[i]; i32 t=tab[j];                \
      if (t==n) rp[i]=tab[j]=u++; else rp[i]=t;  \
    }                                            \
    decG(x); TFREE(tab);                         \
    return r
  if (xia>=16 && xe==el_i8 && xia<=(usz)I32_MAX+1) { LOOKUP(8); }
  if (xia>=256 && xe==el_i16 && xia<=(usz)I32_MAX+1) { LOOKUP(16); }
  #undef LOOKUP

  if (xe==el_i32) {
    i32* xp = i32any_ptr(x);
    i32 min=I32_MAX, max=I32_MIN;
    for (usz i = 0; i < xia; i++) {
      i32 c = xp[i];
      if (c<min) min = c;
      if (c>max) max = c;
    }
    i64 dst = 1 + (max-(i64)min);
    if ((dst<xia*5 || dst<50) && min!=I32_MIN) {
      i32* rp; B r = m_i32arrv(&rp, xia);
      TALLOC(i32, tmp, dst);
      for (i64 i = 0; i < dst; i++) tmp[i] = I32_MIN;
      i32* tc = tmp-min;
      i32 ctr = 0;
      for (usz i = 0; i < xia; i++) {
        i32 c = xp[i];
        if (tc[c]==I32_MIN) tc[c] = ctr++;
        rp[i] = tc[c];
      }
      decG(x); TFREE(tmp);
      return r;
    }
  }
  // // relies on equal hashes implying equal objects, which has like a 2⋆¯64 chance of being false per item
  // i32* rp; B r = m_i32arrv(&rp, xia);
  // u64 size = xia*2;
  // wyhashmap_t idx[size];
  // i32 val[size];
  // for (i64 i = 0; i < size; i++) { idx[i] = 0; val[i] = -1; }
  // SGet(x)
  // i32 ctr = 0;
  // for (usz i = 0; i < xia; i++) {
  //   u64 hash = bqn_hash(Get(x,i), wy_secret);
  //   u64 p = wyhashmap(idx, size, &hash, 8, true, wy_secret);
  //   if (val[p]==-1) val[p] = ctr++;
  //   rp[i] = val[p];
  // }
  // dec(x);
  // return r;
  i32* rp; B r = m_i32arrv(&rp, xia);
  H_b2i* map = m_b2i(64);
  SGetU(x)
  i32 ctr = 0;
  for (usz i = 0; i < xia; i++) {
    bool had; u64 p = mk_b2i(&map, GetU(x,i), &had);
    if (had) rp[i] = map->a[p].val;
    else     rp[i] = map->a[p].val = ctr++;
  }
  free_b2i(map); decG(x);
  return r;
}

extern B rt_find;
B find_c1(B t, B x) {
  if (isAtm(x) || RNK(x)==0) thrM("⍷: Argument cannot have rank 0");
  if (RNK(x)>1) return c1(rt_find, x);
  usz xia = IA(x);
  if (xia<=1) return x;
  B xf = getFillQ(x);
  
  B r = emptyHVec();
  H_Sb* set = m_Sb(64);
  SGetU(x)
  for (usz i = 0; i < xia; i++) {
    B c = GetU(x,i);
    if (!ins_Sb(&set, c)) r = vec_add(r, inc(c));
  }
  free_Sb(set); decG(x);
  return withFill(r, xf);
}
