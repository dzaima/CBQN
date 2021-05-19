typedef struct TmpFile { // to be turned into a proper I8Arr
  struct Arr;
  i8 a[];
} TmpFile;

TmpFile* file_bytes(B path) { // consumes; may throw
  u64 plen = utf8lenB(path);
  char p[plen+1];
  toUTF8(path, p);
  p[plen] = 0;
  FILE* f = fopen(p, "r");
  if (f==NULL) thrF("Couldn't read file \"%R\"", path);
  fseek(f, 0, SEEK_END);
  u64 len = ftell(f);
  fseek(f, 0, SEEK_SET);
  TmpFile* src = mm_allocN(fsizeof(TmpFile,a,u8,len), t_i8arr);
  arr_shVec(tag(src,ARR_TAG), len);
  fread((char*)src->a, 1, len, f);
  fclose(f);
  dec(path);
  return src;
}
B file_chars(B path) { // consumes; may throw
  TmpFile* c = file_bytes(path);
  B r = fromUTF8((char*)c->a, c->ia);
  ptr_dec(c);
  return r;
}

B path_resolve(B base, B rel) { // consumes rel; may error; assumes base is a char vector or bi_N
  assert((isArr(base) || isNothing(base)) && isArr(rel));
  BS2B rgetU = TI(rel).getU;
  usz ria = a(rel)->ia;
  if (rnk(rel)!=1) thrM("Paths must be character vectors");
  for (usz i = 0; i < ria; i++) if (!isC32(rgetU(rel, i))) thrM("Paths must be character vectors");
  if (ria==0) { dec(rel); return inc(base); }
  if (o2cu(rgetU(rel, 0))=='/') return rel;
  if (isNothing(base)) thrM("Using relative path with no absolute base path known");
  BS2B bgetU = TI(base).getU;
  usz bia = a(base)->ia;
  bool has = bia && o2cu(bgetU(base, bia-1))=='/';
  u32* rp; B r = m_c32arrv(&rp, bia+ria+(has?0:1));
  usz ri = 0;
  for (usz i = 0; i < bia-(has?1:0); i++) rp[ri++] = o2cu(bgetU(base, i));
  rp[ri++] = '/';
  for (usz i = 0; i < ria; i++) rp[ri++] = o2cu(rgetU(rel, i));
  dec(rel);
  return r;
}

B path_dir(B path) { // consumes; returns directory part of file path, with trailing slash; may error
  assert(isArr(path) || isNothing(path));
  if (isNothing(path)) return path;
  BS2B pgetU = TI(path).getU;
  usz pia = a(path)->ia;
  if (pia==0) thrM("Empty file path");
  for (usz i = 0; i < pia; i++) if (!isC32(pgetU(path, i))) thrM("Paths must be character vectors");
  for (i64 i = (i64)pia-1; i >= 0; i--) {
    if (o2cu(pgetU(path, i))=='/') {
      B r = TI(path).slice(path, 0);
      arr_shVec(r, i+1);
      return r;
    }
  }
  dec(path);
  u32* rp; B r = m_c32arrv(&rp, 2); rp[0] = '.'; rp[1] = '/';
  return r;
}