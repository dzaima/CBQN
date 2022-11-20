#include "../core.h"
#if !NO_MMAP
#include <sys/mman.h>
#endif
#include "gc.c"

#ifdef OBJ_COUNTER
  u64 currObjCounter;
#endif

u64 mm_ctrs[64];
EmptyValue* mm_buckets[64];
#define  ALSZ   20
#define  BSZ(X) (1ull<<(X))
#define  MMI(X) X
#define   BN(X) mm_##X
#include "mm_buddyTemplate.c"

u64 mm_heapUsed() {
  u64 r = 0;
  for (i32 i = 0; i < 64; i++) r+= mm_ctrs[i]*BSZ(i);
  return r;
}
#undef BN
#undef BSZ

#if VERIFY_TAIL
FORCE_INLINE u64 ptrHash(void* ptr, usz off) {
  u64 pv = ptr2u64(ptr) ^ off;
  pv*= 0xa0761d6478bd642full;
  pv-= off;
  pv^= 0xe7037ed1a0b428dbull;
  return pv;
}
FORCE_INLINE u64 limitLen(u64 l) {
  if (l > VERIFY_TAIL*4) return VERIFY_TAIL*4;
  return l;
}

#define ITER_TAIL(F) {                                        \
  u64 o = filled; u8* d = (u8*)ptr;                           \
  if ((o&7) && o<end) {                                       \
    u64 h = ptrHash(ptr, o&~7ULL);                            \
    for(;(o&7)&&o<end;o++) F(d[o], (u8)(h>>((o&7)*8)), o, 1); \
  }                                                           \
  for(;o+8<=end;o+=8) F(*(u64*)(d+o), ptrHash(ptr, o), o, 8); \
  if (o!=end) {                                               \
    u64 h = ptrHash(ptr, o&~7ULL);                            \
    for(;o!=end;o++) F(d[o], (u8)(h>>((o&7)*8)), o, 1);       \
  }                                                           \
}

static void tailVerifyInit(void* ptr, u64 filled, u64 end, u64 allocEnd) {
  #define F(W, X, O, L) W = X
  ITER_TAIL(F)
  #undef F
}
void tailVerifyAlloc(void* ptr, u64 filled, i64 logAlloc, u8 type) {
  u64 end = 1ULL<<logAlloc;
  filled = 8; // permit decreasing used size without having written anything to the space
  tailVerifyInit(ptr, filled, end, end);
}
void verifyEnd(void* ptr, u64 sz, u64 start, u64 end) {
  if (end+64>sz) { printf("Bad used range: "N64u".."N64u", allocation size "N64u"\n", start, end, sz); exit(1); }
}
void tailVerifyReinit(void* ptr, u64 filled, u64 end) {
  if(filled>end || filled<=8) { printf("Bad reinit arguments: "N64u".."N64u"\n", filled, end); exit(1); }
  verifyEnd(ptr, mm_size(ptr), filled, end);
  tailVerifyInit(ptr, filled, end, mm_size(ptr));
}
void g_iv(void*);
NOINLINE void dumpByte(bool exp, bool has, void* ptr, u64 o) {
  u8 h = ptrHash(ptr, o&~7ULL)>>((o&7)*8);
  u8 m = ((u8*)ptr)[o];
  u8 v = exp? h : m;
  if (!(o&7)) printf(" ");
  bool col = exp? !has : has && h!=m;
  if (col) printf(exp? "\x1b[38;5;240m" : "\x1b[38;5;203m");
  printf("%02x", v);
  if (col) printf("\x1b[0m");
}
NOINLINE NORETURN void tailFail(u64 got, u64 exp, void* ptr, u64 off, int len, u64 allocFilled, u64 allocTotal, u64 ia) {
  printf("Corrupted tail @ %p + "N64d", checked length %d\n", ptr, off, len);
  printf("Allocation filled with "N64d" bytes, total allocation "N64d", IA=="N64d"\n", allocFilled, allocTotal, ia);
  printf("Expected: x=%016"SCNx64" / u="N64u"\nGot:      x=%016"SCNx64" / u="N64u"\n\n", exp, exp, got, got);
  fflush(stdout); fflush(stderr);
  
  u64 dS = off<32? 0 : off-32;
  if (dS>0) dS&= ~7ULL;
  u64 dE = off+32; if (dE>allocTotal) dE = allocTotal; dE&= ~7ULL;
  printf("%-+6"SCNd64,dS); for (u64 i=dS; i<dE; i+= 8) printf("   0x%012"SCNx64, i + ptr2u64(ptr)); printf("\n");
  printf("exp:  ");          for (u64 i=dS; i<dE; i++) dumpByte(true,  i>=allocFilled, ptr, i); printf("\n");
  printf("got:  ");          for (u64 i=dS; i<dE; i++) dumpByte(false, i>=allocFilled, ptr, i); printf("\n");
  fflush(stdout); fflush(stderr);
  
  if (((Value*)ptr)->refc==0) ((Value*)ptr)->refc = 1010101009; // this allocation was just about to be freed, so refcount 0 is understandable
  g_iv(ptr);
  fflush(stdout); fflush(stderr);
  __builtin_trap();
}
void tailVerifyFree(void* ptr) {
  u64 filled; u64 ia; Arr* xa = ptr;
  switch(PTY(xa)) { default: return;
    case t_bitarr: filled = BITARR_SZ(ia=PIA(xa)); break;
    case t_i8arr:  filled = TYARR_SZ(I8,  ia=PIA(xa)); break;
    case t_i16arr: filled = TYARR_SZ(I16, ia=PIA(xa)); break;
    case t_i32arr: filled = TYARR_SZ(I32, ia=PIA(xa)); break;
    case t_f64arr: filled = TYARR_SZ(F64, ia=PIA(xa)); break;
    case t_harr:    filled = fsizeof(HArr,a,B,ia=PIA(xa)); break;
    case t_fillarr: filled = fsizeof(FillArr,a,B,ia=PIA(xa)); break;
  }
  u64 end = mm_size(ptr);
  verifyEnd(ptr, end, 8, filled);
  #define F(G, X, O, L) if ((G) != (X)) tailFail(G, X, ptr, O, L, filled, end, ia)
  ITER_TAIL(F)
  #undef F
}

#endif
