#include "core.h"
#include "h.h"
#include "nfns.h"

STATIC_GLOBAL u32 nfn_curr;
STATIC_GLOBAL B nfn_list;

NFnDesc* registerNFn(B name, FC1 c1, FC2 c2) {
  u32 id = nfn_curr++;
  NFnDesc* r = mm_alloc(sizeof(NFnDesc), t_nfnDesc);
  r->id = id;
  r->c1 = c1;
  r->c2 = c2;
  r->name = name;
  nfn_list = vec_addN(nfn_list, tag(r,OBJ_TAG));
  return r;
}
B m_nfn(NFnDesc* desc, B obj) {
  NFn* r = mm_alloc(sizeof(NFn), t_nfn);
  r->id = desc->id;
  r->c1 = desc->c1;
  r->c2 = desc->c2;
  r->obj = obj;
  return tag(r,FUN_TAG);
}
B nfn_name(B x) { VTY(x, t_nfn);
  return incG(c(NFnDesc,IGetU(nfn_list,c(NFn,x)->id))->name);
}

DEF_FREE(nfn) { dec(((NFn*)x)->obj); }
void nfn_visit(Value* x) { mm_visit(((NFn*)x)->obj); }
void nfn_print(FILE* f, B x) { fprintsB(f, c(NFnDesc,IGetU(nfn_list,c(NFn,x)->id))->name); }
DEF_FREE(nfnDesc) { fatal("nfnDesc shouldn't be freed!"); }
void nfnDesc_visit(Value* x) { mm_visit(((NFnDesc*)x)->name); }
void nfnDesc_print(FILE* f, B x) { fprintf(f, "(native function description)"); }

B block_decompose(B x);
void nfn_init(void) {
  nfn_list = emptyHVec();
  TIi(t_nfn,freeO) = nfn_freeO; TIi(t_nfnDesc,freeO) = nfnDesc_freeO;
  TIi(t_nfn,freeF) = nfn_freeF; TIi(t_nfnDesc,freeF) = nfnDesc_freeF;
  TIi(t_nfn,visit) = nfn_visit; TIi(t_nfnDesc,visit) = nfnDesc_visit;
  TIi(t_nfn,print) = nfn_print; TIi(t_nfnDesc,print) = nfnDesc_print;
  TIi(t_nfn,decompose) = block_decompose;
  TIi(t_nfn,byRef) = true;
  gc_add_ref(&nfn_list);
}
