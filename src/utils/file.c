#include "../core.h"
#include "file.h"
#include "talloc.h"
#include "cstr.h"
#include <dirent.h>
#include <unistd.h>
#include <errno.h>

#if defined(_WIN32) || defined(_WIN64)
  #include "../windows/realpath.c"
#endif


FILE* file_open(B path, char* desc, char* mode) { // doesn't consume
  char* p = toCStr(path);
  FILE* f = fopen(p, mode);
  freeCStr(p);
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


I8Arr* stream_bytes(FILE* f) {
  B r = emptyIVec();
  u64 SZ = 8192;
  TALLOC(i8, t, SZ);
  while(true) {
    u64 read = fread(t, 1, SZ, f);
    if (read==0) break;
    i8* ap; B a = m_i8arrv(&ap, read);
    memcpy(ap, t, read);
    r = vec_join(r, a);
  }
  TFREE(t);
  return toI8Arr(r);
}

I8Arr* path_bytes(B path) { // consumes
  FILE* f = file_open(path, "read", "r");
  int seekRes = fseek(f, 0, SEEK_END);
  I8Arr* src;
  if (seekRes==-1) {
    src = stream_bytes(f);
  } else {
    i64 len = ftell(f);
    CHECK_IA(len, 1);
    #if USZ_64
      if(len > USZ_MAX) { fclose(f); thrOOM(); }
    #else
      if (len > ((1LL<<31) - 1000)) { fclose(f); thrOOM(); }
    #endif
    fseek(f, 0, SEEK_SET);
    src = m_arr(fsizeof(I8Arr,a,u8,len), t_i8arr, len); arr_shVec((Arr*)src); // TODO fclose file if allocation fails (+ elsewhere too)
    if (fread((char*)src->a, 1, len, f)!=len) {
      fclose(f);
      thrF("Error reading file \"%R\"", path);
    }
  }
  dec(path);
  fclose(f);
  return src;
}
B path_chars(B path) { // consumes
  return utf8DecodeA(path_bytes(path));
}
B path_lines(B path) { // consumes; TODO rewrite this, it's horrible
  I8Arr* tf = path_bytes(path);
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
  M_HARR(r, lineCount)
  usz pos = 0;
  for (usz i = 0; i < lineCount; i++) {
    usz spos = pos;
    while(pos<ia && p[pos]!='\n' && p[pos]!='\r') pos++;
    HARR_ADD(r, i, utf8Decode((char*)p+spos, pos-spos));
    if (pos<ia && p[pos]=='\r' && pos+1<ia && p[pos+1]=='\n') pos+= 2;
    else pos++;
  }
  ptr_dec(tf);
  return HARR_FV(r);
}


static NOINLINE void guaranteeStr(B x) { // assumes x is an array
  if (elChr(TI(x,elType))) return;
  usz xia = IA(x);
  SGetU(x)
  for (usz i = 0; i < xia; i++) if (!isC32(GetU(x, i))) thrM("Paths must be character vectors");
}


B path_rel(B base, B rel) { // consumes rel; assumes base is a char vector or bi_N
  assert(isArr(base) || q_N(base));
  if (!isArr(rel) || RNK(rel)!=1) thrM("Paths must be character vectors");
  SGetU(rel)
  usz ria = IA(rel);
  if (RNK(rel)!=1) thrM("Paths must be character vectors");
  guaranteeStr(rel);
  if (ria>0 && o2cG(GetU(rel, 0))=='/') return rel;
  if (q_N(base)) thrM("Using relative path with no absolute base path known");
  if (ria==0) { dec(rel); return incG(base); }
  usz bia = IA(base);
  if (bia==0) return rel;
  SGetU(base)
  bool has = o2cG(GetU(base, bia-1))=='/';
  u32* rp; B r = m_c32arrv(&rp, bia+ria+(has?0:1));
  usz ri = 0;
  for (usz i = 0; i < bia-(has?1:0); i++) rp[ri++] = o2cG(GetU(base, i));
  rp[ri++] = '/';
  for (usz i = 0; i < ria; i++) rp[ri++] = o2cG(GetU(rel, i));
  dec(rel);
  return r;
}

B path_parent(B path) {
  assert(isArr(path));
  SGetU(path)
  usz pia = IA(path);
  if (pia==0) thrM("Empty file path");
  guaranteeStr(path);
  for (i64 i = (i64)pia-2; i >= 0; i--) {
    if (o2cG(GetU(path, i))=='/') return taga(arr_shVec(TI(path,slice)(path, 0, i+1)));
  }
  if (o2cG(GetU(path, 0))=='/') return path;
  dec(path);
  u32* rp; B r = m_c32arrv(&rp, 2); rp[0] = '.'; rp[1] = '/';
  return r;
}
B path_name(B path) {
  assert(isArr(path));
  SGetU(path)
  usz pia = IA(path);
  if (pia==0) thrM("Empty file path");
  guaranteeStr(path);
  for (i64 i = (i64)pia-1; i >= 0; i--) {
    if (o2cG(GetU(path, i))=='/') {
      if (i == pia-1) thrF("File path ended with a slash: \"%R\"", path);
      return taga(arr_shVec(TI(path,slice)(path, i+1, pia - (i+1))));
    }
  }
  return path;
}

B path_abs(B path) {
  #if WASM
  return path; // lazy
  #else
  if (q_N(path)) return path;
  u64 plen = utf8lenB(path);
  TALLOC(char, p, plen+1);
  toUTF8(path, p);
  p[plen] = 0;
  char* res = realpath(p, NULL);
  if (res==NULL) thrF("Failed to convert %R to absolute path", path);
  B r = utf8Decode0(res);
  free(res);
  dec(path);
  TFREE(p);
  return r;
  #endif
}

CharBuf get_chars(B x) {
  char* buf;
  bool alloc;
  u64 len = IA(x);
  u8 el = TI(x,elType);
  if (el==el_i8 || el==el_c8) {
    buf = tyany_ptr(x);
    alloc = false;
  } else {
    buf = TALLOCP(char, len);
    alloc = true;
    SGetU(x)
    for (u64 i = 0; i < len; i++) {
      B c = GetU(x,i);
      buf[i] = isNum(c)? o2iG(c) : o2c(c);
    }
  }
  return (CharBuf){.data=buf, .alloc=alloc};
}
void free_chars(CharBuf b) {
  if (b.alloc) TFREE(b.data);
}

void path_wChars(B path, B x) { // consumes path
  FILE* f = file_open(path, "write to", "w");
  
  u64 len = utf8lenB(x);
  TALLOC(char, val, len);
  toUTF8(x, val);
  
  if (fwrite(val, 1, len, f) != len) { fclose(f); thrF("Error writing to file \"%R\"", path); }
  TFREE(val);
  dec(path);
  fclose(f);
}
void file_wBytes(FILE* f, B name, B x) {
  u64 len = IA(x);
  
  CharBuf buf = get_chars(x);
  
  if (fwrite(buf.data, 1, len, f) != len) {
    if (q_N(name)) thrM("Error writing to file");
    else thrF("Error writing to file \"%R\"", name);
  }
  
  free_chars(buf);
}
void path_wBytes(B path, B x) { // consumes path
  FILE* f = file_open(path, "write to", "w");
  file_wBytes(f, path, x);
  fclose(f);
  dec(path);
}

B path_list(B path) {
  DIR* d = dir_open(path);
  struct dirent *c;
  B res = emptySVec();
  while ((c = readdir(d)) != NULL) {
    char* name = c->d_name;
    if (name[0]=='.'? !(name[1]==0 || (name[1]=='.'&&name[2]==0)) : true) res = vec_addN(res, utf8Decode(name, strlen(name)));
  }
  closedir(d);
  dec(path);
  return res;
}

#if __has_include(<sys/mman.h>) && __has_include(<fcntl.h>) && __has_include(<unistd.h>) && !WASM && !NO_MMAP

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct MmapHolder {
  struct Arr;
  int fd;
  u64 size;
  u8* a;
} MmapHolder;

void mmapH_visit(Value* v) { }
DEF_FREE(mmapH) {
  MmapHolder* p = (MmapHolder*)x;
  if (munmap(p->a, p->size)) thrF("Failed to unmap: %S", strerror(errno));
  if (close(p->fd)) thrF("Failed to close file: %S", strerror(errno));
}

B info_c1(B,B);
static Arr* mmapH_slice(B x, usz s, usz ia) {
  TySlice* r = m_arr(sizeof(TySlice), t_c8slice, ia);
  r->a = c(MmapHolder,x)->a + s;
  r->p = a(x);
  return (Arr*)r;
}

B mmap_file(B path) {
  char* p = toCStr(path);
  dec(path);
  int fd = open(p, 0);
  freeCStr(p);
  if (fd==-1) thrF("Failed to open file: %S", strerror(errno));
  u64 len = lseek(fd, 0, SEEK_END);
  
  u8* data = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0); // TODO count in some memory usage counter
  if (data==MAP_FAILED) {
    close(fd);
    thrM("failed to mmap file");
  }
  
  MmapHolder* holder = m_arr(sizeof(MmapHolder), t_mmapH, len);
  holder->fd = fd;
  holder->a = data;
  holder->size = len;
  arr_shVec((Arr*)holder);
  return taga(arr_shVec(mmapH_slice(taga(holder), 0, len)));
}

B mmapH_get(Arr* a, usz pos) { thrM("Reading mmapH directly"); }

void mmap_init() {
  TIi(t_mmapH,get)   = mmapH_get;
  TIi(t_mmapH,getU)  = mmapH_get;
  TIi(t_mmapH,slice) = mmapH_slice;
  TIi(t_mmapH,freeO) = mmapH_freeO;
  TIi(t_mmapH,freeF) = mmapH_freeF;
  TIi(t_mmapH,visit) = noop_visit;
  TIi(t_mmapH,print) = farr_print;
  TIi(t_mmapH,isArr) = true;
  TIi(t_mmapH,arrD1) = true;
  TIi(t_mmapH,elType) = el_c8;
  // use default canStore
}
#else
B mmap_file(B path) {
  thrM("CBQN was compiled without •file.MapBytes support");
}
void mmap_init() { }
#endif

#include <sys/types.h>
#include <sys/stat.h>

bool dir_create(B path) {
  char* p = toCStr(path);
  #if defined(_WIN32) || defined(_WIN64)
    bool r = 1;
  #else
    bool r = mkdir(p, S_IRWXU) == 0;
  #endif
  freeCStr(p);
  return r;
}

bool path_rename(B old_path, B new_path) {
  char* old = toCStr(old_path);
  char* new = toCStr(new_path);
  // TODO Fix race condition, e.g., with renameat2 on Linux, etc.
  bool ok = access(new, F_OK) != 0 && rename(old, new) == 0;
  freeCStr(new);
  freeCStr(old);
  dec(old_path);
  return ok;
}

bool path_remove(B path) {
  char* p = toCStr(path);
  bool ok = unlink(p) == 0;
  freeCStr(p);
  dec(path);
  return ok;
}

int path_stat(struct stat* s, B path) { // doesn't consume; get stat of s; errors if path isn't string; returns non-zero on failure
  char* p = toCStr(path);
  int r = stat(p, s);
  freeCStr(p);
  return r;
}

char path_type(B path) {
  struct stat s;
  int r = path_stat(&s, path);
  dec(path);
  
  if (r==-1) return 0;
  i64 mode = s.st_mode;
  if (S_ISREG (mode)) return 'f';
  if (S_ISDIR (mode)) return 'd';
  if (S_ISLNK (mode)) return 'l';
  if (S_ISFIFO(mode)) return 'p';
  if (S_ISSOCK(mode)) return 's';
  if (S_ISBLK (mode)) return 'b';
  if (S_ISCHR (mode)) return 'c';
  thrM("Unexpected file type");
}


static B get_timespec(struct timespec ts) { return m_f64(ts.tv_sec + ts.tv_nsec*1e-9); }
#if (_POSIX_C_SOURCE >= 200809L) && defined(st_mtime)
  #define GET_TIME(C) get_timespec(s.st_##C##tim);
#elif defined(__APPLE__)
  #define GET_TIME(C) get_timespec(s.st_##C##timespec);
#else
  #define GET_TIME(C) m_f64(s.st_##C##time);
#endif

B path_info(B path, i32 mode) {
  struct stat s;
  int r = path_stat(&s, path);
  if (r==-1) thrF("Failed to access file \"%R\": %S", path, strerror(errno));
  dec(path);
  if (mode==0) return GET_TIME(c);
  if (mode==1) return GET_TIME(a);
  if (mode==2) return GET_TIME(m);
  if (mode==3) return m_f64(s.st_size);
  thrM("Unknown path_info mode");
}

void mmX_dumpHeap(FILE* f);
NOINLINE void writeNum(FILE* f, u64 v, i32 len) {
  u8 buf[8];
  for (i32 i = 0; i < len; i++) buf[i] = (v>>(8*i)) & 0xff;
  fwrite(buf, 1, len, f);
}
static char* types_str[] = {
  #define F(X) #X,
  FOR_TYPE(F)
  #undef F
  NULL
};
static u8 types_val[] = {
  #define F(X) t_##X,
  FOR_TYPE(F)
  #undef F
};

void cbqn_heapDump() {
  char* name = "CBQNHeapDump";
  FILE* f = fopen(name, "w");
  if (f==NULL) {
    fprintf(stderr, "Failed to dump heap - could not open file for writing\n");
    return;
  }
  // fwrite(&size, 8, 1, f);
  u8 t8 = 1;
  fwrite(&t8, 1, 1, f); // version
  fwrite("CBQN", 1, 5, f);
  
  writeNum(f, sizeof(ur ), 4);
  writeNum(f, sizeof(usz), 4);
  #if WASM
    writeNum(f, (u64)~0ULL, 8);
  #else
    writeNum(f, getpid(), 8);
  #endif
  
  { // t_names
    for (i32 i = 0; types_str[i]; i++) {
      t8=types_val[i]; fwrite(&t8, 1, 1, f);
      char* s = types_str[i]; fwrite(s, 1, strlen(s)+1, f);
    }
  }
  t8 = 255; fwrite(&t8, 1, 1, f); // end of t_names
  
  t8 = 12; fwrite(&t8, 1, 1, f); // number of tag names
  u8 t16a[2];
  #define F(X) { \
    t16a[0] = X##_TAG&0xff; t16a[1] = (X##_TAG>>8)&0xff; fwrite(&t16a, 1, 2, f); \
    char* s = #X"_TAG"; fwrite(s, 1, strlen(s)+1, f); \
  }
  F(C32)F(TAG)F(VAR)F(EXT)F(RAW)F(MD1)F(MD2)F(FUN)F(NSP)F(OBJ)F(ARR)F(VAL)
  #undef F
  
  mm_dumpHeap(f);
  mmX_dumpHeap(f);
  fprintf(stderr, "Heap dumped to \"%s\"\n", name);
  fclose(f);
}
