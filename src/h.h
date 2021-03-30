#pragma once
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdarg.h>
#define i8  int8_t
#define u8 uint8_t
#define i16  int16_t
#define u16 uint16_t
#define i32  int32_t
#define u32 uint32_t
#define i64  int64_t
#define u64 uint64_t
#define f64 double
#define I32_MAX ((i32)((1LL<<31)-1))
#define U16_MAX ((u16)-1)
#define UD __builtin_unreachable();

#ifdef DEBUG
  #include<assert.h>
  #define VALIDATE(x) validate(x) // preferred validating level
#else
  #define assert(x) {if (!(x)) __builtin_unreachable();}
  #define VALIDATE(x) x
#endif

#define fsizeof(T,F,E,n) (offsetof(T, F) + sizeof(E)*n) // type; FAM name; FAM type; amount
#define usz u32
#define ur u8
#define ftag(x) ((u64)(x) << 48)
#define tag(v, t) b(((u64)(v)) | ftag(t))
                                  // .111111111110000000000000000000000000000000000000000000000000000 infinity
                                  // .111111111111000000000000000000000000000000000000000000000000000 qNaN
                                  // .111111111110nnn................................................ sNaN aka tagged aka not f64, if nnn≠0
                                  // 0111111111110................................................... direct value with no need of refcounting
u16 C32_TAG = 0b0111111111110001; // 0111111111110001................00000000000ccccccccccccccccccccc char
u16 TAG_TAG = 0b0111111111110010; // 0111111111110010................nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn special value (0=nothing, 1=undefined var, 2=bad header; 3=optimized out; 4=error?)
u16 VAR_TAG = 0b0111111111110011; // 0111111111110011ddddddddddddddddnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn variable reference
u16 I32_TAG = 0b0111111111110111; // 0111111111110111................nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn 32-bit int; unused
u16 MD1_TAG = 0b1111111111110010; // 1111111111110010ppppppppppppppppppppppppppppppppppppppppppppp000 1-modifier
u16 MD2_TAG = 0b1111111111110011; // 1111111111110011ppppppppppppppppppppppppppppppppppppppppppppp000 2-modifier
u16 FUN_TAG = 0b1111111111110100; // 1111111111110100ppppppppppppppppppppppppppppppppppppppppppppp000 function
u16 NSP_TAG = 0b1111111111110101; // 1111111111110101ppppppppppppppppppppppppppppppppppppppppppppp000 namespace maybe?
u16 OBJ_TAG = 0b1111111111110110; // 1111111111110110ppppppppppppppppppppppppppppppppppppppppppppp000 custom object (e.g. bigints)
u16 ARR_TAG = 0b1111111111110111; // 1111111111110111ppppppppppppppppppppppppppppppppppppppppppppp000 array (everything else is an atom)
u16 VAL_TAG = 0b1111111111110   ; // 1111111111110................................................... pointer to Value, needs refcounting

enum Type {
  /* 0*/ t_empty, // empty bucket placeholder
  /* 1*/ t_fun_def, t_fun_block,
  /* 3*/ t_md1_def, t_md1_block,
  /* 5*/ t_md2_def, t_md2_block,
  /* 7*/ t_noGC, // doesn't get visited, shouldn't be unallocated by gc
  
  /* 8*/ t_fork, t_atop,
  /*10*/ t_md1D, t_md2D, t_md2H,
  
  /*13*/ t_harr, t_i32arr,
  /*15*/ t_hslice, t_i32slice,
  
  /*17*/ t_comp, t_block, t_body, t_scope,
  
  
  Type_MAX
};

enum PrimFns {
  pf_none,
  pf_add, pf_sub, pf_mul, pf_div, pf_pow, pf_floor, pf_eq, pf_le, pf_log, // arith.c
  pf_shape, pf_pick, pf_ud, pf_pair, pf_fne, pf_lt, pf_rt, // sfns.c
  pf_fork, pf_atop, pf_md1d, pf_md2d, // derv.c
  pf_type, pf_decp, pf_primInd, pf_glyph, pf_fill, pf_grLen, pf_grOrd, pf_asrt, // sysfn.c
};
char* format_pf(u8 u) {
  switch(u) {
    default: case pf_none: return"(unknown fn)";
    case pf_add:return"+"; case pf_sub:return"-"; case pf_mul:return"×"; case pf_div:return"÷"; case pf_pow:return"⋆"; case pf_floor:return"⌊"; case pf_eq:return"="; case pf_le:return"≤"; case pf_log:return"⋆⁼";
    case pf_shape:return"⥊"; case pf_pick:return"⊑"; case pf_ud:return"↕"; case pf_pair:return"{𝕨‿𝕩}"; case pf_fne:return"≢"; case pf_lt:return"⊣"; case pf_rt:return"⊢";
    case pf_fork:return"(fork)"; case pf_atop:return"(atop)"; case pf_md1d:return"(derived 1-modifier)"; case pf_md2d:return"(derived 2-modifier)";
    case pf_type:return"•Type"; case pf_decp:return"•Decompose"; case pf_primInd:return"•PrimInd"; case pf_glyph:return"•Glyph"; case pf_fill:return"•FillFn"; case pf_grLen:return"•GroupLen"; case pf_grOrd:return"•GroupOrd"; case pf_asrt:return"!";  }
}
enum PrimMd1 {
  pm1_none,
  pm1_tbl, pm1_scan, // md1.c
};
char* format_pm1(u8 u) {
  switch(u) {
    default: case pf_none: return"(unknown 1-modifier)";
    case pm1_tbl: return"⌜"; case pm1_scan: return"`";
  }
}

typedef union B {
  u64 u;
  i64 s;
  f64 f;
} B;
#define b(x) ((B)(x))

typedef struct Value {
  i32 refc;
  u16 flags; // incl GC stuff when that's a thing, possibly whether is sorted/a permutation/whatever, bucket size, etc
  u8 type; // needed globally so refc-- and GC know what to visit
  ur extra; // whatever object-specific stuff. Rank for arrays, id for functions
} Value;
typedef struct Arr {
  struct Value;
  usz ia;
  usz* sh;
} Arr;

// memory manager
B     mm_alloc (usz sz, u8 type, u64 tag);
void* mm_allocN(usz sz, u8 type);
void mm_free(Value* x);
void mm_visit(B x);

// some primitive actions
void dec(B x);
void inc(B x);
void ptr_dec(void* x);
void ptr_inc(void* x);
void print(B x);
void arr_print(B x);
B m_v1(B a               );
B m_v2(B a, B b          );
B m_v3(B a, B b, B c     );
B m_v4(B a, B b, B c, B d);

#define c(T,x) ((T*)((x).u&0xFFFFFFFFFFFFull))
#define VT(x,t) assert(isVal(x) && v(x)->type==t)
Value* v(B x) { return c(Value, x); }
Arr*   a(B x) { return c(Arr  , x); }
#define  rnk(x  ) (v(x)->extra) // expects argument to be Arr
#define srnk(x,v) (x)->extra=(v)

void print_vmStack();
#ifdef DEBUG
  B validate(B x);
  B recvalidate(B x);
#else
  #define validate(x) x
  #define recvalidate(x) x
#endif
B err(char* s) {
  puts(s); fflush(stdout);
  print_vmStack();
  exit(1);
}

// tag checks
#ifdef ATOM_I32
bool isI32(B x) { return (x.u>>48) == I32_TAG; }
#else
bool isI32(B x) { return false; }
#endif
bool isFun(B x) { return (x.u>>48) == FUN_TAG; }
bool isArr(B x) { return (x.u>>48) == ARR_TAG; }
bool isC32(B x) { return (x.u>>48) == C32_TAG; }
bool isVar(B x) { return (x.u>>48) == VAR_TAG; }
bool isMd1(B x) { return (x.u>>48) == MD1_TAG; }
bool isMd2(B x) { return (x.u>>48) == MD2_TAG; }
bool isMd (B x) { return (x.u>>49) ==(MD2_TAG>>1); }
bool isNsp(B x) { return (x.u>>48) == NSP_TAG; }
bool isObj(B x) { return (x.u>>48) == OBJ_TAG; }
// bool isVal(B x) { return ((x.u>>51) == VAL_TAG)  &  ((x.u<<13) != 0); }
// bool isF64(B x) { return ((x.u>>51&0xFFF) != 0xFFE)  |  ((x.u<<1)==(b(1.0/0.0).u<<1)); }
bool isVal(B x) { return (x.u - (((u64)VAL_TAG<<51) + 1)) < ((1ull<<51) - 1); } // ((x.u>>51) == VAL_TAG)  &  ((x.u<<13) != 0);
bool isF64(B x) { return (x.u<<1) - ((0xFFEull<<52) + 2) >= (1ull<<52) - 2; }

bool isAtm(B x) { return !isVal(x); }

// shape mess
typedef struct ShArr {
  struct Value;
  usz a[];
} ShArr;
usz* allocSh(ur r) {
  assert(r>0);
  B x = mm_alloc(fsizeof(ShArr, a, usz, r), t_noGC, ftag(OBJ_TAG));
  return ((ShArr*)v(x))->a;
}
ShArr* shObj(B x) { return (ShArr*)((u64)a(x)->sh-offsetof(ShArr,a)); }
void decSh(B x) { if (rnk(x)>1) ptr_dec(shObj(x)); }

void arr_shVec(B x, usz ia) {
  a(x)->ia = ia;
  v(x)->extra = 1;
  a(x)->sh = &a(x)->ia;
}
usz* arr_shAlloc(B x, usz ia, usz r) {
  a(x)->ia = ia;
  a(x)->extra = r;
  if (r>1) return a(x)->sh = allocSh(r);
  a(x)->sh = &a(x)->ia;
  return 0;
}
void arr_shCopy(B n, B o) { // copy shape from o to n
  assert(isArr(o));
  a(n)->ia = a(o)->ia;
  ur r = a(n)->extra = rnk(o);
  if (r<=1) {
    a(n)->sh = &a(n)->ia;
  } else {
    a(n)->sh = a(o)->sh;
    ptr_inc(shObj(o));
  }
}
bool shEq(B w, B x) { assert(isArr(w)); assert(isArr(x));
  ur wr = rnk(w); usz* wsh = a(w)->sh;
  ur xr = rnk(x); usz* xsh = a(x)->sh;
  if (wr!=xr) return false;
  if (wsh==xsh) return true;
  return memcmp(wsh,xsh,wr*sizeof(usz))==0;
}
usz arr_csz(B x) {
  ur xr = rnk(x);
  if (xr<=1) return 1;
  usz* sh = a(x)->sh;
  usz r = 1;
  for (i32 i = 1; i < xr; i++) r*= sh[i];
  return r;
}

// make objects
B m_arr(usz min, u8 type) { return mm_alloc(min, type, ftag(ARR_TAG)); }
B m_f64(f64 n) { assert(isF64(b(n))); return b(n); } // assert just to make sure we're actually creating a float
B m_c32(i32 n) { return tag(n, C32_TAG); } // TODO check validity?
#ifdef ATOM_I32
B m_i32(i32 n) { return tag(n, I32_TAG); }
#else
B m_i32(i32 n) { return m_f64(n); }
#endif
B m_error() { return tag(4, TAG_TAG); }
B m_usz(usz n) { return n==(i32)n? m_i32(n) : m_f64(n); }

i32 o2i  (B x) { if ((i32)x.f!=x.f) err("o2i"": expected integer"); return (i32)x.f; }
usz o2s  (B x) { if ((usz)x.f!=x.f) err("o2s"": expected integer"); return (usz)x.f; }
i64 o2i64(B x) { if ((i64)x.f!=x.f) err("o2i64: expected integer"); return (i64)x.f; }



typedef struct Slice {
  struct Arr;
  B p;
} Slice;
void slice_free(B x) { dec(c(Slice,x)->p); decSh(x); }
void slice_print(B x) { arr_print(x); }



typedef void (*B2V)(B);
typedef B (* BS2B)(B, usz);
typedef B (*BSS2B)(B, usz, usz);
typedef B (*    B2B)(B);
typedef B (*   BB2B)(B, B);
typedef B (*  BBB2B)(B, B, B);
typedef B (* BBBB2B)(B, B, B, B);
typedef B (*BBBBB2B)(B, B, B, B, B);

typedef struct TypeInfo {
  B2V free;   // expects refc==0
  B2V visit;  // for GC when that comes around
  B2V print;  // doesn't consume
  BS2B get;   // increments result, doesn't consume arg
  BB2B  m1_d; // consume all args
  BBB2B m2_d; // consume all args
  B2B decompose; // consumes; must return a HArr
  BS2B slice; // consumes; create slice from given starting position; add ia, rank, shape yourself
} TypeInfo;
TypeInfo ti[Type_MAX];
#define TI(x) (ti[v(x)->type])


void do_nothing(B x) { }
void def_print(B x) { printf("(type %d)", v(x)->type); }
B    get_self(B x, usz n) { return x; }
B    def_m1_d(B m, B f     ) { return err("cannot derive this"); }
B    def_m2_d(B m, B f, B g) { return err("cannot derive this"); }
B    def_slice(B x, usz s) { return err("cannot slice non-array!"); }
B    def_decompose(B x) { return m_v2(m_i32((isFun(x)|isMd(x))? 0 : -1),x); }

B bi_nothing, bi_noVar, bi_badHdr, bi_optOut;
void hdr_init() {
  for (i32 i = 0; i < Type_MAX; ++i) {
    ti[i].visit = ti[i].free = do_nothing;
    ti[i].get = get_self;
    ti[i].print = def_print;
    ti[i].m1_d = def_m1_d;
    ti[i].m2_d = def_m2_d;
    ti[i].decompose = def_decompose;
    ti[i].slice = def_slice;
  }
  bi_nothing = tag(0, TAG_TAG);
  bi_noVar   = tag(1, TAG_TAG);
  bi_badHdr  = tag(2, TAG_TAG);
  bi_optOut  = tag(3, TAG_TAG);
}

bool isNothing(B b) { return b.u==bi_nothing.u; }


// refcount
void dec(B x) {
  if (!isVal(VALIDATE(x))) return;
  Value* vx = v(x);
  assert(vx->refc>0);
  if (!--vx->refc) {
    // printf("freeing type %d: %p\n", vx->type, (void*)x.u);fflush(stdout);
    ti[vx->type].free(x);
    mm_free(vx);
  }
}
void inc (B x) { if (isVal(VALIDATE(x))) v(x)->refc++; }
B    inci(B x) { inc(x); return x; }
void ptr_dec(void* x) { dec(tag(x, OBJ_TAG)); }
void ptr_inc(void* x) { inc(tag(x, OBJ_TAG)); }
bool reusable(B x) { return v(x)->refc==1; }


void printUTF8(u32 c);

void print(B x) {
  if (isF64(x)) {
    printf("%g", x.f);
  } else if (isC32(x)) {
    if ((u32)x.u>=32) { printf("'"); printUTF8((u32)x.u); printf("'"); }
    else printf("\\x%x", (u32)x.u);
  } else if (isI32(x)) {
    printf("%d", (i32)x.u);
  } else if (isVal(x)) {
    #ifdef DEBUG
    if (isVal(x) && v(x)->refc<0) {printf("(FREED)"); exit(1);} else
    #endif
    TI(x).print(x);
  }
  else if (isVar(x)) printf("(var d=%d i=%d)", (u16)(x.u>>32), (i32)x.u);
  else if (x.u==bi_nothing.u) printf("·");
  else if (x.u==bi_optOut.u) printf("(value optimized out)");
  else if (x.u==bi_noVar.u) printf("(unset variable placeholder)");
  else if (x.u==bi_badHdr.u) printf("(bad header note)");
  else printf("(todo tag %lx)", x.u>>48);
}



typedef struct Fun {
  struct Value;
  BB2B c1;
  BBB2B c2;
} Fun;

B c1_invalid(B f,      B x) { return err("This function can't be called monadically"); }
B c2_invalid(B f, B w, B x) { return err("This function can't be called dyadically"); }

B c1(B f, B x) { // BQN-call f monadically; consumes x
  if (isFun(f)) return VALIDATE(c(Fun,f)->c1(f, x));
  dec(x);
  if (isMd(f)) return err("Calling a modifier");
  return inci(VALIDATE(f));
}
B c2(B f, B w, B x) { // BQN-call f dyadically; consumes w,x
  if (isFun(f)) return VALIDATE(c(Fun,f)->c2(f, w, x));
  dec(w);dec(x);
  if (isMd(f)) return err("Calling a modifier");
  return inci(VALIDATE(f));
}



typedef struct Md1 {
  struct Value;
  BBB2B  c1; // f(m,f,  x); consumes x
  BBBB2B c2; // f(m,f,w,x); consumes w,x
} Md1;
typedef struct Md2 {
  struct Value;
  BBBB2B  c1; // f(m,f,g,  x); consumes x
  BBBBB2B c2; // f(m,f,g,w,x); consumes w,x
} Md2;


void arr_print(B x) {
  usz r = rnk(x);
  BS2B xget = TI(x).get;
  usz ia = a(x)->ia;
  if (r!=1) {
    if (r==0) {
      printf("<");
      print(xget(x,0));
      return;
    }
    usz* sh = a(x)->sh;
    for (i32 i = 0; i < r; i++) {
      if(i==0)printf("%d",sh[i]);
      else printf("‿%d",sh[i]);
    }
    printf("⥊");
  } else if (ia>0) {
    for (usz i = 0; i < ia; i++) {
      B c = xget(x,i);
      bool is = isC32(c);
      dec(c);
      if (!is) goto reg;
    }
    printf("\"");
    for (usz i = 0; i < ia; i++) printUTF8((u32)xget(x,i).u); // c32, no need to decrement
    printf("\"");
    return;
  }
  reg:;
  printf("⟨");
  for (usz i = 0; i < ia; i++) {
    if (i!=0) printf(", ");
    B c = xget(x,i);
    print(c);
    dec(c);
  }
  printf("⟩");
}


#ifdef DEBUG
  B validate(B x) {
    if (!isVal(x)) return x;
    if (v(x)->refc<=0 || (v(x)->refc>>28) == 'a') {
      printf("bad refcount for type %d: %d; val=%p\nattempting to print: ", v(x)->type, v(x)->refc, (void*)x.u); fflush(stdout);
      print(x); puts(""); fflush(stdout);
      err("");
    }
    if (isArr(x)) {
      ur r = rnk(x);
      if (r<=1) assert(a(x)->sh == &a(x)->ia);
      else validate(tag(shObj(x),OBJ_TAG));
    }
    return x;
  }
  B recvalidate(B x) {
    validate(x);
    if (isArr(x)) {
      BS2B xget = TI(x).get;
      usz ia = a(x)->ia;
      for (usz i = 0; i < ia; i++) {
        B c = xget(x,i);
        assert(c.u!=x.u);
        recvalidate(c);
        dec(c);
      }
    }
    return x;
  }
#endif
