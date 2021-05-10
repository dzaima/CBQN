// #define ATOM_I32
#ifdef DEBUG
  // #define DEBUG_VM
#endif

#define CATCH_ERRORS // whether to allow catching errors; currently means refcounts won't be accurate and can't be tested for
#define ENABLE_GC    // whether to ever garbage-collect
// #define HEAP_VERIFY  // enable usage of heapVerify()
// #define ALLOC_STAT   // store basic allocation statistics
// #define ALLOC_SIZES  // store per-type allocation size statistics
// #define USE_VALGRIND // whether to mark freed memory for valgrind
// #define DONT_FREE    // don't actually ever free objects, such that they can be printed after being freed for debugging
// #define OBJ_COUNTER  // store a unique allocation number with each object for easier analysis
// #define ALL_R0       // use all of r0.bqn for runtime_0
// #define ALL_R1       // use all of r1.bqn for runtime
#define VM_POS       false // whether to store detailed execution position information for stacktraces
#define CHECK_VALID  true  // whether to check for valid arguments in places where that would be detrimental to performance (e.g. left argument sortedness of ⍋/⍒)
#define EACH_FILLS   false // whether to try to squeeze out fills for ¨ and ⌜
#define SFNS_FILLS   true  // whether to insert fills for structural functions (∾, ≍, etc)
#define FAKE_RUNTIME false // whether to disable the self-hosted runtime

// #define LOG_GC       // log GC stats
// #define FORMATTER    // use self-hosted formatter for output
// #define TIME         // output runtime of every expression
// #define RT_PERF      // time runtime primitives
// #define NO_COMP      // don't load the compiler, instead execute src/interp; needed for ./precompiled.bqn

#ifdef CATCH_ERRORS
  #define PROPER_FILLS (EACH_FILLS&SFNS_FILLS)
#else
  #undef EACH_FILLS
  #define EACH_FILLS false
  #define PROPER_FILLS false
#endif

#define rtLen 63
#include "h.h"
#include "stuff.c"
#include "heap.c"
#include "mm_buddy.c"
#include "harr.c"
#include "i32arr.c"
#include "c32arr.c"
#include "f64arr.c"
#include "fillarr.c"
#include "mut.c"
#include "utf.c"
#include "derv.c"
#include "fns.c"
#include "sfns.c"
#include "sysfn.c"
#include "arith.c"
#include "grade.c"
#include "md1.c"
#include "md2.c"
#include "vm.c"
#include "ns.c"
#include "rtPerf.c"
#include "load.c"

int main() {
  cbqn_init();
  
  // uncomment to self-compile and use that for the REPL
  // expects a copy of mlochbaum/BQN/src/c.bqn to be at the execution directory (with •args replaced with the array in glyphs.bqn)
  
  // char* c_src = NULL;
  // u64 c_len;
  // FILE* f = fopen("c.bqn", "rb");
  // if (f) {
  //   fseek(f, 0, SEEK_END); c_len = ftell(f);
  //   fseek(f, 0, SEEK_SET); c_src = malloc(c_len);
  //   if (c_src) fread(c_src, 1, c_len, f);
  //   fclose(f);
  // } else {
  //   c_src = NULL;
  // }
  // if (c_src) {
  //   bqn_setComp(bqn_exec(fromUTF8(c_src, c_len)));
  //   // for (i32 i = 0; i < 100; i++) { dec(bqn_exec(fromUTF8(c_src, c_len))); gc_maybeGC(); } rtPerf_print(); exit(0);
  // } else {
  //   printf("couldn't read c.bqn\n");
  //   exit(1);
  // }
  
  
  while (CATCH) {
    printf("Error: "); print(catchMessage); putchar('\n');
    vm_pst(envCurr, envStart+envPrevHeight);
    dec(catchMessage);
  }
  while (true) { // exit by evaluating an empty expression
    char* ln = NULL;
    size_t gl = 0;
    getline(&ln, &gl, stdin);
    if (ln[0]==0 || ln[0]==10) break;
    Block* block = bqn_comp(fromUTF8(ln, strlen(ln)));
    free(ln);
    
    #ifdef TIME
    u64 sns = nsTime();
    B res = m_funBlock(block, 0);
    u64 ens = nsTime();
    printf("%fms\n", (ens-sns)/1e6);
    #else
    B res = m_funBlock(block, 0);
    #endif
    ptr_dec(block);
    
    #ifdef FORMATTER
    B resFmt = bqn_fmt(res);
    printRaw(resFmt); dec(resFmt);
    printf("\n");
    #else
    print(res); putchar('\n'); fflush(stdout);
    dec(res);
    #endif
    
    #ifdef HEAP_VERIFY
    heapVerify();
    #endif
    gc_maybeGC();
    #ifdef DEBUG
    #endif
  }
  rtPerf_print();
  popCatch();
  CTR_FOR(CTR_PRINT)
  // printf("done\n");fflush(stdout); while(1);
  printAllocStats();
}
