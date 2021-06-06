#include <dirent.h>
#include "../core.h"
#include "file.h"
#include "mut.h"

static FILE* file_open(B path, char* desc, char* mode) { // doesn't consume
  u64 plen = utf8lenB(path);
  TALLOC(char, p, plen+1);
  toUTF8(path, p);
  p[plen] = 0;
  FILE* f = fopen(p, mode);
  TFREE(p);
  if (f==NULL) thrF("Couldn't %S file \"%R\"", desc, path);
  return f;
}
static DIR* dir_open(B path) { // doesn't consume
  u64 plen = utf8lenB(path);
  TALLOC(char, p, plen+1);
  toUTF8(path, p);
  p[plen] = 0;
  DIR* f = opendir(p);
  TFREE(p);
  if (f==NULL) thrF("Couldn't open directory \"%R\"", path);
  return f;
}

TmpFile* file_bytes(B path) { // consumes
  FILE* f = file_open(path, "read", "r");
  fseek(f, 0, SEEK_END);
  u64 len = ftell(f);
  fseek(f, 0, SEEK_SET);
  TmpFile* src = mm_allocN(fsizeof(TmpFile,a,u8,len), t_i8arr);
  arr_shVec(tag(src,ARR_TAG), len);
  if (fread((char*)src->a, 1, len, f)!=len) thrF("Error reading file \"%R\"", path);
  dec(path);
  fclose(f);
  return src;
}
B file_chars(B path) { // consumes
  TmpFile* c = file_bytes(path);
  B r = fromUTF8((char*)c->a, c->ia);
  ptr_dec(c);
  return r;
}

B path_resolve(B base, B rel) { // consumes rel; assumes base is a char vector or bi_N
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

B path_dir(B path) { // consumes; returns directory part of file path with trailing slash, or ·
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


void file_wChars(B path, B x) { // consumes path
  FILE* f = file_open(path, "write to", "w");
  
  u64 len = utf8lenB(x);
  TALLOC(char, val, len);
  toUTF8(x, val);
  
  if (fwrite(val, 1, len, f) != len) thrF("Error writing to file \"%R\"", path);
  TFREE(val);
  dec(path);
  fclose(f);
}
void file_wBytes(B path, B x) { // consumes path
  FILE* f = file_open(path, "write to", "w");
  
  u64 len = a(x)->ia;
  TALLOC(char, val, len);
  BS2B xgetU = TI(x).getU;
  for (u64 i = 0; i < len; i++) val[i] = o2i(xgetU(x,i));
  
  if (fwrite(val, 1, len, f) != len) thrF("Error writing to file \"%R\"", path);
  TFREE(val);
  dec(path);
  fclose(f);
}

B file_list(B path) {
  DIR* d = dir_open(path);
  struct dirent *c;
  B res = inc(bi_emptySVec);
  while ((c = readdir(d)) != NULL) {
    char* name = c->d_name;
    if (name[0]=='.'? !(name[1]==0 || (name[1]=='.'&&name[2]==0)) : true) res = vec_add(res, m_str8l(name));
  }
  dec(path);
  return res;
}
