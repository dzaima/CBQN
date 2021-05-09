#pragma once
#include "h.h"
typedef struct Scope Scope;
typedef struct Body Body;
typedef struct NSDesc NSDesc;
void m_nsDesc(Body* body, bool imm, u8 ty, B nameList, B varIDs, B exported); // consumes nameList
B m_ns(Scope* sc, NSDesc* desc); // consumes both
B ns_getU(B ns, B nameList, i32 nameID); // doesn't consume anything, doesn't increment result
B ns_nameList(NSDesc* d);