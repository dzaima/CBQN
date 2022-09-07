#include "../core.h"
#define HASH_C 1
#include "hash.h"
#include "time.h"

NOINLINE u64 bqn_hashArr(B x, const u64 secret[4]) { // TODO manual separation of atom & arr probably won't be worth it when there are actually sane typed array hashing things
  usz xia = IA(x);
  if (xia==0) return ~secret[3]; // otherwise squeeze will care about fills
  x = any_squeeze(incG(x));
  u8 xr = RNK(x);
  u8 xe = TI(x,elType);
  u64 shHash;
  if (xr<=1) shHash = wyhash64(xia, xe);
  else shHash = wyhash(SH(x), xr*sizeof(usz), xe, secret);
  bool isTemp = false;
  void* data;
  u64 bytes;
  switch(xe) { default: UD;
    case el_bit: bcl(x,xia);  data = bitarr_ptr(x); bytes = (xia+7)>>3; break;
    case el_i8:  case el_c8:  data =  tyany_ptr(x); bytes = xia*1; break;
    case el_i16: case el_c16: data =  tyany_ptr(x); bytes = xia*2; break;
    case el_i32: case el_c32: data =  tyany_ptr(x); bytes = xia*4; break;
    case el_f64:              data = f64any_ptr(x); bytes = xia*8; break;
    case el_B:;
      data = TALLOCP(u64, xia);
      isTemp = true;
      SGetU(x)
      for (usz i = 0; i < xia; i++) ((u64*)data)[i] = bqn_hash(GetU(x, i), secret);
      bytes = xia*sizeof(B);
      break;
  }
  assert(bytes!=0);
  u64 r = wyhash(data, bytes, shHash, secret);
  if (isTemp) TFREE(data);
  dec(x);
  return r;
}


u64 wy_secret[4];

void hash_init() {
  u64 bad1=0xa0761d6478bd642full; // values wyhash64 is afraid of
  u64 bad2=0xe7037ed1a0b428dbull;
  again:
  #ifdef PERF_TEST
    make_secret(0, wy_secret);
  #else
    make_secret(nsTime(), wy_secret);
  #endif
  for (u64 i = 0; i < 4; i++) if(wy_secret[i]==bad1 || wy_secret[i]==bad2) goto again;
}
