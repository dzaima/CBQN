#pragma once
#include "wyhash.h"
#include "talloc.h"

extern u64 wy_secret[4];

static u64 bqn_hash(B x, const u64 secret[4]) { // doesn't consume
  if (isAtm(x)) {
    if (q_f64(x)) return wyhash64(secret[0], x.u);
    if (isC32(x)) return wyhash64(secret[1], x.u);
    assert(isVal(x));
    return wyhash64(secret[2], x.u);
  }
  inc(x);
  x = any_squeeze(x);
  u64 shHash = wyhash(a(x)->sh, rnk(x)*sizeof(usz), 0, secret);
  u8 xe = TI(x,elType);
  usz xia = a(x)->ia;
  u64 r;
  if      (xe==el_i8 ) { r = wyhash(i8any_ptr (x), xia*1, shHash, secret); }
  else if (xe==el_i16) { r = wyhash(i16any_ptr(x), xia*2, shHash, secret); }
  else if (xe==el_i32) { r = wyhash(i32any_ptr(x), xia*4, shHash, secret); }
  else if (xe==el_c8 ) { r = wyhash(c8any_ptr (x), xia*1, shHash, secret); }
  else if (xe==el_c16) { r = wyhash(c16any_ptr(x), xia*2, shHash, secret); }
  else if (xe==el_c32) { r = wyhash(c32any_ptr(x), xia*4, shHash, secret); }
  else if (xe==el_f64) { r = wyhash(f64any_ptr(x), xia*8, shHash, secret); }
  else {
    assert(xe==el_B);
    TALLOC(u64, data, xia);
    SGetU(x)
    for (usz i = 0; i < xia; i++) data[i] = bqn_hash(GetU(x, i), secret);
    r = wyhash(data, xia*8, shHash, secret);
    TFREE(data);
  }
  dec(x);
  return r;
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
