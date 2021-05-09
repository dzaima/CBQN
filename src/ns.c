#include "ns.h"

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

B ns_getU(B ns, B nameList, i32 nameID) {
  NS* n = c(NS, ns);
  NSDesc* d = n->desc;
  if (n->nameList.u != nameList.u) thrM("TODO cross-program namespace access");
  i32 varAm = d->varAm;
  // printf("getting from %p with vam=%d\n", d, d->varAm);
  assert(nameID<a(nameList)->ia && nameID>=0);
  for (i32 i = 0; i < varAm; i++) {
    if (d->expIDs[i]==nameID) return n->sc->vars[i];
  }
  thrM("No key found");
}

B ns_nameList(NSDesc* d) {
  return d->nameList;
}



void ns_free(B x) {
  NS* c = c(NS, x);
  ptr_decR(c->desc);
  ptr_decR(c->sc);
}
void ns_visit(B x) {
  NS* c = c(NS, x);
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

void nsDesc_free(B x) {
  decR(c(NSDesc,x)->nameList);
}
void nsDesc_visit(B x) {
  mm_visit(c(NSDesc,x)->nameList);
}
void nsDesc_print(B x) {
  printf("(namespace description)");
}
static inline void ns_init() {
  ti[t_ns].free  = ns_free;  ti[t_nsDesc].free  = nsDesc_free;
  ti[t_ns].visit = ns_visit; ti[t_nsDesc].visit = nsDesc_visit;
  ti[t_ns].print = ns_print; ti[t_nsDesc].print = nsDesc_print;
}
