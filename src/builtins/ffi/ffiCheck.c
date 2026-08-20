#include "utils/mem.h"
#include "ffiCore.c"
#ifdef _WIN32
  #include "windows/winError.h"
  #include <windows.h>
#endif
#if !HAS_MMAP && !defined(_WIN32)
  STATIC_GLOBAL bool no_mmap_msg;
#endif
GLOBAL u16 ffi_extra_checks;

static void handler_core(char* raw, uintptr_t mem);

#if __has_include(<signal.h>) && !defined(_WIN32)
  #include <signal.h>
  STATIC_GLOBAL volatile bool handler_active;
  static void handler_action(int a, siginfo_t* info, void* b) {
    if (handler_active) __builtin_trap();
    handler_active = true;
    handler_core(info->si_signo == SIGBUS? "SIGBUS" : "SIGSEGV", PTR_TO_U64(info->si_addr));
    abort();
  }
  STATIC_GLOBAL bool handler_installed;
  static bool handler_set(bool enable) {
    if (handler_installed == enable) return true;
    handler_installed = enable;
    struct sigaction act = {};
    if (enable) {
      act.sa_flags = SA_SIGINFO;
      act.sa_sigaction = handler_action;
    } else {
      act.sa_handler = SIG_DFL;
    }
    return sigaction(SIGSEGV, &act, NULL) == 0
        && sigaction(SIGBUS,  &act, NULL) == 0;
  }
#elif defined(_WIN32)
  STATIC_GLOBAL volatile bool handler_active;
  static LONG WINAPI handler_action(EXCEPTION_POINTERS* info) {
    if (info->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION) {
      return EXCEPTION_CONTINUE_SEARCH;
    }
    if (handler_active) __builtin_trap();
    handler_active = true;
    handler_core("ACCESS_VIOLATION", (uintptr_t)info->ExceptionRecord->ExceptionInformation[1]);
    abort();
  }
  STATIC_GLOBAL PVOID hHandler;
  static bool handler_set(bool enable) {
    if (enable == (hHandler != NULL)) return true;
    if (enable) {
      hHandler = AddVectoredExceptionHandler(1, handler_action);
      return hHandler != NULL;
    } else {
      if (RemoveVectoredExceptionHandler(hHandler) == 0) return false;
      hHandler = NULL;
      return true;
    }
  }
#else
  static bool handler_set(bool enable) { return false; }
#endif

bool set_ffi_check(u8 mode, u8 alignment) {
  ffi_extra_checks = mode<<8 | alignment;
  MAYBE_UNUSED bool handler_ok = handler_set(ffi_extra_checks != 0);
  #if HAS_MMAP
    if (!handler_ok) fprintf(stderr, "Note: SIGSEGV/SIGBUS handler not available; FFI debugging won't give pretty messages\n");
  #elif defined(_WIN32)
    if (!handler_ok) fprintf(stderr, "Note: exception handler not available; FFI debugging won't give pretty messages\n");
  #else
    if (!no_mmap_msg) fprintf(stderr, "Note: mmap not available in this CBQN build; FFI debugging functionality is heavily reduced\n");
    no_mmap_msg = true;
  #endif
  return true;
}

typedef struct CheckedOuterBlock CheckedOuterBlock;
typedef struct CheckedInnerBlock CheckedInnerBlock;
struct CheckedOuterBlock { // permanent massive blocks
  void* start;
  ux capacity, offset;
  CheckedInnerBlock* last;
  CheckedOuterBlock* next;
};
struct CheckedInnerBlock { // block per pointer
  void* readable;
  ux szx;
  CheckedInnerBlock* next;
  CheckedOuterBlock* parent;
  u32 szxOver; // size+szxOver == szx
  bool writable;
  bool freed;
};
typedef struct TempBlock {
  struct CustomObj;
  CheckedInnerBlock* inner;
  void* tgt;
  void* src;
  B ptrObjRef;
  ux size;
} TempBlock;
STATIC_GLOBAL CheckedOuterBlock* checked_lastOuter;

static const char* checked_outerBuffer = "FFI buffer place";
static const char* checked_innerBuffer = "FFI pointer buffer";
void tempBlock_free(Value* v) { TempBlock* b = (TempBlock*)v;
  if (b->inner) {
    // log_printf("TempBlock free %p %ld\n", b->inner->readable, b->inner->szx);
    b->inner->freed = true;
    if (b->inner->szx == 0) return;
    #if HAS_MMAP
      mmap_anon(b->inner->readable, b->inner->szx, PROT_NONE, MAP_FIXED);
    #elif defined(_WIN32)
      if (!VirtualFree(b->inner->readable, b->inner->szx, MEM_DECOMMIT)) {
        log_printf("FFI: Failed to decommit checked memory: %s\n", winError());
      }
    #endif
  }
}
void tempBlock_visit(Value* v) {
  mm_visit(((TempBlock*)v)->ptrObjRef);
}
void checked_forRanges(AllocRangeConsumer f, void* data) {
  CheckedOuterBlock* o = checked_lastOuter;
  while (o != NULL) {
    CheckedInnerBlock* i = o->last;
    while (i != NULL) {
      f(i->readable, i->szx, checked_innerBuffer, i, data);
      i = i->next;
    }
    #if HAS_MMAP || defined(_WIN32)
    f(o->start, o->capacity, checked_outerBuffer, o, data);
    #endif
    o = o->next;
  }
}
static B checked_wrapper(void* src, CheckedInnerBlock* inner, void* tgt, B ptrObjRef, ux size) {
  TempBlock* bl = m_customObj(sizeof(TempBlock), tempBlock_visit, tempBlock_free);
  bl->src = src;
  bl->inner = inner;
  bl->tgt = tgt;
  bl->size = size;
  bl->ptrObjRef = ptrObjRef;
  return tag(bl, OBJ_TAG);
}
static B checked_wrap(B ptrObjRef) {
  return checked_wrapper(NULL, NULL, NULL, ptrObjRef, 0);
}
static B checked_allocBlock(void** mem, ux size, bool writable, B ptrObjRef) {
  // log_printf("TempBlock alloc " N64u " %s from %p\n", (u64)size, writable?"writable":"read-only", *mem);
  
  void* src = *mem;
  ux psz = getPageSize();
  ux szx = (size + psz-1) & ~(psz-1);
  ux pszx = IMAX(4096, psz);
  ux needed = szx + pszx*2;
  
  if (checked_lastOuter == NULL) {
    addMemoryRangeSource(checked_forRanges);
    goto newOuter;
  }
  if (checked_lastOuter->offset + needed > checked_lastOuter->capacity) { newOuter:;
    // log_printf("TempBlock new outer block\n");
    ux cap = IMAX(needed, 1ULL<<30);
    // cap = needed*3;
    #if HAS_MMAP
      void* outerMem = mmap_anon(NULL, cap, PROT_NONE, 0);
      if (outerMem == MAP_FAILED) thrM("FFI: Failed to run mmap for checked memory");
    #elif defined(_WIN32)
      void* outerMem = VirtualAlloc(NULL, cap, MEM_RESERVE, PAGE_NOACCESS);
      if (outerMem == NULL) thrF("FFI: Failed to reserve checked memory: %S", winError());
    #else
      void* outerMem = NULL;
    #endif
    CheckedOuterBlock* prev = checked_lastOuter; checked_lastOuter = malloc(sizeof(CheckedOuterBlock));
    *checked_lastOuter = (CheckedOuterBlock) { .start = outerMem, .capacity=cap, .offset=0, .last=NULL, .next=prev };
  }
  
  #if HAS_MMAP || defined(_WIN32)
    u8* block = (u8*) checked_lastOuter->start + checked_lastOuter->offset;
  #else
    u8* block = malloc(needed);
  #endif
  u8* readable = block + pszx;
  checked_lastOuter->offset += needed;
  
  u8 align0 = ffi_extra_checks & 0xff;
  bool alignStart = align0==2 || (align0==3 && (internalRand()&1));
  void* tgt = alignStart? readable : readable + szx - size;
  *mem = tgt;

  if (size) {
    #if HAS_MMAP
      mprotect(readable, size, PROT_READ|PROT_WRITE);
    #elif defined(_WIN32)
      if (VirtualAlloc(readable, size, MEM_COMMIT, PAGE_READWRITE) == NULL) {
        thrF("FFI: Failed to commit read-write checked memory: %S", winError());
      }
    #endif
    memcpy(tgt, src, size);
    #if HAS_MMAP
      if (!writable) mprotect(readable, size, PROT_READ);
    #elif defined(_WIN32)
      if (!writable) {
        DWORD oldProtect;
        if (!VirtualProtect(readable, size, PAGE_READONLY, &oldProtect)) {
          thrF("FFI: Failed to set checked memory read-only: %S", winError());
        }
      }
    #endif
  }
  // log_printf("  got %p (%p..%p)\n", block, readable, readable+szx);
  
  CheckedInnerBlock* inner = malloc(sizeof(CheckedInnerBlock));
  *inner = (CheckedInnerBlock){ .next = checked_lastOuter->last, .readable = readable, .szx = szx, .szxOver=szx-size, .writable=writable, .freed=false };
  checked_lastOuter->last = inner;
  return checked_wrapper(src, inner, tgt, ptrObjRef, size);
}

static void* checked_readBlock(B* obj, FFICompoundType* ct) {
  TempBlock* b = c(TempBlock, *obj);
  assert(b->type == t_customObj && b->freeO == tempBlock_free);
  *obj = b->ptrObjRef;
  if (b->inner!=NULL && b->size!=0) memcpy(b->src, b->tgt, b->size);
  return b->src;
}



typedef struct MemSearch {
  uintptr_t target;
  uintptr_t bestStart, bestEnd;
  CheckedInnerBlock* inner;
  ux distance;
  const char* bestName;
} MemSearch;
void searchRange(void* start, ux size, const char* name, void* extra, void* data) {
  MemSearch* m = data;
  if (m->distance == 0) return;
  uintptr_t s = PTR_TO_U64(start), e = s+size, tgt = m->target;
  ux distance = tgt<s? s-tgt : tgt>e? tgt-e : 0;
  if (name == checked_outerBuffer) distance += 4096; // prefer FFI inner buffers when available
  // log_printf("comparing to range %s %p..%ld: %lx %ld\n", name, start, size, m->target, distance);
  if (distance < m->distance) {
    m->distance = distance;
    m->bestStart = s;
    m->bestEnd = e;
    m->bestName = name;
    m->inner = name==checked_innerBuffer? (CheckedInnerBlock*)extra : NULL;
  }
}

MAYBE_UNUSED static void handler_core(char* raw, uintptr_t mem) {
  fprintf(
    stderr,
    "Fatal error: the process attempted to incorrectly access a memory location (%s).\n"
    "This could be due to an incorrect FFI invocation, a bug in foreign code, or a bug in CBQN.\n",
    raw
  );
  fprintf(stderr, "Accessed memory address: 0x" N64x "\n", (u64) mem);
  fflush(NULL);
  if (mem < 4096) {
    fprintf(stderr, "Hint: this is a null pointer%s\n", mem==0? "" : ", with a small offset");
  } else {
    MemSearch m = { .target=mem, .distance=~(ux)0 };
    forAllMemoryRanges(searchRange, &m);
    
    const char* name = m.bestName;
    if (name == NULL) name = "(unknown)"; // shouldn't happen, but we definitely don't want null-pointer dereferences here
    if (!strcmp(name, "mm_") || !strcmp(name, "b1_") || !strcmp(name, "b3_")) name = "CBQN heap";
    if (!strcmp(name, "mmX_")) name = "CBQN JIT memory";
    
    fprintf(stderr, "Hint: closest CBQN-managed memory block to this address:\n");
    fprintf(stderr, "  Start: 0x" N64x "\n", (u64) m.bestStart);
    fprintf(stderr, "  End  : 0x" N64x "\n", (u64) m.bestEnd);
    fprintf(stderr, "  Relative position of accessed address: ");
    if      (mem < m.bestStart) fprintf(stderr, N64u " bytes before the start of the block", (u64)(m.bestStart-mem));
    else if (mem >=  m.bestEnd) fprintf(stderr, N64u " bytes after the end of the block", (u64)(mem-m.bestEnd));
    else                        fprintf(stderr, N64u " bytes into the block (" N64u " bytes from the end)", (u64)(mem-m.bestStart), (u64)(m.bestEnd-mem));
    fprintf(stderr, "\n");
    
    fprintf(stderr, "  Block type: %s\n", name);
    fflush(stderr);
    if (m.inner != NULL) {
      fprintf(stderr, "  FFI buffer info:\n");
      char* mode = m.inner->writable? "writable" : "read-only";
      if (m.inner->freed) fprintf(stderr, "    Was %s, is now illegal to access as the FFI call that required this buffer for an argument returned\n", mode);
      else fprintf(stderr, "    Is %s\n", mode);
      ux size = m.inner->szx - m.inner->szxOver;
      fprintf(stderr, "    FFI buffer is " N64u " byte%s\n", (u64)size, size!=1? "s" : "");
    }
    fflush(stderr);
  }
  fprintf(stderr, "Attempting to print BQN stacktrace:\n");
  vm_pstLive();
  fflush(stderr);
}