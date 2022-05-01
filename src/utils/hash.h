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

u64 bqn_hashArr(B x, const u64 secret[4]);
static u64 bqn_hash(B x, const u64 secret[4]) { // doesn't consume
  if (isArr(x)) return bqn_hashArr(x, secret);
  return wyhash64(secret[0], x.u);
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
