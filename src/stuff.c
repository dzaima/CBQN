#include "h.h"
// a bunch of random things that don't really belong in any other file

void arr_print(B x) { // should accept refc=0 arguments for debugging purposes
  usz r = rnk(x);
  BS2B xgetU = TI(x).getU;
  usz ia = a(x)->ia;
  if (r!=1) {
    if (r==0) {
      printf("<");
      print(xgetU(x,0));
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
      B c = xgetU(x,i);
      if (!isC32(c) || (u32)c.u=='\n') goto reg;
    }
    printf("\"");
    for (usz i = 0; i < ia; i++) printUTF8((u32)xgetU(x,i).u); // c32, no need to decrement
    printf("\"");
    return;
  }
  reg:;
  printf("⟨");
  for (usz i = 0; i < ia; i++) {
    if (i!=0) printf(", ");
    print(xgetU(x,i));
  }
  printf("⟩");
}

void print(B x) {
  if (isF64(x)) {
    printf("%g", x.f);
  } else if (isC32(x)) {
    if ((u32)x.u>=32) { printf("'"); printUTF8((u32)x.u); printf("'"); }
    else if((u32)x.u>15) printf("\\x%x", (u32)x.u);
    else printf("\\x0%x", (u32)x.u);
  } else if (isI32(x)) {
    printf("%d", (i32)x.u);
  } else if (isVal(x)) {
    #ifdef DEBUG
    if (isVal(x) && (v(x)->type==t_freed || v(x)->type==t_empty)) {
      u8 t = v(x)->type;
      v(x)->type = v(x)->flags;
      printf(t==t_freed?"FREED:":"EMPTY:");
      TI(x).print(x);
      v(x)->type = t;
      return;
    }
    #endif
    TI(x).print(x);
  }
  else if (isVar(x)) printf("(var d=%d i=%d)", (u16)(x.u>>32), (i32)x.u);
  else if (x.u==bi_N.u) printf("·");
  else if (x.u==bi_optOut.u) printf("(value optimized out)");
  else if (x.u==bi_noVar.u) printf("(unset variable placeholder)");
  else if (x.u==bi_badHdr.u) printf("(bad header note)");
  else if (x.u==bi_noFill.u) printf("(no fill placeholder)");
  else printf("(todo tag %lx)", x.u>>48);
}

void printRaw(B x) {
  if (isAtm(x)) {
    if (isF64(x)) printf("%g", x.f);
    else if (isC32(x)) printUTF8((u32)x.u);
    else err("bad printRaw argument: atom arguments should be either numerical or characters");
  } else {
    usz ia = a(x)->ia;
    BS2B xgetU = TI(x).getU;
    for (usz i = 0; i < ia; i++) {
      B c = xgetU(x,i);
      if (c.u==0 || noFill(c)) { printf(" "); continue; }
      if (!isC32(c)) err("bad printRaw argument: expected all character items");
      printUTF8((u32)c.u);
    }
  }
}

B eq_c2(B t, B w, B x);
bool equal(B w, B x) { // doesn't consume
  bool wa = isArr(w);
  bool xa = isArr(x);
  if (wa!=xa) return false;
  if (!wa) return o2iu(eq_c2(bi_N, inc(w), inc(x)))?1:0;
  if (!eqShape(w,x)) return false;
  usz ia = a(x)->ia;
  BS2B xgetU = TI(x).getU;
  BS2B wgetU = TI(w).getU;
  for (usz i = 0; i < ia; i++) if(!equal(wgetU(w,i),xgetU(x,i))) return false;
  return true;
}

bool eqShPrefix(usz* w, usz* x, ur len) {
  return memcmp(w, x, len*sizeof(usz))==0;
}
bool eqShape(B w, B x) { assert(isArr(w)); assert(isArr(x));
  ur wr = rnk(w); usz* wsh = a(w)->sh;
  ur xr = rnk(x); usz* xsh = a(x)->sh;
  if (wr!=xr) return false;
  if (wsh==xsh) return true;
  return eqShPrefix(wsh, xsh, wr);
}

usz arr_csz(B x) {
  ur xr = rnk(x);
  if (xr<=1) return 1;
  usz* sh = a(x)->sh;
  usz r = 1;
  for (i32 i = 1; i < xr; i++) r*= sh[i];
  return r;
}



#ifdef DEBUG
  Value* VALIDATEP(Value* x) {
    if (x->refc<=0 || (x->refc>>28) == 'a' || x->type==t_empty) {
      printf("bad refcount for type %d: %d\nattempting to print: ", x->type, x->refc); fflush(stdout);
      print(tag(x,OBJ_TAG)); puts(""); fflush(stdout);
      err("");
    }
    if (ti[x->type].isArr) {
      Arr* a = (Arr*)x;
      if (rnk(tag(x,ARR_TAG))<=1) assert(a->sh == &a->ia);
      else VALIDATE(tag(shObj(tag(x,ARR_TAG)),OBJ_TAG));
    }
    return x;
  }
  B VALIDATE(B x) {
    if (!isVal(x)) return x;
    VALIDATEP(v(x));
    if(isArr(x)!=TI(x).isArr && v(x)->type!=t_freed) {
      printf("wat %d %p\n", v(x)->type, (void*)x.u);
      print(x);
      err("\nk");
    }
    return x;
  }
#endif

#ifdef USE_VALGRIND
  #include <valgrind/valgrind.h>
  #include <valgrind/memcheck.h>
  void pst(char* msg) {
    VALGRIND_PRINTF_BACKTRACE("%s", msg);
  }
#endif

void empty_free(B x) { err("FREEING EMPTY\n"); }
void builtin_free(B x) { err("FREEING BUILTIN\n"); }
void def_visit(B x) { printf("(no visit for %d=%s)\n", v(x)->type, format_type(v(x)->type)); }
void freed_visit(B x) {
  #ifndef CATCH_ERRORS
  err("visiting t_freed\n");
  #endif
}
void def_print(B x) { printf("(%d=%s)", v(x)->type, format_type(v(x)->type)); }
B    def_identity(B f) { return bi_N; }
B    def_get (B x, usz n) { return inc(x); }
B    def_getU(B x, usz n) { return x; }
B    def_m1_d(B m, B f     ) { return err("cannot derive this"); }
B    def_m2_d(B m, B f, B g) { return err("cannot derive this"); }
B    def_slice(B x, usz s) { return err("cannot slice non-array!"); }
bool def_canStore(B x) { return false; }

static inline void hdr_init() {
  for (i32 i = 0; i < t_COUNT; i++) {
    ti[i].free  = do_nothing;
    ti[i].visit = def_visit;
    ti[i].get   = def_get;
    ti[i].getU  = def_getU;
    ti[i].print = def_print;
    ti[i].m1_d  = def_m1_d;
    ti[i].m2_d  = def_m2_d;
    ti[i].isArr = false;
    ti[i].arrD1 = false;
    ti[i].identity = def_identity;
    ti[i].decompose = def_decompose;
    ti[i].slice     = def_slice;
    ti[i].canStore  = def_canStore;
  }
  ti[t_empty].free = empty_free;
  ti[t_freed].free = do_nothing;
  ti[t_freed].visit = freed_visit;
  ti[t_shape].visit = do_nothing;
  ti[t_funBI].visit = ti[t_md1BI].visit = ti[t_md2BI].visit = do_nothing;
  ti[t_funBI].free  = ti[t_md1BI].free  = ti[t_md2BI].free  = builtin_free;
  bi_N = tag(0, TAG_TAG);
  bi_noVar   = tag(1, TAG_TAG);
  bi_badHdr  = tag(2, TAG_TAG);
  bi_optOut  = tag(3, TAG_TAG);
  bi_noFill  = tag(5, TAG_TAG);
  assert((MD1_TAG>>1) == (MD2_TAG>>1)); // just to be sure it isn't changed incorrectly, `isMd` depends on this
}


#ifdef ALLOC_STAT
  u64* ctr_a = 0;
  u64* ctr_f = 0;
  u64 actrc = 21000;
  u64 talloc = 0;
  #ifdef ALLOC_SIZES
    u32** actrs;
  #endif
#endif

static inline void onAlloc(usz sz, u8 type) {
  #ifdef ALLOC_STAT
    if (!ctr_a) {
      #ifdef ALLOC_SIZES
        actrs = malloc(sizeof(u32*)*actrc);
        for (i32 i = 0; i < actrc; i++) actrs[i] = calloc(t_COUNT, sizeof(u32));
      #endif
      ctr_a = calloc(t_COUNT, sizeof(u64));
      ctr_f = calloc(t_COUNT, sizeof(u64));
    }
    assert(type<t_COUNT);
    #ifdef ALLOC_SIZES
      actrs[(sz+3)/4>=actrc? actrc-1 : (sz+3)/4][type]++;
    #endif
    ctr_a[type]++;
    talloc+= sz;
  #endif
}
static inline void onFree(Value* x) {
  #ifdef ALLOC_STAT
    ctr_f[x->type]++;
  #endif
  #ifdef DEBUG
    if (x->type==t_empty) err("double-free");
    // u32 undef;
    // x->refc = undef;
    x->refc = -1431655000;
  #endif
  // x->refc = 0x61616161;
}
