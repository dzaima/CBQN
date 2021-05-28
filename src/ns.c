#include "core.h"
#include "ns.h"

void m_nsDesc(Body* body, bool imm, u8 ty, B nameList, B varIDs, B exported) { // consumes nameList
  if (!isArr(varIDs) || !isArr(exported)) thrM("Bad namespace description information");
  
  usz ia = a(varIDs)->ia;
  if (ia!=a(exported)->ia) thrM("Bad namespace description information");
  i32 off = (ty==0?0:ty==1?2:3) + (imm?0:3);
  i32 vam = ia+off;
  if (vam != body->varAm) thrM("Bad namespace description information");
  
  NSDesc* rc = c(NSDesc, mm_alloc(fsizeof(NSDesc, expIDs, i32, vam), t_nsDesc, ftag(OBJ_TAG)));
  rc->nameList = nameList;
  rc->varAm = vam;
  BS2B getIDU = TI(varIDs).getU;
  BS2B getOnU = TI(exported).getU;
  for (i32 i = 0; i < off; i++) {
    body->varIDs[i] = -1;
    rc  ->expIDs[i] = -1;
  }
  for (usz i = 0; i < ia; i++) {
    i32 cid = o2i(getIDU(varIDs, i));
    bool cexp = o2b(getOnU(exported, i));
    body->varIDs[i+off] = cid;
    rc->expIDs[i+off] = cexp? cid : -1;
  }
  // printf("def %p:\n", rc);
  // for (usz i = 0; i < vam; i++) printf("  %d: %d %d\n", i, body->varIDs[i], rc->expIDs[i]);
  body->nsDesc = rc;
}
B m_ns(Scope* sc, NSDesc* desc) { // consumes both
  B r = mm_alloc(sizeof(NS), t_ns, ftag(NSP_TAG));
  NS* rc = c(NS,r);
  rc->desc = desc;
  rc->nameList = rc->desc->nameList;
  rc->sc = sc;
  return r;
}

B ns_getU(B ns, B cNL, i32 nameID) {
  NS* n = c(NS, ns);
  NSDesc* d = n->desc;
  i32 dVarAm = d->varAm;
  assert(nameID<a(cNL)->ia && nameID>=0);
  B dNL = d->nameList;
  if (cNL.u != dNL.u) {
    B cName = TI(cNL).getU(cNL, nameID);
    BS2B dNLgetU = TI(dNL).getU;
    for (i32 i = 0; i < dVarAm; i++) {
      i32 dID = d->expIDs[i];
      if (dID>=0 && equal(dNLgetU(dNL, dID), cName)) return n->sc->vars[i];
    }
  } else {
    for (i32 i = 0; i < dVarAm; i++) {
      if (d->expIDs[i]==nameID) return n->sc->vars[i];
    }
  }
  thrM("No key found");
}
B ns_getNU(B ns, B name) {
  NS* n = c(NS, ns);
  NSDesc* d = n->desc;
  i32 dVarAm = d->varAm;
  B dNL = d->nameList;
  BS2B dNLgetU = TI(dNL).getU;
  for (i32 i = 0; i < dVarAm; i++) {
    i32 dID = d->expIDs[i];
    if (dID>=0 && equal(dNLgetU(dNL, dID), name)) return n->sc->vars[i];
  }
  thrM("No key found");
}

B ns_nameList(NSDesc* d) {
  return d->nameList;
}



void ns_free(Value* x) {
  NS* c = (NS*)x;
  ptr_decR(c->desc);
  ptr_decR(c->sc);
}
void ns_visit(Value* x) {
  NS* c = (NS*)x;
  mm_visitP(c->desc);
  mm_visitP(c->sc);
}
void ns_print(B x) {
  putchar('{');
  NSDesc* desc = c(NS,x)->desc;
  Scope* sc = c(NS,x)->sc;
  i32 am = desc->varAm;
  BS2B getNameU = TI(desc->nameList).getU;
  bool first = true;
  for (i32 i = 0; i < am; i++) {
    i32 id = desc->expIDs[i];
    if (id>=0) {
      if (first) first=false;
      else printf(" ⋄ ");
      printRaw(getNameU(desc->nameList, id));
      printf("⇐");
      print(sc->vars[i]);
    }
  }
  putchar('}');
}

void nsDesc_free(Value* x) {
  decR(((NSDesc*)x)->nameList);
}
void nsDesc_visit(Value* x) {
  mm_visit(((NSDesc*)x)->nameList);
}
void nsDesc_print(B x) {
  printf("(namespace description)");
}



void ns_init() {
  ti[t_ns].free  = ns_free;  ti[t_nsDesc].free  = nsDesc_free;
  ti[t_ns].visit = ns_visit; ti[t_nsDesc].visit = nsDesc_visit;
  ti[t_ns].print = ns_print; ti[t_nsDesc].print = nsDesc_print;
}
