#include "core.h"
#include "ns.h"
#include "utils/mut.h"

void m_nsDesc(Body* body, bool imm, u8 ty, i32 actualVam, B nameList, B varIDs, B exported) { // doesn't consume nameList
  if (!isArr(varIDs) || !isArr(exported)) thrM("Bad namespace description information");
  
  usz ia = a(varIDs)->ia;
  if (ia!=a(exported)->ia) thrM("Bad namespace description information");
  i32 off = (ty==0?0:ty==1?2:3) + (imm?0:3);
  i32 vam = ia+off;
  
  NSDesc* r = mm_alloc(fsizeof(NSDesc, expGIDs, i32, actualVam<2?2:actualVam), t_nsDesc);
  r->varAm = vam;
  SGetU(varIDs)
  SGetU(exported)
  for (i32 i = 0; i < actualVam; i++) {
    body->varData[i] = -1;
    r   ->expGIDs[i] = -1;
    body->varData[i+actualVam] = -1;
  }
  
  SGetU(nameList)
  for (usz i = 0; i < ia; i++) {
    i32 cid = o2i(GetU(varIDs, i));
    bool cexp = o2b(GetU(exported, i));
    body->varData[i+off + actualVam] = cid;
    r->expGIDs[i+off] = cexp? str2gid(GetU(nameList, cid)) : -1;
  }
  body->nsDesc = r;
}
B m_ns(Scope* sc, NSDesc* desc) { // consumes both
  NS* r = mm_alloc(sizeof(NS), t_ns);
  r->desc = desc;
  r->sc = sc;
  return tag(r,NSP_TAG);
}


i32 pos2gid(Body* body, i32 pos) {
  i32 gid = body->varData[pos];
  if (LIKELY(gid!=-1)) return gid;
  
  i32 nlIdx = body->varData[pos+body->varAm];
  if (nlIdx==-1) thrM("Cannot use special variable name as namespace key");
  return body->varData[pos] = str2gid(IGetU(body->bl->comp->nameList, nlIdx));
}


B ns_getU(B ns, i32 gid) { VTY(ns, t_ns);
  NS* n = c(NS, ns);
  NSDesc* d = n->desc;
  
  i32 ia = d->varAm;
  for (i32 i = 0; i < ia; i++) if (d->expGIDs[i]==gid) return n->sc->vars[i];
  thrM("No key found");
}

B ns_qgetU(B ns, i32 gid) { VTY(ns, t_ns);
  NS* n = c(NS, ns);
  NSDesc* d = n->desc;
  
  i32 ia = d->varAm;
  for (i32 i = 0; i < ia; i++) if (d->expGIDs[i]==gid) return n->sc->vars[i];
  return bi_N;
}

B ns_getNU(B ns, B name, bool thrEmpty) { VTY(ns, t_ns);
  NS* n = c(NS, ns);
  NSDesc* d = n->desc;
  i32 gid = str2gid(name);
  
  i32 ia = d->varAm;
  for (i32 i = 0; i < ia; i++) if (d->expGIDs[i]==gid) return n->sc->vars[i];
  if (thrEmpty) thrM("No key found");
  return bi_N;
}

void ns_set(B ns, B name, B val) { VTY(ns, t_ns);
  NS* n = c(NS, ns);
  Scope* sc = n->sc;
  NSDesc* d = n->desc;
  i32 gid = str2gid(name);
  
  i32 ia = d->varAm;
  for (i32 i = 0; i < ia; i++) {
    if (d->expGIDs[i]==gid) {
      dec(sc->vars[i]);
      sc->vars[i] = val;
      return;
    }
  }
  thrM("No key found");
}

i32 ns_pos(B ns, B name) { VTY(ns, t_ns);
  NS* n = c(NS, ns);
  Body* body = n->sc->body;
  B nameList = body->bl->comp->nameList; SGetU(nameList);
  
  i32 ia = body->varAm;
  for (i32 i = 0; i < ia; i++) {
    i32 pos = body->varData[i+ia];
    if (pos>=0 && equal(name, GetU(nameList, pos))) return i;
  }
  thrM("No key found");
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
  bool first = true;
  for (i32 i = 0; i < am; i++) {
    i32 id = desc->expGIDs[i];
    if (id>=0) {
      if (first) first=false;
      else printf(" ⋄ ");
      printRaw(gid2str(id));
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
  bool first = true;
  for (i32 i = 0; i < am; i++) {
    i32 id = desc->expGIDs[i];
    if (id>=0) {
      if (first) first=false;
      else ACHR(U'‿');
      AFMT("%R", gid2str(id));
    }
  }
  AU("⇐}");
  dec(x);
  return s;
}

static void nsDesc_print(B x) { printf("(namespace description)"); }



void ns_init() {
  TIi(t_ns,freeO) = ns_freeO;
  TIi(t_ns,freeF) = ns_freeF;
  TIi(t_ns,visit) = ns_visit; TIi(t_nsDesc,visit) = noop_visit;
  TIi(t_ns,print) = ns_print; TIi(t_nsDesc,print) = nsDesc_print;
}
