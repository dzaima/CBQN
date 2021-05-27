#pragma once
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
  i32* bc; // pointer in an owned I32Arr
  i32* map; // pointer in an owned I32Arr
  u32 maxStack;
  u16 maxPSC;
  u16 varAm;
  NSDesc* nsDesc;
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
  Body* body;
  u16 varAm;
  ScopeExt* ext;
  B vars[];
};


Block* compile(B bcq, B objs, B blocksq, B indices, B tokenInfo, B src, Scope* sc);
Scope* m_scope(Body* body, Scope* psc, u16 varAm);
B evalBC(Body* b, Scope* sc); // doesn't consume; executes bytecode of the body directly in the scope

typedef struct Env {
  Scope* sc;
  union { i32* bcL; i32 bcV; };
} Env;

void vm_pst(Env* s, Env* e);
void vm_pstLive();

typedef struct FunBlock { struct Fun; Scope* sc; Block* bl; } FunBlock;
typedef struct Md1Block { struct Md1; Scope* sc; Block* bl; } Md1Block;
typedef struct Md2Block { struct Md2; Scope* sc; Block* bl; } Md2Block;
// all don't consume anything
B m_funBlock(Block* bl, Scope* psc); // may return evaluated result, whatever
B m_md1Block(Block* bl, Scope* psc);
B m_md2Block(Block* bl, Scope* psc);






extern Env* envCurr;
extern Env* envStart;
extern Env* envEnd;

static inline void pushEnv(Scope* sc, i32* bc) {
  if (envCurr==envEnd) thrM("Stack overflow");
  envCurr->sc = sc;
  envCurr->bcL = bc;
  envCurr++;
}
static inline void popEnv() {
  assert(envCurr>envStart);
  envCurr--;
}
