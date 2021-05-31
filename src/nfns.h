#pragma once
#include "core.h"

typedef struct NFnDesc NFnDesc;
typedef struct NFn { // native function
  struct Fun;
  u32 id; // index in nfn_list
  // custom fields:
  i32 data;
  B obj;
} NFn;

NFnDesc* registerNFn(B name, BB2B c1, BBB2B c2); // should be called a constant number of times; consumes name
B m_nfn(NFnDesc* desc, B obj);
B nfn_name(B x); // doesn't consume
static B nfn_objU(B t) {
  return c(NFn,t)->obj;
}
static i32 nfn_data(B t) {
  return c(NFn,t)->data;
}