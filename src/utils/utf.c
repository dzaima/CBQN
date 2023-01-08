#include "../core.h"
#include "utf.h"

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
#if defined(_WIN32) || defined(_WIN64)
  #include <io.h>
  #include <fcntl.h>
  void fprintsU32(FILE* f, u32* s, usz len) {
    _setmode(_fileno(f), _O_U16TEXT);
    #define BUF_SZ 1024
    wchar_t buf[BUF_SZ];
    u32* s_e = s+len;
    while (s<s_e) {
      wchar_t* buf_c = buf;
      wchar_t* buf_e = buf+BUF_SZ-10;
      while (s<s_e && buf_c<buf_e) {
        u32 c = *s;
        if (c<65536) {
          if (c==0) break; // can't print null bytes into null-terminated buffer
          buf_c[0] = c;
          buf_c++;
        } else {
          c-= 0x10000;
          buf_c[0] = 0xD800 + (c >> 10);
          buf_c[1] = 0xDC00 + (c & ((1<<10)-1));
          buf_c+= 2;
        }
        s++;
      }
      buf_c[0] = 0;
      fwprintf(f, L"%ls", buf);
      
      while (s<s_e && *s==0) { // handle printing of null bytes; does nothing? idk
        fwprintf(f, L"%c", '\0');
        s++;
      }
    }
    #undef BUF_SZ
    _setmode(_fileno(f), _O_BINARY);
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
    incG(x);
    if (!elChr(xe)) { x=chr_squeeze(x); xe=TI(x,elType); }
    if (!elChr(xe)) {
      #if !CATCH_ERRORS
        SGetU(x)
        for (usz i = 0; i < ia; i++) {
          B c = GetU(x, i);
          if (isC32(c)) fprintCodepoint(f, o2cG(c));
          else if (c.u==0 || noFill(c)) fprintf(f, " ");
          else thrM("Trying to output non-character");
        }
        return;
      #endif
      thrM("Trying to output non-character");
    }
    x = taga(cpyC32Arr(x));
    fprintsU32(f, c32any_ptr(x), ia);
    decG(x);
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
  for (usz i = 0; i < ia; i++) {
    u32 c = o2cG(GetU(x,i));
    if (c<128)          { *p++ = c; }
    else if (c<=0x07FF) { *p++ = 0xC0|c>>6 ; *p++ = 0x80|(c    &0x3F); }
    else if (c<=0xFFFF) { *p++ = 0xE0|c>>12; *p++ = 0x80|(c>>6 &0x3F);*p++ = 0x80|(c   &0x3F); }
    else                { *p++ = 0xF0|c>>18; *p++ = 0x80|(c>>12&0x3F);*p++ = 0x80|(c>>6&0x3F); *p++ = 0x80|(c&0x3F); }
  }
}
