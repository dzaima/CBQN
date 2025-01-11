#include "../utils/talloc.h"
#include <windows.h>

static u64 utf16lenB(B x) { // doesn't consume
  assert(isArr(x));
  SGetU(x)
  usz ia = IA(x);
  u64 res = 0;
  for (usz i = 0; i < ia; ++i) {
    u32 c = o2c(GetU(x,i));
    res+= 1+(c > 0xFFFF);
  }
  return res;
}

FORCE_INLINE void utf16_w(WCHAR** buf_i, u32 c)
{
  WCHAR* buf = *buf_i;
  if (c<=0xFFFF) {
    *buf++ = c;
  } else {
    assert(c <= 0x10FFFF);
    *buf++ = ((c-0x10000) >> 10)+0xD800;
    *buf++ = ((c-0x10000)&0x3FF)+0xDC00;
  }
  *buf_i = buf;
}

static void toUTF16(B x, WCHAR* p) {
  SGetU(x)
  usz ia = IA(x);
  for (u64 i = 0; i < ia; ++i) utf16_w(&p, o2cG(GetU(x,i)));
}

static B utf16Decode(const WCHAR* s, i64 len) {
#define UTF16_MASK 0xFC00
#define UTF16_IS_HI(WC) ((UTF16_MASK&(WC))==0xD800) /* 0xD800..0xDBFF */
#define UTF16_IS_LO(WC) ((UTF16_MASK&(WC))==0xDC00) /* 0xDC00..0xDFFF */
#define UTF16_SURROGATE(HI, LO) (0x10000+(((HI)-0xD800) << 10)+((LO)-0xDC00))
  u64 sz = 0;
  for (i64 j = 0; ; ++j) {
    if (j>=len) {
      if (j!=len) assert(0);
      break;
    }
    if (UTF16_IS_HI(s[j]) && j+1<len && UTF16_IS_LO(s[j+1])) ++j;
    ++sz;
  }
  u32* rp; B r = m_c32arrv(&rp, sz);
  u64 p = 0;
  for (i64 i = 0; i < len; ++i) {
    if (UTF16_IS_HI(s[i]) && i+1<len && UTF16_IS_LO(s[i+1])) {
      rp[p++] = UTF16_SURROGATE(s[i], s[i+1]);  
      ++i;
    } else {
      rp[p++] = s[i];
    }
  }
  assert(p==sz);
  return r;
#undef UTF16_MASK
#undef UTF16_IS_HI
#undef UTF16_IS_LO
#undef UTF16_SURROGATE
}

static WCHAR* toWStr(B x) { // doesn't consume
  u64 len = utf16lenB(x);
  TALLOC(WCHAR, p, len+1);
  toUTF16(x, p);
  p[len] = 0;
  return p;
}
static void freeWStr(WCHAR* p) {
  TFREE(p);
}
