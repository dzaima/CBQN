#include "../core.h"
#include "utf.h"
#include "calls.h"

static i8 utf8lenb(u8 ch) {
  if (ch<128)           return 1;
  if ((ch>>5)==  0b110) return 2;
  if ((ch>>4)== 0b1110) return 3;
  if ((ch>>3)==0b11110) return 4;
  return -1;
}
static u32 utf8_p(u8* p) {
  i32 len = utf8lenb(*p);
  switch (len) { default: UD;
    case -1: return (u32)-1;
    case  1: return *p;
    case  2: return (0b11111u&*p)<< 6  |  (0b111111u&p[1]);
    case  3: return (0b1111u &*p)<<12  |  (0b111111u&p[2]) | (0b111111u&p[1])<<6;
    case  4: return (0b111u  &*p)<<18  |  (0b111111u&p[3]) | (0b111111u&p[2])<<6 | (0b111111u&p[1])<<12;
  }
}
FORCE_INLINE void utf8_w(char** buf_i, u32 c) {
  char* buf = *buf_i;
  if (c<128) { *buf++ = c; }
  else if (c<=0x07FF) { *buf++ = 0xC0| c>>6 ; *buf++ = 0x80|(c    &0x3F); }
  else if (c<=0xFFFF) { *buf++ = 0xE0| c>>12; *buf++ = 0x80|(c>>6 &0x3F); *buf++ = 0x80|(c   &0x3F); }
  else                { *buf++ = 0xF0| c>>18; *buf++ = 0x80|(c>>12&0x3F); *buf++ = 0x80|(c>>6&0x3F); *buf++ = 0x80|(c&0x3F); }
  *buf_i = buf;
}

B utf8Decode(const char* s, i64 len) {
  u64 sz = 0;
  i64 j = 0;
  while (true) {
    if (j>=len) {
      if (j!=len) thrM("Invalid UTF-8");
      break;
    }
    i8 l = utf8lenb((u8)s[j]);
    if (l==-1) thrM("Invalid UTF-8");
    // TODO validate unicode or something
    sz++;
    j+= l;
  }
  if (sz==len) {
    return m_c8vec((char*)s, len);
  } else {
    u32* rp; B r = m_c32arrv(&rp, sz);
    u64 p = 0;
    for (i64 i = 0; i < len; i+= utf8lenb((u8)s[i])) rp[p++] = utf8_p((u8*)s+i); // may read after end, eh
    return r;
  }
}

B utf8Decode0(const char* s) {
  return utf8Decode(s, strlen(s));
}

B utf8DecodeA(I8Arr* a) { // consumes a
  B r = utf8Decode((char*)a->a, PIA(a));
  ptr_dec(a);
  return r;
}


// printing functions should avoid allocations, as they'll be used to print.. the out-of-memory message
#if defined(USE_REPLXX_IO)
  void fprintsU32(FILE* f, u32* s, usz len) {
    u32* s_e = s+len;
    
    #define BUF_SZ 1024
    char buf[BUF_SZ];
    while (s<s_e) {
      char* buf_c = buf;
      char* buf_e = buf+BUF_SZ-10;
      while (s<s_e && buf_c<buf_e) {
        u32 c = *s;
        if (c==0) break; // can't print null bytes into null-terminated buffer
        utf8_w(&buf_c, c);
        s++;
      }
      buf_c[0] = 0;
      fprintf(f, "%s", buf);
      
      while (s<s_e && *s==0) {
        fprintf(f, "%c", '\0');
        s++;
      }
    }
    #undef BUF_SZ
  }
  void fprintCodepoint(FILE* f, u32 c) {
    fprintsU32(f, (u32[1]){c}, 1);
  }
#else
  void fprintCodepoint(FILE* f, u32 c) {
    if (c<128) fprintf(f, "%c", c);
    else if (c<=0x07FF) fprintf(f, "%c%c"    , 0xC0| c>>6 , 0x80|(c    &0x3F)                                 );
    else if (c<=0xFFFF) fprintf(f, "%c%c%c"  , 0xE0| c>>12, 0x80|(c>>6 &0x3F), 0x80|(c   &0x3F)               );
    else                fprintf(f, "%c%c%c%c", 0xF0| c>>18, 0x80|(c>>12&0x3F), 0x80|(c>>6&0x3F), 0x80|(c&0x3F));
  }
  void fprintsU32(FILE* f, u32* s, usz len) {
    for (usz i = 0; i < len; i++) fprintCodepoint(f, s[i]);
  }
#endif


void fprintsB(FILE* f, B x) {
  u8 xe = TI(x,elType);
  usz ia = IA(x);
  if (ia==0) return;
  if (xe==el_c32) {
    fprintsU32(f, c32any_ptr(x), ia);
  } else {
    #define BUF_SZ 1024
    if (elChr(xe)) {
      if (xe==el_c32) {
        fprintsU32(f, c32any_ptr(x), ia);
      } else {
        u32 buf[BUF_SZ];
        usz i = 0;
        while (i < ia) {
          usz curr = ia-i;
          if (curr>BUF_SZ) curr = BUF_SZ;
          COPY_TO(buf, el_c32, 0, x, i, curr);
          fprintsU32(f, buf, curr);
          i+= curr;
        }
      }
    } else {
      SGetU(x)
      for (usz i = 0; i < ia; i++) {
        B c = GetU(x, i);
        if (isC32(c)) fprintCodepoint(f, o2cG(c));
#if CATCH_ERRORS
        else if (c.u==0 || noFill(c)) fprintf(f, " ");
#endif
        else thrM("Trying to output non-character");
      }
    }
    #undef BUF_SZ
  }
}

u64 utf8lenB(B x) { // doesn't consume; may error as it verifies whether is all chars
  assert(isArr(x));
  SGetU(x)
  usz ia = IA(x);
  u64 res = 0;
  for (usz i = 0; i < ia; i++) {
    u32 c = o2c(GetU(x,i));
    res+= c<128? 1 : c<0x07FF? 2 : c<0xFFFF? 3 : 4;
  }
  return res;
}
void toUTF8(B x, char* p) {
  SGetU(x)
  usz ia = IA(x);
  for (usz i = 0; i < ia; i++) utf8_w(&p, o2cG(GetU(x,i)));
}
