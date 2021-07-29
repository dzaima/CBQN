#include "../core.h"
#include "hash.h"

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
