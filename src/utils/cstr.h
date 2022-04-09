#pragma once
#include "talloc.h"
#include "utf.h"

static char* toCStr(B x) { // doesn't consume
  u64 len = utf8lenB(x);
  TALLOC(char, p, len+1);
  toUTF8(x, p);
  p[len] = 0;
  return p;
}
static void freeCStr(char* p) {
  TFREE(p);
}
