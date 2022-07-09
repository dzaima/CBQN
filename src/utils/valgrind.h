#pragma once
#include <valgrind/memcheck.h>

static u64 vg_getDefined_u64(u64 x) { // for each bit, returns whether it is defined
  u64 r;
  i32 v = VALGRIND_GET_VBITS(&x, &r, 8);
  if(v==0) return ~0ULL; // don't do weird stuff if not on valgrind
  if (v!=1) err("unexpected VALGRIND_GET_VBITS result");
  return ~r;
}
static u64 vg_withDefined_u64(u64 x, u64 where) {
  where = ~where;
  i32 v = VALGRIND_SET_VBITS(&x, &where, 8);
  if (v>1) err("unexpected VALGRIND_SET_VBITS result");
  return x;
}
static u64 vg_undef_u64(u64 x) {
  return vg_withDefined_u64(x, 0);
}
static u64 vg_def_u64(u64 x) {
  return vg_withDefined_u64(x, ~0ULL);
}
static u64 vg_withBit_u64(u64 r, i32 i, bool val) {
  return (r & ~(1ULL<<i)) | ((u64)val)<<i;
}
void vg_printDefined_u64(char* name, u64 x);
void vg_printDump_p(char* name, void* data, u64 len);
#define vg_printDump_v(X) ({ AUTO x_ = (X); vg_printDump_p(#X, &x_, sizeof(x_)); })