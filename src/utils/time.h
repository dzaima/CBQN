#pragma once

#include <time.h>
static inline u64 nsTime() {
  struct timespec t;
  // timespec_get(&t, TIME_UTC); // doesn't seem to exist on Android
  clock_gettime(CLOCK_REALTIME, &t);
  return (u64)(t.tv_sec*1000000000ll + t.tv_nsec);
}
