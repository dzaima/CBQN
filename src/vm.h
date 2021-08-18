#pragma once


#if (defined(__x86_64) || defined(__amd64__)) && defined(MAP_32BIT) && MM!=0
  #ifndef JIT_START
    #define JIT_START 2 // number of calls for when to start JITting. -1: never JIT; 0: JIT everything, n: JIT after n non-JIT invocations; max ¯1+2⋆16
  #endif
#else
  #undef JIT_START
  #define JIT_START -1
#endif

enum {
  PUSH =  0, // N; push object from objs[N]
  VARO =  1, // N; push variable with name strs[N]
  VARM =  2, // N; push mutable variable with name strs[N]
  ARRO =  3, // N; create a vector of top N items
  ARRM =  4, // N; create a mutable vector of top N items
  FN1C =  5, // monadic function call ⟨…,x,f  ⟩ → F x
  FN2C =  6, //  dyadic function call ⟨…,x,f,w⟩ → w F x
  OP1D =  7, // derive 1-modifier to function; ⟨…,  _m,f⟩ → (f _m)
  OP2D =  8, // derive 2-modifier to function; ⟨…,g,_m,f⟩ → (f _m_ g)
  TR2D =  9, // derive 2-train aka atop; ⟨…,  g,f⟩ → (f g)
  TR3D = 10, // derive 3-train aka fork; ⟨…,h,g,f⟩ → (f g h)
  SETN = 11, // set new; _  ←_; ⟨…,x,  mut⟩ → mut←x
  SETU = 12, // set upd; _  ↩_; ⟨…,x,  mut⟩ → mut↩x
  SETM = 13, // set mod; _ F↩_; ⟨…,x,F,mut⟩ → mut F↩x
  POPS = 14, // pop object from stack
  DFND = 15, // N; push dfns[N], derived to current scope
  FN1O = 16, // optional monadic call (FN1C but checks for · at 𝕩)
  FN2O = 17, // optional  dyadic call (FN2C but checks for · at 𝕩 & 𝕨)
  CHKV = 18, // throw error if top of stack is ·
  TR3O = 19, // TR3D but creates an atop if F is ·
  OP2H = 20, // derive 2-modifier to 1-modifier ⟨…,g,_m_⟩ → (_m_ g)
  LOCO = 21, // N0,N1; push variable at depth N0 and position N1
  LOCM = 22, // N0,N1; push mutable variable at depth N0 and position N1
  VFYM = 23, // push a mutable version of ToS that fails if set to a non-equal value (for header assignment)
  SETH = 24, // set header; acts like SETN, but it doesn't push to stack, and, instead of erroring in cases it would, it skips to the next body
  RETN = 25, // returns top of stack
  FLDO = 26, // N; get field nameList[N] from ToS
  FLDM = 27, // N; get mutable field nameList[N] from ToS
  NSPM = 28, // N; replace ToS with one with a namespace field alias N
  RETD = 29, // return a namespace of exported items
  SYSV = 30, // N; get system function N
  LOCU = 31, // N0,N1; like LOCO but overrides the slot with bi_optOut
  EXTO, EXTM, EXTU, // alternate versions of LOC_ for extended variables
  ADDI, ADDU, // separate PUSH for refcounting needed/not needed (stores the object inline as 2 u32s, instead of reading from `objs`)
  FN1Ci, FN1Oi, FN2Ci, FN2Oi, // FN__ alternatives that don't take the function from the stack, but instead as an 2×u32 immediate in the bytecode
  SETNi, SETUi, SETMi, // SET_ alternatives that expect the set variable as a depth-position pair like LOC_
  SETNv, SETUv, SETMv, // SET_i alternatives that also don't return the result
  FAIL, // no body matched
  BC_SIZE
};

typedef struct Comp Comp;
typedef struct BlBlocks BlBlocks;
typedef struct Block Block;
typedef struct Body Body;
typedef struct Scope Scope;
typedef struct ScopeExt ScopeExt;

typedef struct Comp {
  struct Value;
  B bc;
  B src;
  B path;
  B indices;
  HArr* objs;
  u32 blockAm;
} Comp;

struct BlBlocks {
  struct Value;
  u32 am;
  Block* a[];
};

typedef struct NSDesc NSDesc;
struct Block {
  struct Value;
  Comp* comp;
  bool imm;
  u8 ty;
  Block** blocks; // pointer in an owned BlBlocks, or null
  i32* map; // pointer in an owned I32Arr
  i32* bc; // pointer in an owned I32Arr
  i32 bodyCount;
  Body* dyBody; // pointer within bodies; not owned; TODO move to the second item of bodies or something
  Body* bodies[]; // bodies[0] is the first monadic body (also niladic body)
};
struct Body {
  struct Value;
#if JIT_START != -1
  u8* nvm; // either NULL or a pointer to machine code
#endif
#if JIT_START > 0
  u16 callCount;
#endif
  union { u32* bc; i32 bcTmp; }; // pointer in bl->bc; bcTmp to make ubsan happy (god dammit C)
  u32 maxStack;
  u16 maxPSC;
#if JIT_START != -1
  B nvmRefs;
#endif
  Block* bl; // non-owned pointer to corresponding block
  NSDesc* nsDesc;
  u16 varAm;
  i32 varIDs[];
};

struct ScopeExt {
  struct Value;
  u16 varAm;
  B vars[]; // vars has length varAm*2; position varAm and onwards are corresponding names to variables at regular indexes
};

struct Scope {
  struct Value;
  Scope* psc;
  Body* body; // last place where code was executed in this scope; also used for variable name resolution, so can only safely be replaced if varAm==0
  u16 varAm;
  ScopeExt* ext; // will probably be NULL
  B vars[];
};

Block* bqn_comp(B str, B path, B args);
Block* bqn_compSc(B str, B path, B args, Scope* sc, bool repl);
Block* compile(B bcq, B objs, B blocks, B bodies, B indices, B tokenInfo, B src, B path, Scope* sc);
Scope* m_scope(Body* body, Scope* psc, u16 varAm, i32 initVarAm, B* initVars);
B execBlockInline(Block* block, Scope* sc); // doesn't consume; executes bytecode of the monadic body directly in the scope

u32* nextBC(u32* p);
i32 stackDiff(u32* p);
i32 stackConsumed(u32* p);
i32 stackAdded(u32* p);
char* nameBC(u32* p);


typedef struct FunBlock { struct Fun; Scope* sc; Block* bl; } FunBlock;
typedef struct Md1Block { struct Md1; Scope* sc; Block* bl; } Md1Block;
typedef struct Md2Block { struct Md2; Scope* sc; Block* bl; } Md2Block;
// all don't consume anything
B m_funBlock(Block* bl, Scope* psc); // may return evaluated result, whatever
B m_md1Block(Block* bl, Scope* psc);
B m_md2Block(Block* bl, Scope* psc);






typedef struct Env {
  u64 pos; // if top bit set, ((u32)pos)>>1 is an offset into bytecode; otherwise, it's a pointer in the bytecode
  Scope* sc;
} Env;
extern Env* envCurr;
extern Env* envStart;
extern Env* envEnd;
extern u64 envPrevHeight; // envStart+prevEnvHeight gives envCurr+1 from before the error
static inline void pushEnv(Scope* sc, u32* bc) {
  if (envCurr+1==envEnd) thrM("Stack overflow");
  envCurr++;
  envCurr->sc = sc;
  envCurr->pos = (u64)bc;
}
static inline void popEnv() {
  assert(envCurr>=envStart);
  envCurr--;
}
FORCE_INLINE void scope_dec(Scope* sc) { // version of ptr_dec for scopes, that tries to also free trivial cycles
  i32 varAm = sc->varAm;
  if (sc->refc>1) {
    i32 innerRef = 1;
    for (i32 i = 0; i < varAm; i++) {
      B c = sc->vars[i];
      if (isVal(c) && v(c)->refc==1) {
        u8 t = v(c)->type;
        if      (t==t_fun_block && c(FunBlock,c)->sc==sc) innerRef++;
        else if (t==t_md1_block && c(Md1Block,c)->sc==sc) innerRef++;
        else if (t==t_md2_block && c(Md2Block,c)->sc==sc) innerRef++;
      }
    }
    assert(innerRef <= sc->refc);
    if (innerRef==sc->refc) {
      value_free((Value*)sc);
      return;
    }
  }
  ptr_dec(sc);
}
void vm_pst(Env* s, Env* e);
void vm_pstLive(void);
void vm_printPos(Comp* comp, i32 bcPos, i64 pos);
NOINLINE B vm_fmtPoint(B src, B prepend, B path, usz cs, usz ce); // consumes prepend
NOINLINE void printErrMsg(B msg);
NOINLINE void unwindEnv(Env* envNew); // envNew==envStart-1 for emptying the env stack
NOINLINE void unwindCompiler(void); // unwind to the env of the invocation of the compiler; UB when not in compiler!


typedef struct FldAlias {
  struct Value;
  B obj;
  i32 p;
} FldAlias;
typedef struct VfyObj {
  struct Value;
  B obj;
} VfyObj;


NOINLINE B v_getR(Scope* pscs[], B s); // doesn't consume
static inline B v_getI(Scope* sc, u32 p) {
  B r = sc->vars[p];
  sc->vars[p] = bi_optOut;
  return r;
}
static inline B v_get(Scope* pscs[], B s) { // get value representing s, replacing with bi_optOut; doesn't consume
  if (RARE(!isVar(s))) return v_getR(pscs, s);
  return v_getI(pscs[(u16)(s.u>>32)], (u32)s.u);
}

NOINLINE void v_setR(Scope* pscs[], B s, B x, bool upd); // doesn't consume
NOINLINE bool v_sethR(Scope* pscs[], B s, B x); // doesn't consume
static inline void v_setI(Scope* sc, u32 p, B x, bool upd) { // consumes x
  B prev = sc->vars[p];
  if (upd) {
    if (prev.u==bi_noVar.u) thrM("↩: Updating undefined variable");
    dec(prev);
  }
  sc->vars[p] = x;
}
static inline void v_set(Scope* pscs[], B s, B x, bool upd) { // doesn't consume
  if (RARE(!isVar(s))) v_setR(pscs, s, x, upd);
  else v_setI(pscs[(u16)(s.u>>32)], (u32)s.u, inc(x), upd);
}

static inline bool v_seth(Scope* pscs[], B s, B x) { // doesn't consume; s cannot contain extended variables
  if (RARE(!isVar(s))) return v_sethR(pscs, s, x);
  v_setI(pscs[(u16)(s.u>>32)], (u32)s.u, inc(x), false);
  return true;
}
