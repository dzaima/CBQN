#include "core.h"
#include "ns.h"
#include "vm.h"

void m_nsDesc(Body* body, bool imm, u8 ty, i32 actualVam, B nameList, B varIDs, B exported) { // doesn't consume nameList
  if (!isArr(varIDs) || !isArr(exported)) thrM("Internal error: Bad namespace description information");
  
  usz ia = IA(varIDs);
  if (ia!=IA(exported)) thrM("Internal error: Bad namespace description information");
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


NORETURN NOINLINE static void ns_unk_B(B x) {
  thrF("Field named %B not found", x);
}
NORETURN NOINLINE static void ns_unk_gid(i32 gid) {
  ns_unk_B(gid2str(gid));
}

B ns_getU(B ns, i32 gid) { VTY(ns, t_ns);
  NS* n = c(NS, ns);
  NSDesc* d = n->desc;
  
  i32 ia = d->varAm;
  for (i32 i = 0; i < ia; i++) if (d->expGIDs[i]==gid) return n->sc->vars[i];
  ns_unk_gid(gid);
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
  i32 gid = str2gidQ(name);
  if (gid!=-1) {
    i32 ia = d->varAm;
    for (i32 i = 0; i < ia; i++) if (d->expGIDs[i]==gid) return n->sc->vars[i];
  }
  if (thrEmpty) ns_unk_B(name);
  return bi_N;
}
B ns_getC(B ns, char* name) {
  B field = m_c8vec_0(name);
  B r = ns_getNU(ns, field, false);
  decG(field);
  return r;
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
  ns_unk_B(name);
}




STATIC_GLOBAL i32* emptyi32ptr;
STATIC_GLOBAL B emptyi32obj;
Body* m_nnsDescF(i32 n, char** names) {
  if (emptyi32ptr==NULL) gc_add(emptyi32obj = m_i32arrv(&emptyi32ptr, 0));
  incByG(emptyi32obj, 2);
  
  M_HARR(nl, n)
  for (usz i = 0; i < n; i++) HARR_ADD(nl, i, m_c8vec_0(names[i]));
  
  Comp* comp = mm_alloc(sizeof(Comp), t_comp);
  comp->indices = bi_N;
  comp->src = bi_N;
  comp->fullpath = bi_N;
  comp->objs = c(HArr, emptyHVec());
  comp->blockAm = 0;
  comp->nameList = HARR_FV(nl);
  
  Block* bl = mm_alloc(fsizeof(Block,bodies,Body*,0), t_block);
  bl->ty = 0; bl->imm = true;
  bl->comp = comp;
  bl->bc = bl->map = emptyi32ptr;
  bl->blocks = NULL;
  bl->bodyCount = 0;
  gc_add(tag(bl, OBJ_TAG));
  
  NSDesc* nd = mm_alloc(fsizeof(NSDesc, expGIDs, i32, n<2?2:n), t_nsDesc);
  nd->varAm = n;
  for (usz i = 0; i < n; i++) nd->expGIDs[i] = str2gid(HARR_O(nl).a[i]);
  
  Body* body = m_body(n, 0, 0, 0);
  body->nsDesc = nd;
  body->bc = (u32*) emptyi32ptr;
  body->bl = ptr_inc(bl);
  for (usz i = 0; i < n; i++) {
    body->varData[i] = nd->expGIDs[i];
    body->varData[i+n] = i;
  }
  gc_add(tag(body, OBJ_TAG));
  
  bl->dyBody = bl->invMBody = bl->invXBody = bl->invWBody = body;
  return body;
}

B m_nnsF(Body* desc, i32 n, B* vals) {
  assert(n == desc->varAm);
  i32 varAm = n;
  #if ONLY_NATIVE_COMP
  varAm+= 100;
  #endif
  Scope* sc = m_scope(desc, NULL, varAm, n, vals);
  #if ONLY_NATIVE_COMP
  for (usz i = 0; i < 100; i++) sc->vars[i+n] = bi_N;
  #endif
  return m_ns(sc, ptr_inc(desc->nsDesc));
}

i32 nns_pos(Body* body, B name) {
  B nameList = body->bl->comp->nameList; SGetU(nameList);
  
  i32 ia = body->varAm;
  for (i32 i = 0; i < ia; i++) {
    i32 pos = body->varData[i+ia];
    if (pos>=0 && equal(name, GetU(nameList, pos))) { dec(name); return i; }
  }
  ns_unk_B(name);
}



i32 pos2gid(Body* body, i32 pos) {
  i32 gid = body->varData[pos];
  if (LIKELY(gid!=-1)) return gid;
  
  i32 nlIdx = body->varData[pos+body->varAm];
  if (nlIdx==-1) thrM("Internal error: Cannot use special variable name as namespace key");
  return body->varData[pos] = str2gid(IGetU(body->bl->comp->nameList, nlIdx));
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
static void ns_print(FILE* f, B x) {
  fprintf(f, "{");
  NSDesc* desc = c(NS,x)->desc;
  Scope* sc = c(NS,x)->sc;
  i32 am = desc->varAm;
  bool first = true;
  for (i32 i = 0; i < am; i++) {
    i32 id = desc->expGIDs[i];
    if (id>=0) {
      if (first) first=false;
      else fprintf(f," ⋄ ");
      fprintsB(f, gid2str(id));
      fprintf(f, "⇐");
      fprintI(f, sc->vars[i]);
    }
  }
  fprintf(f, "}");
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
  decG(x);
  return s;
}

static void nsDesc_print(FILE* f, B x) { fprintf(f, "(namespace description)"); }



void ns_init(void) {
  TIi(t_ns,freeO) = ns_freeO;
  TIi(t_ns,freeF) = ns_freeF;
  TIi(t_ns,visit) = ns_visit; TIi(t_nsDesc,visit) = noop_visit;
  TIi(t_ns,print) = ns_print; TIi(t_nsDesc,print) = nsDesc_print;
}
