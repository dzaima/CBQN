#pragma once

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
  B indices;
  HArr* objs;
  u32 blockAm;
} Comp;

struct BlBlocks {
  struct Value;
  u32 am;
  Block* a[];
};

struct Block {
  struct Value;
  bool imm;
  u8 ty;
  Body* body;
};

typedef struct NSDesc NSDesc;
struct Body {
  struct Value;
  Comp* comp;
  BlBlocks* blocks;
  // B* objs;
  u32* bc; // pointer in an owned I32Arr
  i32* map; // pointer in an owned I32Arr
  u8* nvm; // either NULL or a pointer to machine code otherwise
  u32 maxStack;
  u16 maxPSC;
  u16 varAm;
  NSDesc* nsDesc;
  B nvmRefs;
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
Block* compile(B bcq, B objs, B blocksq, B indices, B tokenInfo, B src, Scope* sc);
Scope* m_scope(Body* body, Scope* psc, u16 varAm, i32 initVarAm, B* initVars);
B evalBC(Body* b, Scope* sc); // doesn't consume; executes bytecode of the body directly in the scope

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
  Scope* sc;
  union { u32* bcL; i32 bcV; };
} Env;
extern Env* envCurr;
extern Env* envStart;
extern Env* envEnd;
static inline void pushEnv(Scope* sc, u32* bc) {
  if (envCurr==envEnd) thrM("Stack overflow");
  envCurr->sc = sc;
  envCurr->bcL = bc;
  envCurr++;
}
static inline void popEnv() {
  assert(envCurr>envStart);
  envCurr--;
}
void vm_pst(Env* s, Env* e);
void vm_pstLive();
void vm_printPos(Comp* comp, i32 bcPos, i64 pos);



typedef struct FldAlias {
  struct Value;
  B obj;
  i32 p;
} FldAlias;


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
static inline void v_setI(Scope* sc, u32 p, B x, bool upd) { // consumes x
  B prev = sc->vars[p];
  if (upd) {
    if (prev.u==bi_noVar.u) thrM("↩: Updating undefined variable");
    dec(prev);
  }
  sc->vars[p] = x;
}
static inline void v_set(Scope* pscs[], B s, B x, bool upd) { // doesn't consume
  if (RARE(!isVar(s))) return v_setR(pscs, s, x, upd);
  v_setI(pscs[(u16)(s.u>>32)], (u32)s.u, inc(x), upd);
}
