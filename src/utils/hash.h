#pragma once
#include "wyhash.h"
#include "talloc.h"

extern u64 wy_secret[4];

static void bcl(B x, usz ia) { // clean up bitarr tail bits to zero
  if (ia&63) {
    u64* xp = bitarr_ptr(x);
    xp[ia>>6]&= (1ULL<<(ia&63)) - 1;
  }
}

static inline f64 normalizeFloat(f64 v) {
  return v==v? v+0 : 0.0/0.0;
}
u64 bqn_hashObj(B x, const u64 secret[4]);
static u64 bqn_hash(B x, const u64 secret[4]) { // doesn't consume
  u64 h;
  if (LIKELY(x.f==x.f)) {
    h = m_f64(x.f+0).u;
  } else if (isVal(x)) {
    return bqn_hashObj(x, secret);
  } else if ((x.u<<1) == (0x7FF8000000000000U<<1)) {
    h = secret[1];
  } else {
    h = x.u;
  }
  return wyhash64(secret[0], h);
}

static u64 bqn_hashP(B x, const u64 secret[4]) { // bqn_hash but never zero
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
#include "hashmapTemplate.h"

#define N(X) X##_Sb
#define HT u64
#define KT B
#define H1(K) bqn_hashP(K, wy_secret)
#define H2(K,h1) h1
#define H1R(K,h2) h2
#define EMPTY(S,K) ((S)==0)
#define HDEF 0
#define KEYS
#define EQUAL(A,B) equal(A,B)
#include "hashmapTemplate.h"
