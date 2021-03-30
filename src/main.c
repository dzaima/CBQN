// #define ATOM_I32
#ifdef DEBUG
  // #define DEBUG_VM
#endif
// #define ALLOC_STAT

#include "h.h"
#include "mm.c"
#include "harr.c"
#include "i32arr.c"
#include "arith.c"
#include "sfns.c"
#include "md1.c"
#include "md2.c"
#include "sysfn.c"
#include "derv.c"
#include "vm.c"
#include "utf.c"

void pr(char* a, B b) {
  printf("%s", a);
  print(b);
  puts("");
  dec(b);
  fflush(stdout);
}

Block* ca3(B b) {
  B* ps = harr_ptr(b);
  Block* r = compile(inci(ps[0]),inci(ps[1]),inci(ps[2]));
  dec(b);
  return r;
}

B m_str8(char* s) {
  u64 sz = strlen(s);
  HArr_p r = m_harrv(sz);
  for (u64 i = 0; i < sz; i++) r.a[i] = m_c32(s[i]);
  return r.b;
}
B m_str32(u32* s) {
  u64 sz = 0;
  while(s[sz])sz++;
  HArr_p r = m_harrv(sz);
  for (u64 i = 0; i < sz; i++) r.a[i] = m_c32(s[i]);
  return r.b;
}
B m_cai32(usz ia, i32* a) {
  B r = m_i32arrv(ia);
  i32* rp = i32arr_ptr(r);
  for (usz i = 0; i < ia; i++) rp[i] = a[i];
  return r;
}

__ssize_t getline (char **__restrict __lineptr, size_t *restrict n, FILE *restrict stream);

int main() {
  hdr_init();
  harr_init();
  i32arr_init();
  arith_init();
  sfns_init();
  md1_init();
  md2_init();
  sysfn_init();
  derv_init();
  comp_init();
  
  
  // fake runtime
  // B bi_N = bi_nothing;
  // B runtime[] = {
  //   /* +-×÷⋆√⌊⌈|¬  */ bi_add, bi_sub , bi_mul  , bi_div, bi_pow, bi_N , bi_floor, bi_N  , bi_N, bi_N,
  //   /* ∧∨<>≠=≤≥≡≢  */ bi_N  , bi_N   , bi_N    , bi_N  , bi_N  , bi_eq, bi_le   , bi_N  , bi_N, bi_fne,
  //   /* ⊣⊢⥊∾≍↑↓↕«» */ bi_lt , bi_rt  , bi_shape, bi_N  , bi_N  , bi_N , bi_N    , bi_ud , bi_N, bi_N,
  //   /* ⌽⍉/⍋⍒⊏⊑⊐⊒∊  */ bi_N  , bi_N   , bi_N    , bi_N  , bi_N  , bi_N , bi_pick , bi_N  , bi_N, bi_N,
  //   /* ⍷⊔!˙˜˘¨⌜⁼´  */ bi_N  , bi_N   , bi_asrt , bi_N  , bi_N  , bi_N , bi_N    , bi_tbl, bi_N, bi_N,
  //   /* ˝`∘○⊸⟜⌾⊘◶⎉  */ bi_N  , bi_scan, bi_N    , bi_N  , bi_N  , bi_N , bi_N    , bi_val, bi_N, bi_N,
  //   /* ⚇⍟          */ bi_N  , bi_N
  // };
  // Block* c = ca3(
  //   #include "interp"
  // );
  // B interp = m_funBlock(c, 0); ptr_dec(c);
  // pr("interpreted: ", interp);
  
  Block* runtime_b = compile(
    #include "runtime"
  );
  B rtRes = m_funBlock(runtime_b, 0); ptr_dec(runtime_b);
  B rtObj    = TI(rtRes).get(rtRes,0);
  B rtFinish = TI(rtRes).get(rtRes,1);
  B* runtime = toHArr(rtObj)->a;
  runtimeLen = c(Arr,rtObj)->ia;
  for (usz i = 0; i < runtimeLen; i++) {
    if (isVal(runtime[i])) v(runtime[i])->flags|= i+1;
  }
  c1(rtFinish, m_v2(inci(bi_decp), inci(bi_primInd)));
  
  // uncomment to use src/interp; needed for test.bqn
  // Block* c = ca3(
  //   #include "interp"
  // );
  // B interp = m_funBlock(c, 0); ptr_dec(c);
  // pr("result: ", interp);
  // exit(0);
  
  Block* comp_b = compile(
    #include "compiler"
  );
  B comp = m_funBlock(comp_b, 0); ptr_dec(comp_b);
  
  
  // uncomment to self-compile and use that for the REPL; expects a copy of mlochbaum/BQN/src/c.bqn to be at the execution directory
  // char* c_src = 0;
  // u64 c_len;
  // FILE* f = fopen("c.bqn", "rb");
  // if (f) {
  //   fseek(f, 0, SEEK_END);
  //   c_len = ftell(f);
  //   fseek(f, 0, SEEK_SET);
  //   c_src = malloc(c_len);
  //   if (c_src) fread(c_src, 1, c_len, f);
  //   fclose(f);
  // } else {
  //   printf("couldn't read c.bqn\n");
  //   exit(1);
  // }
  // if (c_src) {
  //   B cbc = c2(comp, inci(rtObj), fromUTF8(c_src, c_len));
  //   Block* cbc_b = ca3(cbc);
  //   comp = m_funBlock(cbc_b, 0);
  //   free(c_src);
  // }
  
  
  while (true) { // exit by evaluating an empty expression
    char* ln = NULL;
    size_t gl = 0;
    getline(&ln, &gl, stdin);
    if (ln[0]==10) break;
    B obj = fromUTF8(ln, strlen(ln));
    B cbc = c2(comp, inci(rtObj), obj);
    free(ln);
    Block* cbc_b = ca3(cbc);
    pr("", m_funBlock(cbc_b, 0));
  }
  
  dec(rtRes);
  dec(comp);
  
  #ifdef ALLOC_STAT
    printf("total allocated: %lu\n", talloc);
    printf("ctrA←"); for (i64 j = 0; j < Type_MAX; j++) { if(j)printf("‿"); printf("%lu", ctr_a[j]); } printf("\n");
    printf("ctrF←"); for (i64 j = 0; j < Type_MAX; j++) { if(j)printf("‿"); printf("%lu", ctr_f[j]); } printf("\n");
    for(i64 i = 0; i < actrc; i++) {
      u32* c = actrs[i];
      bool any = false;
      for (i64 j = 0; j < Type_MAX; j++) if (c[j]) any=true;
      if (any) {
        printf("%ld", i);
        for (i64 j = 0; j < Type_MAX; j++) printf("‿%u", c[j]);
        printf("\n");
      }
    }
  #endif
}
