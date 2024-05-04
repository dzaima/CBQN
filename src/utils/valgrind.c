static void printBitDef(u8 val, u8 def) {
  printf("%s", def&1? val&1?"1":"0" : val&1?"¹":"⁰");
}

void vg_printDump_p(char* name, void* data, u64 len) {
  u8 vbits[len];
  int r = VALGRIND_GET_VBITS(data, vbits, len);
  
  if(name!=NULL) printf("%s:\n", name);
  if (r!=1) printf("(failed to get vbits)\n");
  
  for (u64 i = 0; i < len; i++) {
    if (i!=0) printf(i&7? " " : "\n");
    u8 cv = ~vbits[i];
    u8 cd = ((u8*)data)[i];
    VALGRIND_SET_VBITS(&cd, &(u8[]){0}, 1);
    for (i32 j = 7; j >= 0; j--) {
      printBitDef(cd>>j, cv>>j);
    }
  }
  printf("\n");
}

void vg_printDefined_u64(char* name, u64 x) {
  if(name!=NULL) printf("%s: ", name);
  u64 d = vg_getDefined_u64(x);
  u64 xv = x;
  VALGRIND_MAKE_MEM_DEFINED(&xv, 8);
  
  for (i32 i = 63; i >= 0; i--) printBitDef(xv>>i, d>>i);
  printf("\n");
}

u64 vg_rand(u64 x) { // randomize undefined bits in x, and return a value with all bits defined
  u64 d = vg_getDefined_u64(x);
  if (~d == 0) return x;
  return (x & d)  |  (vgRand64() & ~d);
}

B vg_validateResult(B x) {
  if (!isArr(x)) return x;
  void* data;
  u64 len;
  u8 xe = TI(x,elType);
  u64 ia = IA(x);
  if (xe!=el_B) {
    data = tyany_ptr(x);
    if (xe==el_bit) {
      i32 left = ia&63;
      len = (ia>>6)*8;
      if (left) {
        u64 last = ((u64*)data)[len/8];
        u64 exp = (1ULL<<left) - 1;
        u64 got = vg_getDefined_u64(last);
        if ((got&exp) != exp) {
          printf("Expected %d defined trailing bits, got:\n", left);
          vg_printDefined_u64(NULL, last);
          fatal("");
        }
      }
    } else {
      len = elWidth(xe) * ia;
    }
  } else {
    B* xp = arr_bptr(x);
    if (xp==NULL) return x; // can't check unknown type array
    data = xp;
    len = sizeof(B) * ia;
  }
  if (VALGRIND_CHECK_MEM_IS_DEFINED(data, len)) {
    printf("Expected "N64d" defined bytes, got:\n", len);
    vg_printDump_p(NULL, data, len);
    fatal("");
  }
  return x;
}

void g_def(void* data, u64 len) {
  vg_printDump_p(NULL, data, len);
}




u64 loadMask(u64* p, u64 unk, u64 exp, u64 i, u64 pos) {
  if (pos==0) return ~(p[i]^exp);
  u64 res =          loadMask(p, unk, exp, i,     pos<<1);
  if (unk&pos) res&= loadMask(p, unk, exp, i|pos, pos<<1);
  return res;
}

NOINLINE u64 vg_loadLUT64(u64* p, u64 i) {
  u64 unk = ~vg_getDefined_u64(i);
  u64 res = p[vg_withDefined_u64(i, ~0ULL)]; // result value will always be the proper indexing operation
  
  i32 undefCount = POPC(unk);
  if (undefCount>0) {
    if (undefCount>8) fatal("too many unknown bits in index of vg_loadLUT64");
    res = vg_withDefined_u64(res, loadMask(p, unk, res, i & ~unk, 1));
  }
  #if DBG_VG_OVERRIDES
    vg_printDefined_u64("idx", i);
    vg_printDefined_u64("res", res);
  #endif
  return res;
}

NOINLINE u64 vg_pext_u64(u64 src, u64 mask) {
  u64 maskD = vg_getDefined_u64(mask);
  u64 r = vg_undef_u64(0);
  i32 ri = 0;
  u64 undefMask = 0;
  for (i32 i = 0; i < 64; i++) {
    u64 c = 1ull<<i;
    if (!(maskD&c) && undefMask==0) undefMask = (~0ULL)<<ri;
    if (vg_def_u64(mask&c)) r = vg_withBit_u64(r, ri++, (c&src)!=0);
  }
  if (ri<64) r = r & (1ULL<<ri)-1;
  r = vg_withDefined_u64(r, vg_getDefined_u64(r) & ~undefMask);
  #if DBG_VG_OVERRIDES
    printf("pext:\n");
    vg_printDefined_u64("src", src);
    vg_printDefined_u64("msk", mask);
    vg_printDefined_u64("res", r);
    vg_printDefined_u64("exp", _pext_u64(src, mask));
  #endif
  return r;
}

NOINLINE u64 vg_pdep_u64(u64 src, u64 mask) {
  if (0 != ~vg_getDefined_u64(mask)) fatal("pdep impl assumes mask is defined everywhere");
  u64 c = src;
  u64 r = 0;
  for (i32 i = 0; i < 64; i++) {
    if ((mask>>i)&1) {
      r|= (c&1) << i;
      c>>= 1;
    }
  }
  #if DBG_VG_OVERRIDES
    printf("pdep:\n");
    vg_printDefined_u64("src", src);
    vg_printDefined_u64("msk", mask);
    vg_printDefined_u64("res", r);
    vg_printDefined_u64("exp", _pdep_u64(src, mask));
  #endif
  return r;
}

NOINLINE u64 rand_popc64(u64 x) {
  u64 def = vg_getDefined_u64(x);
  if (def==~0ULL) return POPC(x);
  i32 min = POPC(x & def);
  i32 diff = POPC(~def);
  i32 res = min + vgRand64Range(diff);
  #if DBG_VG_OVERRIDES
    printf("popc:\n");
    vg_printDefined_u64("x", x);
    printf("popc in %d-%d; res: %d\n", min, min+diff, res);
  #endif
  return res;
}