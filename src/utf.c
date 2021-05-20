i8 utf8lenb(u8 ch) {
  if (ch<128)           return 1;
  if ((ch>>5)==  0b110) return 2;
  if ((ch>>4)== 0b1110) return 3;
  if ((ch>>3)==0b11110) return 4;
  return -1;
}
u32 utf8_p(u8* p) {
  i32 len = utf8lenb(*p);
  switch (len) { default: UD;
    case -1: return (u32)-1;
    case  1: return *p;
    case  2: return (0b11111u&*p)<< 6  |  (0b111111u&p[1]);
    case  3: return (0b1111u &*p)<<12  |  (0b111111u&p[2]) | (0b111111u&p[1])<<6;
    case  4: return (0b111u  &*p)<<18  |  (0b111111u&p[3]) | (0b111111u&p[2])<<6 | (0b111111u&p[1])<<12;
  }
}
B fromUTF8(char* s, i64 len) {
  u64 sz = 0;
  i64 j = 0;
  while (true) {
    if (j>=len) {
      if (j!=len) thrM("Invalid UTF-8");
      break;
    }
    i8 l = utf8lenb((u8)s[j]);
    if (l==-1) thrM("Invalid UTF-8");
    sz++;
    j+= l;
  }
  u32* rp; B r = m_c32arrv(&rp, sz);
  u64 p = 0;
  // TODO verify
  for (i64 i = 0; i < len; i+= utf8lenb((u8)s[i])) rp[p++] = utf8_p((u8*)s+i); // may read after end, eh
  return r;
}

B fromUTF8l(char* s) {
  return fromUTF8(s, strlen(s));
}

void printUTF8(u32 c) {
  if (c<128) printf("%c", c);
  else if (c<=0x07FF) printf("%c%c"    , 0xC0| c>>6 , 0x80|(c    &0x3F)                                 );
  else if (c<=0xFFFF) printf("%c%c%c"  , 0xE0| c>>12, 0x80|(c>>6 &0x3F), 0x80|(c   &0x3F)               );
  else                printf("%c%c%c%c", 0xF0| c>>18, 0x80|(c>>12&0x3F), 0x80|(c>>6&0x3F), 0x80|(c&0x3F));
}

u64 snprintUTF8(char* p, u64 l, u32 c) {
  if (c<128) return snprintf(p, l, "%c", c);
  else if (c<=0x07FF) return snprintf(p, l, "%c%c"    , 0xC0|c>>6 , 0x80|(c    &0x3F)                                 );
  else if (c<=0xFFFF) return snprintf(p, l, "%c%c%c"  , 0xE0|c>>12, 0x80|(c>>6 &0x3F), 0x80|(c   &0x3F)               );
  else                return snprintf(p, l, "%c%c%c%c", 0xF0|c>>18, 0x80|(c>>12&0x3F), 0x80|(c>>6&0x3F), 0x80|(c&0x3F));
}

u64 utf8lenB(B x) { // doesn't consume; may error as it verifies whether is all chars
  assert(isArr(x));
  BS2B xgetU = TI(x).getU;
  usz ia = a(x)->ia;
  u64 res = 0;
  for (usz i = 0; i < ia; i++) {
    u32 c = o2c(xgetU(x,i));
    res+= c<128? 1 : c<0x07FF? 2 : c<0xFFFF? 3 : 4;
  }
  return res;
}
void toUTF8(B x, char* p) { // doesn't consume; doesn't verify anything; p must have utf8lenB(x) bytes (calculating which should verify that this call is ok)
  BS2B xgetU = TI(x).getU;
  usz ia = a(x)->ia;
  for (usz i = 0; i < ia; i++) {
    u32 c = o2cu(xgetU(x,i));
    if (c<128)          { *p++ = c; }
    else if (c<=0x07FF) { *p++ = 0xC0|c>>6 ; *p++ = 0x80|(c    &0x3F); }
    else if (c<=0xFFFF) { *p++ = 0xE0|c>>12; *p++ = 0x80|(c>>6 &0x3F);*p++ = 0x80|(c   &0x3F); }
    else                { *p++ = 0xF0|c>>18; *p++ = 0x80|(c>>12&0x3F);*p++ = 0x80|(c>>6&0x3F); *p++ = 0x80|(c&0x3F); }
  }
}
