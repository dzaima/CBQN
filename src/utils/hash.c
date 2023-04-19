#include "../core.h"
#define HASH_C 1
#include "hash.h"
#include "time.h"


NOINLINE u64 bqn_hashObj(B x, const u64 secret[4]) { // TODO manual separation of atom & arr probably won't be worth it when there are actually sane typed array hashing things
  if (isArr(x)) {
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
      case el_f64:              data = f64any_ptr(x); bytes = xia*8;
        for (ux i = 0; i < xia; i++) ((f64*)data)[i] = normalizeFloat(((f64*)data)[i]);
        break;
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
  
  u64 hashbuf[3];
  switch(TY(x)) {
    case t_funBl: case t_md1Bl: case t_md2Bl:
    case t_funBI: case t_md1BI: case t_md2BI:
    IF_WRAP(case t_funWrap: case t_md1Wrap: case t_md2Wrap:)
    case t_ns: case t_nfn: case t_comp: // t_comp for profiler
      return wyhash64(secret[0], x.u);
    
    case t_md1D: {
      Md1D* xv = c(Md1D,x);
      hashbuf[0] = 0;
      hashbuf[1] = bqn_hash(xv->f, secret);
      hashbuf[2] = bqn_hash(tag(xv->m1, MD1_TAG), secret);
      break;
    }
    case t_md2D: {
      Md2D* xv = c(Md2D,x);
      hashbuf[0] = bqn_hash(xv->g, secret);
      hashbuf[1] = bqn_hash(xv->f, secret);
      hashbuf[2] = bqn_hash(tag(xv->m2, MD1_TAG), secret);
      break;
    }
    case t_fork: {
      Fork* xv = c(Fork,x);
      hashbuf[0] = bqn_hash(xv->f, secret);
      hashbuf[1] = bqn_hash(xv->g, secret);
      hashbuf[2] = bqn_hash(xv->h, secret);
      break;
    }
    case t_atop: {
      Atop* xv = c(Atop,x);
      hashbuf[0] = 0;
      hashbuf[1] = bqn_hash(xv->g, secret);
      hashbuf[2] = bqn_hash(xv->h, secret);
      break;
    }
    default: /*printf("%d/%s\n",TY(x),type_repr(TY(x)));*/ thrM("Cannot hash this object");
  }
  return wyhash(hashbuf, sizeof(hashbuf), TY(x), secret);
}


u64 wy_secret[4];

void hash_init(void) {
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
