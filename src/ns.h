#pragma once
#include "vm.h"

typedef struct NSDesc {
  struct Value;
  i32 varAm; // number of items in expGIDs (currently equal to sc->varAm/body->varAm)
  i32 expGIDs[]; // for each variable; -1 if not exported, otherwise a gid
} NSDesc;
typedef struct NS {
  struct Value;
  NSDesc* desc;
  Scope* sc;
} NS;

void m_nsDesc(Body* body, bool imm, u8 ty, i32 vam, B nameList, B varIDs, B exported); // doesn't consume nameList
B m_ns(Scope* sc, NSDesc* desc); // consumes both

B ns_getU(B ns, i32 gid); // doesn't consume, doesn't increment result
B ns_qgetU(B ns, i32 gid); // ns_getU but return bi_N on fail
B ns_getNU(B ns, B name, bool thrEmpty); // doesn't consume anything, doesn't increment result; returns bi_N if doesn't exist and !thrEmpty
B ns_getC(B ns, char* name); // get namespace field by C string; returns bi_N if doesn't exist
void ns_set(B ns, B name, B val); // consumes val

i32 pos2gid(Body* body, i32 pos); // converts a variable position to a gid; errors on special name variables
i32 str2gid(B s); // doesn't consume
i32 str2gidQ(B s); // doesn't consume
B gid2str(i32 n); // returns unowned object


#define m_nnsDesc(...) ({ char* names_[] = {__VA_ARGS__}; m_nnsDescF(sizeof(names_)/sizeof(char*), names_); })
#define m_nns(desc, ...) ({ B vals_[] = {__VA_ARGS__}; m_nnsF(desc, sizeof(vals_)/sizeof(B), vals_); })

Body* m_nnsDescF(i32 n, char** names); // adds the result as a permanent GC root
B m_nnsF(Body* desc, i32 n, B* vals);
i32 nns_pos(Body* desc, B name); // consumes name; returns an index in sc->vars for a given variable (exported or local)
