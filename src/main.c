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
#include "file.c"
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

int main(int argc, char* argv[]) {
  cbqn_init();
  
  // expects a copy of mlochbaum/BQN/src/c.bqn to be at the execution directory (with •args replaced with the array in glyphs.bqn)
  #if defined(COMP_COMP) || defined(COMP_COMP_TIME)
    char* c_src = NULL;
    u64 c_len;
    FILE* f = fopen("c.bqn", "rb");
    if (f) {
      fseek(f, 0, SEEK_END); c_len = ftell(f);
      fseek(f, 0, SEEK_SET); c_src = malloc(c_len);
      if (c_src) fread(c_src, 1, c_len, f);
      fclose(f);
    } else {
      c_src = NULL;
    }
    if (c_src) {
      B srcB = fromUTF8(c_src, c_len);
      #ifdef COMP_COMP_TIME
        gc_add(srcB);
        for (i32 i = 0; i < 100; i++) { dec(bqn_exec(inc(srcB), bi_N)); gc_maybeGC(); }
        rtPerf_print();
        exit(0);
      #endif
      bqn_setComp(bqn_exec(srcB, bi_N));
    } else {
      printf("couldn't read c.bqn\n");
      exit(1);
    }
  #endif
  bool startREPL = argc==1;
  if (!startREPL) {
    i32 i = 1;
    while (i!=argc) {
      char* carg = argv[i];
      if (carg[0]!='-') break;
      i++;
      if (carg[1]=='-') {
        if (!strcmp(carg, "--help")) {
          printf(
          "Usage: %s [options] [file.bqn [arguments]]\n"
          "Options:\n"
          "-f file: execute the contents of the file with all further arguments as •args\n"
          "-e code: execute the argument as BQN\n"
          "-p code: execute the argument as BQN and print its result pretty-printed\n"
          "-o code: execute the argument as BQN and print its raw result\n"
          "-r     : start the REPL after all further arguments\n"
          , argv[0]);
          exit(0);
        } else {
          printf("%s: Unknown option: %s\n", argv[0], carg);
          exit(1);
        }
      } else {
        carg++;
        char c;
        while ((c=*carg++) != '\0') {
          switch(c) { default: printf("Unknown option: -%c\n", c);
            #define REQARG(X) if(*carg) { printf("%s: -%s must end the option\n", argv[0], #X); exit(1); } if (i==argc) { printf("%s: -%s requires an argument\n", argv[0], #X); exit(1); }
            case 'f': REQARG(f); goto execFile;
            case 'e': { REQARG(e);
              dec(bqn_exec(fromUTF8l(argv[i++]), m_str32(U"-e"), inc(bi_emptyHVec)));
              break;
            }
            case 'p': { REQARG(p);
              B r = bqn_exec(fromUTF8l(argv[i++]), m_str32(U"-e"), inc(bi_emptyHVec));
              print(r); dec(r);
              printf("\n");
              break;
            }
            case 'o': { REQARG(o);
              B r = bqn_exec(fromUTF8l(argv[i++]), m_str32(U"-e"), inc(bi_emptyHVec));
              printRaw(r); dec(r);
              printf("\n");
              break;
            }
            case 'r': {
              startREPL = true;
              break;
            }
          }
        }
      }
    }
    execFile:
    if (i!=argc) {
      B src = fromUTF8l(argv[i++]);
      B args;
      if (i==argc) {
        args = inc(bi_emptyHVec);
      } else {
        HArr_p ap = m_harrUv(argc-i); // eh whatever, erroring will exit anyways
        for (usz j = 0; j < argc-i; j++) {
          ap.a[j] = fromUTF8l(argv[i+j]);
        }
        args = ap.b;
      }
      dec(bqn_execFile(src, args));
    }
  }
  if (startREPL) {
    while (CATCH) {
      printf("Error: "); print(catchMessage); putchar('\n');
      vm_pst(envCurr, envStart+envPrevHeight);
      dec(catchMessage);
    }
    B replPath = m_str32(U"REPL"); gc_add(replPath);
    while (true) { // exit by evaluating an empty expression
      char* ln = NULL;
      size_t gl = 0;
      getline(&ln, &gl, stdin);
      if (ln[0]==0 || ln[0]==10) break;
      Block* block = bqn_comp(fromUTF8(ln, strlen(ln)), inc(replPath), inc(bi_emptyHVec));
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
    popCatch();
  }
  rtPerf_print();
  CTR_FOR(CTR_PRINT)
  // printf("done\n");fflush(stdout); while(1);
  printAllocStats();
}
