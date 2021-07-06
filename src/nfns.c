#include "core.h"
#include "h.h"
#include "nfns.h"
#include "utils/mut.h"

struct NFnDesc {
  struct Value;
  u32 id;
  B name;
  BB2B c1;
  BBB2B c2;
};
static u32 nfn_curr;
static B nfn_list;

NFnDesc* registerNFn(B name, BB2B c1, BBB2B c2) {
  u32 id = nfn_curr++;
  NFnDesc* r = mm_alloc(sizeof(NFnDesc), t_nfnDesc);
  r->id = id;
  r->c1 = c1;
  r->c2 = c2;
  r->name = name;
  nfn_list = vec_add(nfn_list, tag(r, OBJ_TAG));
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
  return inc(c(NFnDesc,TI(nfn_list,getU)(nfn_list,c(NFn,x)->id))->name);
}

void nfn_free(Value* x) { dec(((NFn*)x)->obj); }
void nfn_visit(Value* x) { mm_visit(((NFn*)x)->obj); }
void nfn_print(B x) { printRaw(c(NFnDesc,TI(nfn_list,getU)(nfn_list,c(NFn,x)->id))->name); }
void nfnDesc_free(Value* x) { err("nfnDesc shouldn't be freed!"); }
void nfnDesc_visit(Value* x) { mm_visit(((NFnDesc*)x)->name); }
void nfnDesc_print(B x) { printf("(native function description)"); }

void nfn_gcFn() {
  mm_visit(nfn_list);
}
void nfn_init() {
  nfn_list = emptyHVec();
  TIi(t_nfn,free)  = nfn_free;  TIi(t_nfnDesc,free)  = nfnDesc_free;
  TIi(t_nfn,visit) = nfn_visit; TIi(t_nfnDesc,visit) = nfnDesc_visit;
  TIi(t_nfn,print) = nfn_print; TIi(t_nfnDesc,print) = nfnDesc_print;
  gc_addFn(nfn_gcFn);
}
