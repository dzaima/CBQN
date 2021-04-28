// #define ATOM_I32
#ifdef DEBUG
  // #define DEBUG_VM
#endif

#define CATCH_ERRORS // whether to allow catching errors; currently means refcounts won't be accurate and can't be tested for
// #define HEAP_VERIFY  // enable usage of heapVerify()
// #define ALLOC_STAT   // store basic allocation statistics
// #define ALLOC_SIZES  // store per-type allocation size statistics
// #define USE_VALGRIND // whether to mark freed memory for valgrind
// #define DONT_FREE    // don't actually ever free objects, such that they can be printed after being freed for debugging
// #define OBJ_COUNTER  // store a unique allocation number with each object for easier analysis
#define FAKE_RUNTIME false // whether to disable the self-hosted runtime
// #define ALL_RUNTIME  // don't use custom native runtime parts

// #define LOG_GC       // log GC stats
// #define FORMATTER    // use self-hosted formatter for output
// #define TIME         // output runtime of every expression
// #define RT_PERF      // time runtime primitives

#define rtLen 63
#include "h.h"
#include "stuff.c"
#include "heap.c"
#include "mm_buddy.c"
#include "harr.c"
#include "fillarr.c"
#include "i32arr.c"
#include "c32arr.c"
#include "f64arr.c"
#include "utf.c"
#include "derv.c"
#include "sfns.c"
#include "sysfn.c"
#include "arith.c"
#include "md1.c"
#include "md2.c"
#include "vm.c"
#include "rtPerf.c"

void pr(char* a, B b) {
  printf("%s", a);
  print(b);
  puts("");
  dec(b);
  fflush(stdout);
}

Block* ca3(B x) {
  B* xp = harr_ptr(x);
  Block* r = compile(inc(xp[0]),inc(xp[1]),inc(xp[2]));
  dec(x);
  return r;
}

ssize_t getline(char** line, size_t* n, FILE* stream);


void printAllocStats() {
  #ifdef ALLOC_STAT
    printf("total ever allocated: %lu\n", talloc);
    printf("allocated heap size:  %ld\n", mm_heapAllocated());
    printf("used heap size:       %ld\n", mm_heapUsed());
    printf("ctrA←"); for (i64 i = 0; i < t_COUNT; i++) { if(i)printf("‿"); printf("%lu", ctr_a[i]); } printf("\n");
    printf("ctrF←"); for (i64 i = 0; i < t_COUNT; i++) { if(i)printf("‿"); printf("%lu", ctr_f[i]); } printf("\n");
    u64 leakedCount = 0;
    for (i64 i = 0; i < t_COUNT; i++) leakedCount+= ctr_a[i]-ctr_f[i];
    printf("leaked object count: %ld\n", leakedCount);
    #ifdef ALLOC_SIZES
      for(i64 i = 0; i < actrc; i++) {
        u32* c = actrs[i];
        bool any = false;
        for (i64 j = 0; j < t_COUNT; j++) if (c[j]) any=true;
        if (any) {
          printf("%ld", i*4);
          for (i64 k = 0; k < t_COUNT; k++) printf("‿%u", c[k]);
          printf("\n");
        }
      }
    #endif
  #endif
}

int main() {
  hdr_init();
  harr_init();
  fillarr_init();
  i32arr_init();
  c32arr_init();
  f64arr_init();
  arith_init();
  sfns_init();
  md1_init();
  md2_init();
  sysfn_init();
  derv_init();
  comp_init();
  rtPerf_init();
  
  // fake runtime
  B fruntime[] = {
    /* +-×÷⋆√⌊⌈|¬  */ bi_add  , bi_sub   , bi_mul  , bi_div, bi_pow   , bi_N     , bi_floor, bi_ceil, bi_stile, bi_not,
    /* ∧∨<>≠=≤≥≡≢  */ bi_and  , bi_or    , bi_lt   , bi_gt , bi_ne    , bi_eq    , bi_le   , bi_ge  , bi_feq  , bi_fne,
    /* ⊣⊢⥊∾≍↑↓↕«» */ bi_ltack, bi_rtack , bi_shape, bi_N  , bi_N     , bi_N     , bi_N    , bi_ud  , bi_N    , bi_N,
    /* ⌽⍉/⍋⍒⊏⊑⊐⊒∊  */ bi_N    , bi_N     , bi_slash, bi_N  , bi_N     , bi_select, bi_pick , bi_N   , bi_N    , bi_N,
    /* ⍷⊔!˙˜˘¨⌜⁼´  */ bi_N    , bi_N     , bi_asrt , bi_N  , bi_N     , bi_N     , bi_each , bi_tbl , bi_N    , bi_fold,
    /* ˝`∘○⊸⟜⌾⊘◶⎉  */ bi_N    , bi_scan  , bi_N    , bi_N  , bi_before, bi_N     , bi_N    , bi_val , bi_N    , bi_N,
    /* ⚇⍟⎊         */ bi_N    , bi_repeat, bi_catch
  };
  bool rtComplete[] = {
    /* +-×÷⋆√⌊⌈|¬  */ 1,1,1,1,1,0,1,1,1,1,
    /* ∧∨<>≠=≤≥≡≢  */ 1,1,1,1,1,1,1,1,1,1,
    /* ⊣⊢⥊∾≍↑↓↕«» */ 1,1,0,0,0,0,0,0,0,0,
    /* ⌽⍉/⍋⍒⊏⊑⊐⊒∊  */ 0,0,1,0,0,1,0,0,0,0,
    /* ⍷⊔!˙˜˘¨⌜⁼´  */ 0,0,1,0,0,0,1,1,0,1,
    /* ˝`∘○⊸⟜⌾⊘◶⎉  */ 0,1,0,0,1,0,0,1,0,0,
    /* ⚇⍟⎊         */ 0,1,1
  };
  assert(sizeof(fruntime)/sizeof(B) == rtLen);
  for (i32 i = 0; i < rtLen; i++) inc(fruntime[i]);
  B frtObj = m_caB(rtLen, fruntime);
  
  B provide[] = {bi_type,bi_fill,bi_log,bi_grLen,bi_grOrd,bi_asrt,bi_add,bi_sub,bi_mul,bi_div,bi_pow,bi_floor,bi_eq,bi_le,bi_fne,bi_shape,bi_pick,bi_ud,bi_tbl,bi_scan,bi_fillBy,bi_val,bi_catch};
  
  Block* runtime_b = compile(
    #include "runtime"
  );
  B rtRes = m_funBlock(runtime_b, 0); ptr_dec(runtime_b);
  B rtObjRaw = TI(rtRes).get(rtRes,0);
  B rtFinish = TI(rtRes).get(rtRes,1);
  dec(rtRes);
  
  runtimeLen = c(Arr,rtObjRaw)->ia;
  HArr_p runtimeH = m_harrUc(rtObjRaw);
  BS2B rtObjGet = TI(rtObjRaw).get;
  
  rt_sortAsc = rtObjGet(rtObjRaw, 10); gc_add(rt_sortAsc);
  rt_sortDsc = rtObjGet(rtObjRaw, 11); gc_add(rt_sortDsc);
  rt_merge   = rtObjGet(rtObjRaw, 13); gc_add(rt_merge);
  rt_undo    = rtObjGet(rtObjRaw, 48); gc_add(rt_undo);
  rt_select  = rtObjGet(rtObjRaw, 35); gc_add(rt_select);
  rt_slash   = rtObjGet(rtObjRaw, 32); gc_add(rt_slash);
  
  for (usz i = 0; i < runtimeLen; i++) {
    #ifdef ALL_RUNTIME
      B r = rtObjGet(rtObjRaw, i);
    #else
      B r = rtComplete[i]? inc(fruntime[i]) : rtObjGet(rtObjRaw, i);
    #endif
    if (isNothing(r)) { printf("· in runtime!\n"); exit(1); }
    r = rtPerf_wrap(r);
    runtimeH.a[i] = r;
    if (isVal(r)) v(r)->flags|= i+1;
  }
  dec(rtObjRaw);
  B* runtime = runtimeH.a;
  B rtObj = runtimeH.b;
  dec(c1(rtFinish, m_v2(inc(bi_decp), inc(bi_primInd)))); dec(rtFinish);
  
  
  B compArg = m_v2(FAKE_RUNTIME? frtObj : rtObj, inc(bi_sys)); gc_add(FAKE_RUNTIME? rtObj : frtObj);
  gc_add(compArg);
  
  
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
  gc_add(comp);
  
  
  #ifdef FORMATTER
  Block* fmt_b = compile(
    #include "formatter"
  );
  B fmtM = m_funBlock(fmt_b, 0); ptr_dec(fmt_b);
  B fmt = TI(fmtM).m1_d(fmtM, m_caB(4, (B[]){inc(bi_type), inc(bi_decp), inc(bi_fmtF), inc(bi_fmtN)}));
  gc_add(fmt);
  #endif
  
  
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
  //   B cbc = c2(comp, inc(rtObj), fromUTF8(c_src, c_len));
  //   Block* cbc_b = ca3(cbc);
  //   comp = m_funBlock(cbc_b, 0);
  //   free(c_src);
  // }
  while (CATCH) {
    printf("caught: ");
    print(catchMessage);
    puts("");
    dec(catchMessage);
  }
  while (true) { // exit by evaluating an empty expression
    char* ln = NULL;
    size_t gl = 0;
    getline(&ln, &gl, stdin);
    if (ln[0]==0 || ln[0]==10) break;
    B cbc = c2(comp, inc(compArg), fromUTF8(ln, strlen(ln)));
    free(ln);
    Block* cbc_b = ca3(cbc);
    
    #ifdef TIME
    u64 sns = nsTime();
    B res = m_funBlock(cbc_b, 0);
    u64 ens = nsTime();
    printf("%fms\n", (ens-sns)/1e6);
    #else
    B res = m_funBlock(cbc_b, 0);
    #endif
    ptr_dec(cbc_b);
    
    #ifdef FORMATTER
    B resFmt = c1(fmt, res);
    printRaw(resFmt); dec(resFmt);
    printf("\n");
    #else
    pr("", res);
    #endif
    
    #ifdef HEAP_VERIFY
    heapVerify();
    #endif
    gc_forceGC();
    #ifdef DEBUG
    #endif
  }
  rtPerf_print();
  popCatch();
  CTR_FOR(CTR_PRINT)
  // printf("done\n");fflush(stdout); while(1);
  printAllocStats();
}
