#pragma once
#include "vm.h"

typedef struct NSDesc {
  struct Value;
  B nameList;
  i32 varAm; // number of items in expIDs (currently equal to sc->varAm/body->varAm)
  i32 expIDs[]; // each item is an index in nameList (or -1), and its position is the corresponding position in sc->vars
} NSDesc;
typedef struct NS {
  struct Value;
  B nameList; // copy of desc->nameList for quick checking; isn't owned
  NSDesc* desc;
  Scope* sc;
} NS;

void m_nsDesc(Body* body, bool imm, u8 ty, B nameList, B varIDs, B exported); // consumes nameList
B m_ns(Scope* sc, NSDesc* desc); // consumes both
B ns_getU(B ns, B nameList, i32 nameID); // doesn't consume anything, doesn't increment result
B ns_getNU(B ns, B name); // doesn't consume anything, doesn't increment result
void ns_set(B ns, B name, B val); // consumes val
i32 ns_pos(B ns, B name); // consumes name; returns an index in sc->vars for any variable, exported or local
