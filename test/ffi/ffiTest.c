#include <stdlib.h>
#include <stdio.h>
#include "../../src/ffi.h"

void do_nothing() { }

BQNV timesTen(BQNV v) {
  size_t len = bqn_bound(v);
  int32_t* buf = malloc(len*4);
  bqn_readI32Arr(v, buf);
  for (int i = 0; i < len; i++) buf[i] = buf[i] * 10;
  BQNV res = bqn_makeI32Vec(len, buf);
  free(buf);
  bqn_free(v);
  return res;
}

BQNV add_args(BQNV t, BQNV x) {
  return bqn_makeF64(bqn_toF64(t) + bqn_toF64(x));
}
BQNV bind_add(BQNV x) {
  BQNV r = bqn_makeBoundFn1(add_args, x);
  bqn_free(x);
  return r;
}



void printArgs(int8_t i8, int16_t i16, int32_t i32, uint8_t u8, uint16_t u16, uint32_t u32, float f, double d) {
  printf("args: %d %d %d %u %u %u %.18f %.18f\n", i8, i16, i32, u8, u16, u32, f, d);
}
void noopArgs(int8_t i8, int16_t i16, int32_t i32, uint8_t u8, uint16_t u16, uint32_t u32, float f, double d) { }

void printPtrArgs(int8_t* i8, int16_t* i16, int32_t* i32, uint8_t* u8, uint16_t* u16, uint32_t* u32, float* f, double* d) {
  printf("args: %d %d %d %u %u %u %.18f %.18f\n", *i8, *i16, *i32, *u8, *u16, *u32, *f, *d);
}

float printU64s(uint64_t a, uint64_t* b) {
  printf("%llx %llx %llx %llx\n", (long long)a, (long long)b[0], (long long)b[1], (long long)b[2]);
  return 12345678;
}



int32_t multiplyI32Ptrs(int32_t* a, int32_t* b, int32_t len) {
  int32_t r = 0;
  for (int i = 0; i < len; i++) {
    r+= a[i]*b[i];
  }
  return r;
}
float sumF32Arr(float* f, int len) {
  float r = 0;
  for (int i = 0; i < len; i++) r+= f[i];
  return r;
}


void incI32s(int32_t* a, int32_t len) {
  for (int i = 0; i < len; i++) a[i]++;
}

void incInts(int32_t* a, int16_t* b, int8_t* c) {
  printf("%d %d %d\n", *a, *b, *c);
  (*a)++;
  (*b)++;
  (*c)++;
}

uint64_t ident_u64(uint64_t x) { return x; }
double ident_f64(double a) { return a; }

void* pick_ptr(void** arr, int idx) {
  return arr[idx];
}
uint64_t pick_u64(uint64_t* arr, int idx) {
  return arr[idx];
}




int plusone(int x) {
  return x + 1;
}
