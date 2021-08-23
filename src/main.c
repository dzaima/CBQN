#include "core.h"
#include "vm.h"
#include "utils/utf.h"
#include "utils/file.h"

B replPath; // also used by sysfn.c
static Scope* gsc;
static bool init = false;

static void repl_init() {
  if (init) return;
  cbqn_init();
  replPath = m_str32(U"."); gc_add(replPath);
  Block* initBlock = bqn_comp(m_str32(U"\"(REPL initializer)\""), inc(replPath), m_f64(0));
  gsc = m_scope(initBlock->bodies[0], NULL, 0, 0, NULL); gc_add(tag(gsc,OBJ_TAG));
  ptr_dec(initBlock);
  init = true;
}

static B gsc_exec_inline(B src, B path, B args) {
  Block* block = bqn_compSc(src, path, args, gsc, true);
  ptr_dec(gsc->body); ptr_inc(block->bodies[0]); // redirect new errors to the newly executed code; initial scope had 0 vars, so this is safe
  gsc->body = block->bodies[0];
  B r = execBlockInline(block, gsc);
  ptr_dec(block);
  return r;
}

int main(int argc, char* argv[]) {
  // expects a copy of mlochbaum/BQN/src/c.bqn to be at the execution directory (with •args replaced with the array in glyphs.bqn)
  #if defined(COMP_COMP) || defined(COMP_COMP_TIME)
    repl_init();
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
        for (i32 i = 0; i < 100; i++) { dec(bqn_exec(inc(srcB), bi_N, bi_N)); gc_maybeGC(); }
        bqn_exit(0);
      #endif
      bqn_setComp(bqn_exec(srcB, bi_N, bi_N));
    } else err("couldn't read c.bqn\n");
  #endif
  bool startREPL = argc==1;
  bool silentREPL = false;
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
          "-M num : set maximum heap size to num megabytes\n"
          "-r     : start the REPL after executing all arguments\n"
          "-s     : start a silent REPL\n"
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
          switch(c) { default: fprintf(stderr, "%s: Unknown option: -%c\n", argv[0], c); exit(1);
            #define REQARG(X) if(*carg) { fprintf(stderr, "%s: -%s must end the option\n", argv[0], #X); exit(1); } if (i==argc) { fprintf(stderr, "%s: -%s requires an argument\n", argv[0], #X); exit(1); }
            case 'f': repl_init(); REQARG(f); goto execFile;
            case 'e': { repl_init(); REQARG(e);
              dec(gsc_exec_inline(fromUTF8l(argv[i++]), m_str32(U"(-e)"), emptySVec()));
              break;
            }
            case 'p': { repl_init(); REQARG(p);
              B r = gsc_exec_inline(fromUTF8l(argv[i++]), m_str32(U"(-p)"), emptySVec());
              print(r); dec(r);
              printf("\n");
              break;
            }
            case 'o': { repl_init(); REQARG(o);
              B r = gsc_exec_inline(fromUTF8l(argv[i++]), m_str32(U"(-o)"), emptySVec());
              printRaw(r); dec(r);
              printf("\n");
              break;
            }
            case 'M': { REQARG(M);
              char* str = argv[i++];
              u64 am = 0;
              while (*str) {
                char c = *str++;
                if (c<'0' | c>'9') { printf("%s: -M: Argument not a number\n", argv[0]); exit(1); }
                if (am>1ULL<<48) { printf("%s: -M: Too large\n", argv[0]); exit(1); }
                am = am*10 + c-48;
              }
              mm_heapMax = am*1024*1024;
              break;
            }
            #ifdef PERF_TEST
            case 'R': { repl_init(); REQARG(R);
              B path = fromUTF8l(argv[i++]);
              B lines = file_lines(path);
              usz ia = a(lines)->ia;
              BS2B lget = TI(lines,get);
              for (u64 i = 0; i < ia; i++) {
                dec(gsc_exec_inline(lget(lines, i), inc(replPath), emptySVec()));
              }
              break;
            }
            #endif
            case 'r': { startREPL = true;                    break; }
            case 's': { startREPL = true; silentREPL = true; break; }
          }
        }
      }
    }
    execFile:
    if (i!=argc) {
      repl_init();
      B src = fromUTF8l(argv[i++]);
      B args;
      if (i==argc) {
        args = emptySVec();
      } else {
        HArr_p ap = m_harrUv(argc-i); // eh whatever, erroring will exit anyways
        for (i64 j = 0; j < argc-i; j++) {
          ap.a[j] = fromUTF8l(argv[i+j]);
        }
        args = ap.b;
      }
      dec(bqn_execFile(src, args));
      #ifdef HEAP_VERIFY
        heapVerify();
      #endif
      gc_forceGC();
    }
  }
  if (startREPL) {
    repl_init();
    while (CATCH) {
      printf("Error: "); printErrMsg(catchMessage); putchar('\n');
      vm_pst(envCurr+1, envStart+envPrevHeight);
      dec(catchMessage);
      #ifdef HEAP_VERIFY
        heapVerify();
      #endif
      gc_maybeGC();
    }
    while (true) { // exit by evaluating an empty expression
      char* ln = NULL;
      size_t gl = 0;
      if (!silentREPL) printf("   ");
      i64 read = getline(&ln, &gl, stdin);
      if (read<=0 || ln[0]==0 || ln[0]==10) { if(!silentREPL) putchar('\n'); break; }
      B code;
      bool output;
      if (ln[0] == ')') {
        if (ln[1]=='e'&ln[2]=='x'&ln[3]==' ') {
          B path = fromUTF8(ln+4, strlen(ln+4)-1);
          code = file_chars(path);
          output = false;
        } else {
          printf("Unknown REPL command\n");
          free(ln);
          continue;
        }
      } else {
        code = fromUTF8(ln, strlen(ln));
        output = true;
      }
      Block* block = bqn_compSc(code, inc(replPath), emptySVec(), gsc, true);
      free(ln);
      
      ptr_dec(gsc->body); ptr_inc(block->bodies[0]);
      gsc->body = block->bodies[0];
      
      #ifdef TIME
      u64 sns = nsTime();
      B res = execBlockInline(block, gsc);
      u64 ens = nsTime();
      printf("%fms\n", (ens-sns)/1e6);
      #else
      B res = execBlockInline(block, gsc);
      #endif
      ptr_dec(block);
      
      if (output) {
        #if FORMATTER
          B resFmt = bqn_fmt(res);
          printRaw(resFmt); dec(resFmt);
          putchar('\n');
        #else
          print(res); putchar('\n'); fflush(stdout);
          dec(res);
        #endif
      } else dec(res);
      
      #ifdef HEAP_VERIFY
        heapVerify();
      #endif
      gc_maybeGC();
    }
    popCatch();
  }
  #ifdef HEAP_VERIFY
    heapVerify();
  #endif
  bqn_exit(0);
  #undef INIT
}
