#include "core.h"
#include "ns.h"
#include "utils/mut.h"

void m_nsDesc(Body* body, bool imm, u8 ty, B nameList, B varIDs, B exported) { // consumes nameList
  if (!isArr(varIDs) || !isArr(exported)) thrM("Bad namespace description information");
  
  usz ia = a(varIDs)->ia;
  if (ia!=a(exported)->ia) thrM("Bad namespace description information");
  i32 off = (ty==0?0:ty==1?2:3) + (imm?0:3);
  i32 vam = ia+off;
  // if (vam != body->varAm) thrM("Bad namespace description information"); // arg remapping makes body->varAm unrelated to named variable count
  
  NSDesc* r = mm_alloc(fsizeof(NSDesc, expIDs, i32, vam), t_nsDesc);
  r->nameList = nameList;
  r->varAm = vam;
  SGetU(varIDs)
  SGetU(exported)
  for (i32 i = 0; i < off; i++) {
    body->varIDs[i] = -1;
    r   ->expIDs[i] = -1;
  }
  for (usz i = 0; i < ia; i++) {
    i32 cid = o2i(GetU(varIDs, i));
    bool cexp = o2b(GetU(exported, i));
    body->varIDs[i+off] = cid;
    r->expIDs[i+off] = cexp? cid : -1;
  }
  // printf("def %p:\n", r);
  // for (usz i = 0; i < vam; i++) printf("  %d: %d %d\n", i, body->varIDs[i], r->expIDs[i]);
  body->nsDesc = r;
}
B m_ns(Scope* sc, NSDesc* desc) { // consumes both
  NS* r = mm_alloc(sizeof(NS), t_ns);
  r->desc = desc;
  r->nameList = r->desc->nameList;
  r->sc = sc;
  return tag(r,NSP_TAG);
}

B ns_getU(B ns, B cNL, i32 nameID) { VTY(ns, t_ns);
  NS* n = c(NS, ns);
  NSDesc* d = n->desc;
  i32 dVarAm = d->varAm;
  assert((u64)nameID < a(cNL)->ia  &&  nameID>=0);
  B dNL = d->nameList;
  if (cNL.u != dNL.u) {
    B cName = IGetU(cNL, nameID);
    SGetU(dNL)
    for (i32 i = 0; i < dVarAm; i++) {
      i32 dID = d->expIDs[i];
      if (dID>=0 && equal(GetU(dNL, dID), cName)) return n->sc->vars[i];
    }
  } else {
    for (i32 i = 0; i < dVarAm; i++) {
      if (d->expIDs[i]==nameID) return n->sc->vars[i];
    }
  }
  thrM("No key found");
}
B ns_qgetU(B ns, B cNL, i32 nameID) { VTY(ns, t_ns); // TODO somehow merge impl with ns_getU
  NS* n = c(NS, ns);
  NSDesc* d = n->desc;
  i32 dVarAm = d->varAm;
  assert((u64)nameID < a(cNL)->ia  &&  nameID>=0);
  B dNL = d->nameList;
  if (cNL.u != dNL.u) {
    B cName = IGetU(cNL, nameID);
    SGetU(dNL)
    for (i32 i = 0; i < dVarAm; i++) {
      i32 dID = d->expIDs[i];
      if (dID>=0 && equal(GetU(dNL, dID), cName)) return n->sc->vars[i];
    }
  } else {
    for (i32 i = 0; i < dVarAm; i++) {
      if (d->expIDs[i]==nameID) return n->sc->vars[i];
    }
  }
  return bi_N;
}
B ns_getNU(B ns, B name, bool thrEmpty) { VTY(ns, t_ns);
  NS* n = c(NS, ns);
  NSDesc* d = n->desc;
  i32 dVarAm = d->varAm;
  B dNL = d->nameList;
  SGetU(dNL)
  for (i32 i = 0; i < dVarAm; i++) {
    i32 dID = d->expIDs[i];
    if (dID>=0 && equal(GetU(dNL, dID), name)) return n->sc->vars[i];
  }
  if (thrEmpty) thrM("No key found");
  return bi_N;
}
void ns_set(B ns, B name, B val) { VTY(ns, t_ns);
  NS* n = c(NS, ns);
  NSDesc* d = n->desc;
  i32 dVarAm = d->varAm;
  B dNL = d->nameList;
  SGetU(dNL)
  for (i32 i = 0; i < dVarAm; i++) {
    i32 dID = d->expIDs[i];
    if (dID>=0 && equal(GetU(dNL, dID), name)) {
      dec(n->sc->vars[i]);
      n->sc->vars[i] = val;
      return;
    }
  }
  thrM("No key found");
}

i32 ns_pos(B ns, B name) { VTY(ns, t_ns);
  Body* b = c(NS, ns)->sc->body;
  B nameList = c(NS, ns)->desc->nameList;
  i32 bVarAm = b->varAm;
  SGetU(nameList)
  for (i32 i = 0; i < bVarAm; i++) {
    i32 id = b->varIDs[i];
    if (id>=0) if (equal(GetU(nameList, id), name)) { dec(name); return i; }
  }
  thrM("No key found");
}

B ns_nameList(NSDesc* d) {
  return d->nameList;
}



DEF_FREE(ns) {
  NS* c = (NS*)x;
  ptr_decR(c->desc);
  ptr_dec(c->sc);
}
static void ns_visit(Value* x) {
  NS* c = (NS*)x;
  mm_visitP(c->desc);
  mm_visitP(c->sc);
}
static void ns_print(B x) {
  putchar('{');
  NSDesc* desc = c(NS,x)->desc;
  Scope* sc = c(NS,x)->sc;
  i32 am = desc->varAm;
  B nl = desc->nameList; SGetU(nl)
  bool first = true;
  for (i32 i = 0; i < am; i++) {
    i32 id = desc->expIDs[i];
    if (id>=0) {
      if (first) first=false;
      else printf(" ⋄ ");
      printRaw(GetU(nl, id));
      printf("⇐");
      print(sc->vars[i]);
    }
  }
  putchar('}');
}
B nsFmt(B x) { // consumes
  B s = emptyCVec();
  ACHR('{');
  NSDesc* desc = c(NS,x)->desc;
  i32 am = desc->varAm;
  B nl = desc->nameList; SGetU(nl)
  bool first = true;
  for (i32 i = 0; i < am; i++) {
    i32 id = desc->expIDs[i];
    if (id>=0) {
      if (first) first=false;
      else ACHR(U'‿');
      AFMT("%R", GetU(nl, id));
    }
  }
  AU("⇐}");
  dec(x);
  return s;
}

DEF_FREE(nsDesc) { decR(((NSDesc*)x)->nameList); }
static void nsDesc_visit(Value* x) { mm_visit(((NSDesc*)x)->nameList); }
static void nsDesc_print(B x) { printf("(namespace description)"); }



void ns_init() {
  TIi(t_ns,freeO) = ns_freeO; TIi(t_nsDesc,freeO) = nsDesc_freeO;
  TIi(t_ns,freeF) = ns_freeF; TIi(t_nsDesc,freeF) = nsDesc_freeF;
  TIi(t_ns,visit) = ns_visit; TIi(t_nsDesc,visit) = nsDesc_visit;
  TIi(t_ns,print) = ns_print; TIi(t_nsDesc,print) = nsDesc_print;
}
