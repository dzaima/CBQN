#include "core.h"
#include "vm.h"
#include "ns.h"
#include "utils/utf.h"
#include "utils/talloc.h"
#include "utils/file.h"
#include "utils/time.h"
#include "utils/interrupt.h"

static B replPath;
static Scope* gsc;
static bool init = false;

static void repl_init() {
  if (init) return;
  cbqn_init();
  replPath = m_ascii0("."); gc_add(replPath);
  Body* body = m_nnsDesc();
  B ns = m_nns(body);
  gsc = ptr_inc(c(NS, ns)->sc); gc_add(tag(gsc,OBJ_TAG));
  ptr_dec(v(ns));
  init = true;
}

static B gsc_exec_inline(B src, B path, B args) {
  Block* block = bqn_compSc(src, path, args, gsc, true);
  ptr_dec(gsc->body); // redirect new errors to the newly executed code; initial scope had 0 vars, so this is safe
  gsc->body = ptr_inc(block->bodies[0]);
  B r = execBlockInline(block, gsc);
  ptr_dec(block);
  return r;
}

static bool isCmd(char* s, char** e, const char* cmd) {
  while (*cmd) {
    if (*cmd==' ' && *s==0) { *e = s; return true; }
    if (*s!=*cmd || *s==0) return false;
    s++; cmd++;
  }
  *e = s;
  return true;
}
bool profiler_alloc(void);
bool profiler_start(i64 hz);
bool profiler_stop(void);
void profiler_free(void);
void profiler_displayResults(void);
void clearImportCache(void);

static B escape_parser;
B simple_unescape(B x) {
  if (RARE(escape_parser.u==0)) {
    escape_parser = bqn_exec(utf8Decode0("{m←\"Expected surrounding quotes\" ⋄ m!2≤≠𝕩 ⋄ m!\"\"\"\"\"\"≡0‿¯1⊏𝕩 ⋄ s←¬e←<`'\\'=𝕩 ⋄ i‿o←\"\\\"\"nr\"⋈\"\\\"\"\"∾@+10‿13 ⋄ 1↓¯1↓{n←i⊐𝕩 ⋄ \"Unknown escape\"!∧´n≠≠i ⋄ n⊏o}⌾((s/»e)⊸/) s/𝕩}"), bi_N, bi_N);
    gc_add(escape_parser);
  }
  return c1(escape_parser, x);
}

i64 readInt(char** p) {
  char* c = *p;
  i64 am = 0;
  while (*c>='0' & *c<='9') {
    am = am*10 + (*c - '0');
    c++;
  }
  *p = c;
  return am;
}
void cbqn_runLine0(char* ln, i64 read) {
  if (ln[0]==10) return;
  
  B code;
  int output; // 0-no; 1-formatter; 2-internal
  i32 time = 0;
  i64 profile = -1;
  if (ln[0] == ')') {
    char* cmdS = ln+1;
    char* cmdE;
    if (isCmd(cmdS, &cmdE, "ex ")) {
      B path = utf8Decode0(cmdE);
      code = path_chars(path);
      output = 0;
    } else if (isCmd(cmdS, &cmdE, "r ")) {
      code = utf8Decode0(cmdE);
      output = 0;
    } else if (isCmd(cmdS, &cmdE, "escaped ")) {
      B u = simple_unescape(utf8Decode0(cmdE));
      u64 len = utf8lenB(u);
      TALLOC(char, ascii, len+1);
      toUTF8(u, ascii);
      dec(u);
      ascii[len] = '\0';
      cbqn_runLine0(ascii, len+1);
      TFREE(ascii);
      return;
    } else if (isCmd(cmdS, &cmdE, "t ") || isCmd(cmdS, &cmdE, "time ")) {
      code = utf8Decode0(cmdE);
      time = -1;
      output = 0;
    } else if (isCmd(cmdS, &cmdE, "profile ") || isCmd(cmdS, &cmdE, "profile@")) {
      char* cpos = cmdE;
      profile = '@'==*(cpos-1)? readInt(&cpos) : 5000;
      if (profile==0) { printf("Cannot profile with 0hz sampling frequency\n"); return; }
      if (profile>999999) { printf("Cannot profile with >999999hz frequency\n"); return; }
      code = utf8Decode0(cpos);
      output = 0;
    } else if (isCmd(cmdS, &cmdE, "t:") || isCmd(cmdS, &cmdE, "time:")) {
      char* repE = cmdE;
      i64 am = readInt(&repE);
      if (repE==cmdE) { printf("time command not given repetition count\n"); return; }
      if (am==0) { printf("repetition count was zero\n"); return; }
      code = utf8Decode0(repE);
      time = am;
      output = 0;
    } else if (isCmd(cmdS, &cmdE, "mem ")) {
      bool sizes = 0;
      bool types = 0;
      bool freed = 0;
      char c;
      while ((c=*(cmdE++)) != 0) {
        if (c=='t') types=1;
        if (c=='s') sizes=1;
        if (c=='f') freed=1;
      }
      heap_printInfo(sizes, types, freed);
      return;
    } else if (isCmd(cmdS, &cmdE, "erase ")) {
      char* name = cmdE;
      i64 len = strlen(name);
      ScopeExt* e = gsc->ext;
      if (e!=NULL) {
        i32 am = e->varAm;
        for (i32 i = 0; i < am; i++) {
          B c = e->vars[i+am];
          if (a(c)->ia != len) continue;
          SGetU(c)
          bool ok = true;
          for (i32 j = 0; j < len; j++) ok&= o2cu(GetU(c, j))==name[j];
          if (ok) {
            B val = e->vars[i];
            e->vars[i] = bi_noVar;
            dec(val);
            #if ENABLE_GC
              if (!gc_depth) gc_forceGC();
            #endif
            return;
          }
        }
      }
      printf("No such variable found\n");
      return;
    } else if (isCmd(cmdS, &cmdE, "clearImportCache ")) {
      clearImportCache();
      return;
    } else if (isCmd(cmdS, &cmdE, "vars")) {
      B r = listVars(gsc);
      if (q_N(r)) {
        printf("Couldn't list variables\n");
      } else {
        usz ia = a(r)->ia;
        B* rp = harr_ptr(r);
        for (usz i = 0; i < ia; i++) {
          if (i!=0) printf(", ");
          printRaw(rp[i]);
        }
        putchar('\n');
      }
      return;
    } else if (isCmd(cmdS, &cmdE, "gc ")) {
      #if ENABLE_GC
        if (0==*cmdE) {
          if (gc_depth!=0) {
            printf("GC is disabled, but forcibly GCing anyway\n");
            gc_enable();
            gc_forceGC();
            gc_disable();
          } else {
            gc_forceGC();
          }
        } else if (strcmp(cmdE,"on")==0) {
          if (gc_depth==0) printf("GC already on\n");
          else if (gc_depth>1) printf("GC cannot be enabled\n");
          else {
            gc_enable();
            printf("GC enabled\n");
          }
        } else if (strcmp(cmdE,"off")==0) {
          if (gc_depth>0) printf("GC already off\n");
          else {
            gc_disable();
            printf("GC disabled\n");
          }
        } else printf("Unknown GC command\n");
      #else
        printf("Macro ENABLE_GC was false at compile-time, cannot GC\n");
      #endif
      return;
    } else if (isCmd(cmdS, &cmdE, "internalPrint ")) {
      code = utf8Decode0(cmdE);
      output = 2;
    } else if (isCmd(cmdS, &cmdE, "e ") || isCmd(cmdS, &cmdE, "explain ")) {
      B expl = bqn_explain(utf8Decode0(cmdE), replPath);
      HArr* expla = toHArr(expl);
      usz ia=expla->ia;
      for(usz i=0; i<ia; i++) {
          printRaw(expla->a[i]);
          putchar('\n');
      }
      dec(expl);
      return;
    } else {
      printf("Unknown REPL command\n");
      return;
    }
  } else {
    code = utf8Decode0(ln);
    output = 1;
  }
  Block* block = bqn_compSc(code, inc(replPath), emptySVec(), gsc, true);
  
  ptr_dec(gsc->body);
  gsc->body = ptr_inc(block->bodies[0]);
  
  B res;
  if (time) {
    f64 t;
    if (time==-1) {
      u64 sns = nsTime();
      res = execBlockInline(block, gsc);
      u64 ens = nsTime();
      t = ens-sns;
    } else {
      u64 sns = nsTime();
      for (i64 i = 0; i < time; i++) dec(execBlockInline(block, gsc));
      u64 ens = nsTime();
      t = (ens-sns)*1.0 / time;
      res = m_c32(0);
    }
    if      (t<1e3) printf("%.5gns\n", t);
    else if (t<1e6) printf("%.4gus\n", t/1e3);
    else if (t<1e9) printf("%.4gms\n", t/1e6);
    else            printf("%.5gs\n", t/1e9);
  } else if (profile>0) {
    if (CATCH) { profiler_stop(); profiler_free(); rethrow(); }
    if (profiler_alloc() && profiler_start(profile)) {
      res = execBlockInline(block, gsc);
      profiler_stop();
      profiler_displayResults();
      profiler_free();
    }
    popCatch();
  } else res = execBlockInline(block, gsc);
  ptr_dec(block);
  
  if (output) {
    if (output!=2 && FORMATTER) {
      B resFmt = bqn_fmt(res);
      printRaw(resFmt); dec(resFmt);
      putchar('\n');
    } else {
      print(res); putchar('\n'); fflush(stdout);
      dec(res);
    }
  } else dec(res);
  
}

void cbqn_runLine(char* ln, i64 len) {
  if(CATCH) {
    cbqn_takeInterrupts(false);
    fprintf(stderr, "Error: "); printErrMsg(thrownMsg); fputc('\n', stderr);
    vm_pst(envCurr+1, envStart+envPrevHeight);
    freeThrown();
    #ifdef HEAP_VERIFY
      heapVerify();
    #endif
    gc_maybeGC();
    return;
  }
  cbqn_takeInterrupts(true);
  cbqn_runLine0(ln, len);
  #ifdef HEAP_VERIFY
    heapVerify();
  #endif
  gc_maybeGC();
  cbqn_takeInterrupts(false);
  popCatch();
}

#if WASM
void cbqn_evalSrc(char* src, i64 len) {
  B code = utf8Decode(src, len);
  B res = bqn_exec(code, bi_N, bi_N);
  
  B resFmt = bqn_fmt(res);
  printRaw(resFmt); dec(resFmt);
  putchar('\n');
}
#endif

#if EMCC
int main() {
  repl_init();
}
#elif !CBQN_SHARED
int main(int argc, char* argv[]) {
  repl_init();
  bool startREPL = argc==1;
  bool silentREPL = false;
  bool execStdin = false;
  if (!startREPL) {
    i32 i = 1;
    while (i!=argc) {
      char* carg = argv[i];
      if (carg[0]!='-') break;
      if (carg[1]==0) { execStdin=true; i++; break; }
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
        while ((c = *carg++)) {
          switch(c) { default: fprintf(stderr, "%s: Unknown option: -%c\n", argv[0], c); exit(1);
            #define REQARG(X) if(*carg) { fprintf(stderr, "%s: -%s must end the option\n", argv[0], #X); exit(1); } if (i==argc) { fprintf(stderr, "%s: -%s requires an argument\n", argv[0], #X); exit(1); }
            case 'f': repl_init(); REQARG(f); goto execFile;
            case 'e': { repl_init(); REQARG(e);
              dec(gsc_exec_inline(utf8Decode0(argv[i++]), m_ascii0("(-e)"), emptySVec()));
              break;
            }
            case 'L': { repl_init(); break; } // just initialize. mostly for perf testing
            case 'p': { repl_init(); REQARG(p);
              B r = bqn_fmt(gsc_exec_inline(utf8Decode0(argv[i++]), m_ascii0("(-p)"), emptySVec()));
              printRaw(r); dec(r);
              printf("\n");
              break;
            }
            case 'o': { repl_init(); REQARG(o);
              B r = gsc_exec_inline(utf8Decode0(argv[i++]), m_ascii0("(-o)"), emptySVec());
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
              B path = utf8Decode0(argv[i++]);
              B lines = path_lines(path);
              usz ia = a(lines)->ia;
              SGet(lines)
              for (u64 i = 0; i < ia; i++) {
                dec(gsc_exec_inline(Get(lines, i), inc(replPath), emptySVec()));
              }
              break;
            }
            #endif
            case 'r': { startREPL=true;                  break; }
            case 's': { startREPL=true; silentREPL=true; break; }
          }
        }
      }
    }
    execFile:
    if (i!=argc || execStdin) {
      repl_init();
      B src;
      if (!execStdin) src = utf8Decode0(argv[i++]);
      B args;
      if (i==argc) {
        args = emptySVec();
      } else {
        M_HARR(ap, argc-i)
        for (usz j = 0; j < argc-i; j++) HARR_ADD(ap, j, utf8Decode0(argv[i+j]));
        args = HARR_FV(ap);
      }
      
      B execRes;
      if (execStdin) {
        execRes = gsc_exec_inline(utf8DecodeA(stream_bytes(stdin)), m_ascii0("(-)"), args);
      } else {
        execRes = bqn_execFile(src, args);
      }
      dec(execRes);
      #ifdef HEAP_VERIFY
        heapVerify();
      #endif
      gc_forceGC();
    }
  }
  if (startREPL) {
    repl_init();
    while (true) {
      if (!silentREPL) {
        printf("   ");
        fflush(stdout);
      }
      char* ln = NULL;
      size_t gl = 0;
      i64 read = getline(&ln, &gl, stdin);
      if (read<=0 || ln[0]==0) { if(!silentREPL) putchar('\n'); break; }
      if (ln[read-1]==10) ln[--read] = 0;
      cbqn_runLine(ln, read);
      free(ln);
    }
  }
  #ifdef HEAP_VERIFY
    heapVerify();
  #endif
  bqn_exit(0);
  #undef INIT
}
#endif