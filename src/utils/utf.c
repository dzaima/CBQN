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
    u8* rp; B r = m_c8arrv(&rp, sz);
    for (i64 i = 0; i < len; i++) rp[i] = s[i];
    return r;
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
  B r = utf8Decode((char*)a->a, a->ia);
  ptr_dec(a);
  return r;
}

void printUTF8(u32 c) {
  if (c<128) printf("%c", c);
  else if (c<=0x07FF) printf("%c%c"    , 0xC0| c>>6 , 0x80|(c    &0x3F)                                 );
  else if (c<=0xFFFF) printf("%c%c%c"  , 0xE0| c>>12, 0x80|(c>>6 &0x3F), 0x80|(c   &0x3F)               );
  else                printf("%c%c%c%c", 0xF0| c>>18, 0x80|(c>>12&0x3F), 0x80|(c>>6&0x3F), 0x80|(c&0x3F));
}
void fprintUTF8(FILE* f, u32 c) {
  if (c<128) fprintf(f, "%c", c);
  else if (c<=0x07FF) fprintf(f, "%c%c"    , 0xC0| c>>6 , 0x80|(c    &0x3F)                                 );
  else if (c<=0xFFFF) fprintf(f, "%c%c%c"  , 0xE0| c>>12, 0x80|(c>>6 &0x3F), 0x80|(c   &0x3F)               );
  else                fprintf(f, "%c%c%c%c", 0xF0| c>>18, 0x80|(c>>12&0x3F), 0x80|(c>>6&0x3F), 0x80|(c&0x3F));
}


u64 utf8lenB(B x) { // doesn't consume; may error as it verifies whether is all chars
  assert(isArr(x));
  SGetU(x)
  usz ia = a(x)->ia;
  u64 res = 0;
  for (usz i = 0; i < ia; i++) {
    u32 c = o2c(GetU(x,i));
    res+= c<128? 1 : c<0x07FF? 2 : c<0xFFFF? 3 : 4;
  }
  return res;
}
void toUTF8(B x, char* p) {
  SGetU(x)
  usz ia = a(x)->ia;
  for (usz i = 0; i < ia; i++) {
    u32 c = o2cu(GetU(x,i));
    if (c<128)          { *p++ = c; }
    else if (c<=0x07FF) { *p++ = 0xC0|c>>6 ; *p++ = 0x80|(c    &0x3F); }
    else if (c<=0xFFFF) { *p++ = 0xE0|c>>12; *p++ = 0x80|(c>>6 &0x3F);*p++ = 0x80|(c   &0x3F); }
    else                { *p++ = 0xF0|c>>18; *p++ = 0x80|(c>>12&0x3F);*p++ = 0x80|(c>>6&0x3F); *p++ = 0x80|(c&0x3F); }
  }
}
