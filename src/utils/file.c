#include <dirent.h>
#include "../core.h"
#include "file.h"
#include "mut.h"
#include "talloc.h"

FILE* file_open(B path, char* desc, char* mode) { // doesn't consume
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

I8Arr* file_bytes(B path) { // consumes
  FILE* f = file_open(path, "read", "r");
  fseek(f, 0, SEEK_END);
  u64 len = ftell(f);
  fseek(f, 0, SEEK_SET);
  I8Arr* src = m_arr(fsizeof(I8Arr,a,u8,len), t_i8arr, len); arr_shVec((Arr*)src);
  if (fread((char*)src->a, 1, len, f)!=len) thrF("Error reading file \"%R\"", path);
  dec(path);
  fclose(f);
  return src;
}
B file_chars(B path) { // consumes
  return fromUTF8a(file_bytes(path));
}
B file_lines(B path) { // consumes
  I8Arr* tf = file_bytes(path);
  usz ia = tf->ia; u8* p = (u8*)tf->a;
  usz lineCount = 0;
  for (usz i = 0; i < ia; i++) {
    if (p[i]=='\n') lineCount++;
    else if (p[i]=='\r') {
      lineCount++;
      if(i+1<ia && p[i+1]=='\n') i++;
    }
  }
  if (ia && (p[ia-1]!='\n' && p[ia-1]!='\r')) lineCount++;
  usz i = 0;
  HArr_p r = m_harrs(lineCount, &i);
  usz pos = 0;
  while (i < lineCount) {
    usz spos = pos;
    while(pos<ia && p[pos]!='\n' && p[pos]!='\r') pos++;
    r.a[i++] = fromUTF8((char*)p+spos, pos-spos);
    if (p[pos]=='\r' && pos+1<ia && p[pos+1]=='\n') pos+= 2;
    else pos++;
  }
  ptr_dec(tf);
  return harr_fv(r);
}

I8Arr* stdin_allBytes() {
  B r = emptyIVec();
  u64 SZ = 8192;
  TALLOC(i8, t, SZ);
  while(true) {
    u64 read = fread(t, 1, SZ, stdin);
    if (read==0) break;
    i8* ap; B a = m_i8arrv(&ap, read);
    memcpy(ap, t, read);
    r = vec_join(r, a);
  }
  TFREE(t);
  return toI8Arr(r);
}



B path_resolve(B base, B rel) { // consumes rel; assumes base is a char vector or bi_N
  assert((isArr(base) || q_N(base)));
  if (!isArr(rel) || rnk(rel)!=1) thrM("Paths must be character vectors");
  SGetU(rel)
  usz ria = a(rel)->ia;
  if (rnk(rel)!=1) thrM("Paths must be character vectors");
  for (usz i = 0; i < ria; i++) if (!isC32(GetU(rel, i))) thrM("Paths must be character vectors");
  if (ria>0 && o2cu(GetU(rel, 0))=='/') return rel;
  if (q_N(base)) thrM("Using relative path with no absolute base path known");
  if (ria==0) { dec(rel); return inc(base); }
  SGetU(base)
  usz bia = a(base)->ia;
  bool has = bia && o2cu(GetU(base, bia-1))=='/';
  u32* rp; B r = m_c32arrv(&rp, bia+ria+(has?0:1));
  usz ri = 0;
  for (usz i = 0; i < bia-(has?1:0); i++) rp[ri++] = o2cu(GetU(base, i));
  rp[ri++] = '/';
  for (usz i = 0; i < ria; i++) rp[ri++] = o2cu(GetU(rel, i));
  dec(rel);
  return r;
}

B path_dir(B path) { // consumes; returns directory part of file path with trailing slash, or ·
  assert(isArr(path) || q_N(path));
  if (q_N(path)) return path;
  SGetU(path)
  usz pia = a(path)->ia;
  if (pia==0) thrM("Empty file path");
  for (usz i = 0; i < pia; i++) if (!isC32(GetU(path, i))) thrM("Paths must be character vectors");
  for (i64 i = (i64)pia-1; i >= 0; i--) {
    if (o2cu(GetU(path, i))=='/') {
      Arr* r = TI(path,slice)(path, 0, i+1); arr_shVec(r);
      return taga(r);
    }
  }
  dec(path);
  u32* rp; B r = m_c32arrv(&rp, 2); rp[0] = '.'; rp[1] = '/';
  return r;
}

B path_abs(B path) {
  if (q_N(path)) return path;
  u64 plen = utf8lenB(path);
  TALLOC(char, p, plen+1);
  toUTF8(path, p);
  p[plen] = 0;
  char* res = realpath(p, NULL);
  if (res==NULL) thrF("Failed to convert %R to absolute path", path);
  B r = fromUTF8l(res);
  free(res);
  dec(path);
  TFREE(p);
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
  SGetU(x)
  for (u64 i = 0; i < len; i++) val[i] = o2i(GetU(x,i));
  
  if (fwrite(val, 1, len, f) != len) thrF("Error writing to file \"%R\"", path);
  TFREE(val);
  dec(path);
  fclose(f);
}

B file_list(B path) {
  DIR* d = dir_open(path);
  struct dirent *c;
  B res = emptySVec();
  while ((c = readdir(d)) != NULL) {
    char* name = c->d_name;
    if (name[0]=='.'? !(name[1]==0 || (name[1]=='.'&&name[2]==0)) : true) res = vec_add(res, m_str8l(name));
  }
  dec(path);
  return res;
}
