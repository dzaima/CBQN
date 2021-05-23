// args:
//   N(X) - X##_mapName
//   HT - stored hash type (u32 or u64)
//   KT - key type
//   H1(K) - function to compute index in hashmap, returns u64; the expensive part
//   H2(K,h1) - function to compute HT (returning just h1 (or K if applicable) is safe)
//   H1R(K,h2) - calculate H1 from H2, preferrably fast
//   EMPTY(S,K) - test whether value counts as unpopulated (ignore K if !KEYS)
//   HDEF - if defined, the value to initialize HT to (otherwise it's undefined)
//   KEYS - whether to separately store and check keys (otherwise only HT is compared)
//     EQUAL(A,B) - compare two keys
//     KDEF - if defined, the value to initialize keys to (otherwise it's undefined)
//   VALS - whether to store values corresponding to keys (otherwise this is a hashset)
//     VT - value type

#define Map N(H)
#define Ent N(Ent)
#ifdef KEYS
  #define IFKEY(...) __VA_ARGS__
#else
  #define IFKEY(...)
#endif
#ifdef VALS
  #define IFVAL(...) __VA_ARGS__
#else
  #define IFVAL(...)
#endif

typedef struct Ent {
#ifdef KEYS
  KT key;
#endif
  HT hash;
#ifdef VALS
  VT val;
#endif
} Ent;

typedef struct Map {
  struct Value;
  u64 pop; // count of defined entries
  u64 mask; // sz-1
  u64 sz; // count of allocated entries, a power of 2
  Ent a[];
} Map;

static Map* N(m) (u64 sz) {
  assert(sz && (sz & sz-1)==0);
  Map* r = mm_allocN(fsizeof(Map,a,Ent,sz), t_hashmap);
  #ifdef HDEF
    for (u64 i = 0; i < sz; i++) r->a[i].hash = HDEF;
  #endif
  #ifdef KDEF
    for (u64 i = 0; i < sz; i++) r->a[i].key = KDEF;
  #endif
  r->sz = sz;
  r->mask = sz-1;
  r->pop = 0;
  return r;
}
static void N(free) (Map* m) {
  mm_free((Value*) m);
}


static inline u64 N(find) (Map* m, KT k, u64 h1, u64 h2, bool* had) {
  assert(m->sz > m->pop);
  *had = true;
  u64 mask = m->mask;
  u64 p = h1 & mask;
  while (true) {
    Ent e = m->a[p];
    if (e.hash==h2 IFKEY(&& LIKELY(EQUAL(e.key, k)))) return p;
    if (EMPTY(e.hash, e.key)) { *had=false; return p; }
    if (RARE(p++==mask)) p = 0;
  }
}
static inline bool N(has) (Map* m, KT k) {
  bool has;
  u64 h1 = H1(k); u64 h2 = H2(k, h1);
  N(find)(m, k, h1, h2, &has);
  return has;
}
#ifdef VALS
  static inline u64 N(getD) (Map* m, KT k, VT def) {
    bool has;
    u64 h1 = H1(k); u64 h2 = H2(k, h1);
    u64 p = N(find)(m, k, h1, h2, &has);
    return has? m->a[p].val : def;
  }
#endif


static inline void N(qins) (Map* m, u64 h1, HT h2, KT k IFVAL(, VT v)) { // if guaranteed that k doesn't exist in the map yet and there's space for this
  u64 mask = m->mask;
  u64 p = h1 & mask;
  while (true) {
    u64 ch2 = m->a[p].hash;
    if (EMPTY(ch2, m->a[p].key)) {
      m->a[p].hash = h2;
      IFKEY(m->a[p].key = k);
      IFVAL(m->a[p].val = v);
      m->pop++;
      return;
    }
    if (p++==mask) p = 0;
  }
}
static void N(dbl) (Map** m) {
  Map* pm = *m;
  u64 psz = pm->sz;
  Map* nm = N(m)(psz*2);
  for (u64 i = 0; i < psz; i++) {
    Ent e = pm->a[i];
    if(!EMPTY(e.hash, e.key)) N(qins)(nm, H1R(e.key, e.hash), e.hash, e.key IFVAL(, e.val));
  }
  mm_free((Value*)pm);
  *m = nm;
}


static inline u64 N(mk) (Map** mp, KT k, bool* had) {
  Map* m = *mp;
  if (m->pop*2 > m->sz) { N(dbl)(mp); m=*mp; }
  u64 h1 = H1(k); u64 h2 = H2(k, h1);
  u64 p = N(find)(m, k, h1, h2, had);
  if (*had) return p;
  m->a[p].hash = h2;
  IFKEY(m->a[p].key = k);
  m->pop++;
  return p;
}
static inline bool N(ins) (Map** mp, KT k IFVAL(, VT v)) { // returns whether element was replaced
  bool had;
  IFVAL(u64 p =) N(mk)(mp, k, &had);
  IFVAL((*mp)->a[p].val = v);
  return had;
}


#undef IFKEY
#undef IFVAL
#undef Ent
#undef Map
#undef N
#undef HT
#undef KT
#undef H1
#undef H2
#undef EMPTY
#undef HDEF
#ifdef KEYS
  #undef KEYS
  #undef EQUAL
  #undef KDEF
#endif
#ifdef VALS
  #undef VALS
  #undef VT
#endif
