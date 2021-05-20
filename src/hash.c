#include "h.h"
#include "wyhash.h"

B bqn_squeeze(B x) {
  assert(isArr(x));
  u8 xe = TI(x).elType;
  if (xe==el_i32 || xe==el_c32) return x;
  usz ia = a(x)->ia;
  if (ia==0) return x;
  if (xe==el_f64) {
    f64* xp = f64any_ptr(x);
    for (usz i = 0; i < ia; i++) if (xp[i] != (f64)(i32)xp[i]) return x;
    return tag(toI32Arr(x), ARR_TAG);
  }
  assert(xe==el_B);
  BS2B xgetU = TI(x).getU;
  B x0 = xgetU(x, 0);
  if (isNum(x0)) {
    for (usz i = 0; i < ia; i++) {
      B c = xgetU(x, i);
      if (!isNum(c)) return x;
      if (!q_i32(c)) {
        for (i++; i < ia; i++) if (!isNum(xgetU(x, i))) return x;
        return tag(toF64Arr(x), ARR_TAG);
      }
    }
    return tag(toI32Arr(x), ARR_TAG);
  } else if (isC32(x0)) {
    for (usz i = 1; i < ia; i++) {
      B c = xgetU(x, i);
      if (!isC32(c)) return x;
    }
    return tag(toC32Arr(x), ARR_TAG);
  } else return x;
}

u64 bqn_hash(B x, const u64 secret[4]) { // doesn't consume
  if (isAtm(x)) {
    if (q_f64(x)) return wyhash64(secret[0], x.u);
    if (isC32(x)) return wyhash64(secret[1], x.u);
    assert(isVal(x));
    return wyhash64(secret[2], x.u);
  }
  inc(x);
  x = bqn_squeeze(x);
  u64 shHash = wyhash(a(x)->sh, rnk(x)*sizeof(usz), 0, secret);
  u8 xe = TI(x).elType;
  usz xia = a(x)->ia;
  u64 r;
  if (xe==el_i32) {
    r = wyhash(i32any_ptr(x), xia*4, shHash, secret);
  } else if (xe==el_c32) {
    r = wyhash(c32any_ptr(x), xia*4, shHash, secret);
  } else if (xe==el_f64) {
    r = wyhash(f64any_ptr(x), xia*8, shHash, secret);
  } else {
    assert(xe==el_B);
    u64 data[xia];
    BS2B xgetU = TI(x).getU;
    for (usz i = 0; i < xia; i++) data[i] = bqn_hash(xgetU(x, i), secret);
    r = wyhash(data, xia*8, shHash, secret);
  }
  dec(x);
  return r;
}

u64 wy_secret[4];
u64 bqn_hashP(B x, const u64 secret[4]) { // bqn_hash but never zero
  u64 r = bqn_hash(x, secret);
  return LIKELY(r)?r:secret[3]; // bias towards secret[3], whatever
}



#define N(X) X##_b2i
#define HT u64
#define KT B
#define H1(K) bqn_hashP(K, wy_secret)
#define H2(K,h1) h1
#define H1R(K,h2) h2
#define EMPTY(S,K) ((S)==0)
#define HDEF 0
#define KEYS
#define EQUAL(A,B) equal(A,B)
#define VALS
#define VT i32
#include "hashmap.c"



void hash_init() {
  u64 bad1=0xa0761d6478bd642full; // values wyhash64 is afraid of
  u64 bad2=0xe7037ed1a0b428dbull;
  again:
  make_secret(nsTime(), wy_secret);
  for (u64 i = 0; i < 4; i++) if(wy_secret[i]==bad1 || wy_secret[i]==bad2) goto again;
}
