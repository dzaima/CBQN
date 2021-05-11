#pragma once
#include "h.h"
typedef struct Block Block;
typedef struct Body Body;
typedef struct Scope Scope;

typedef struct Comp {
  struct Value;
  B bc;
  B src;
  B indices;
  HArr* objs;
  u32 blockAm;
  Block* blocks[];
} Comp;

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
  // B* objs;
  i32* bc; // pointer in comp->bc
  u32 maxStack;
  u16 maxPSC;
  u16 varAm;
  u32 endStack;
  NSDesc* nsDesc;
  i32 varIDs[];
};

struct Scope {
  struct Value;
  Scope* psc;
  Body* body;
  u16 varAm;
  B vars[];
};

typedef struct FunBlock { struct Fun; Scope* sc; Block* bl; } FunBlock;
typedef struct Md1Block { struct Md1; Scope* sc; Block* bl; } Md1Block;
typedef struct Md2Block { struct Md2; Scope* sc; Block* bl; } Md2Block;
