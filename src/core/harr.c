#include "../core.h"
#include "gstack.h"



B toCells(B x) {
  assert(isArr(x) && RNK(x)>1);
  usz cam = SH(x)[0];
  usz csz = arr_csz(x);
  BSS2A slice = TI(x,slice);
  M_HARR(r, cam)
  usz p = 0;
  if (RNK(x)==2) {
    for (usz i = 0; i < cam; i++) {
      HARR_ADD(r, i, taga(arr_shVec(slice(incG(x), p, csz))));
      p+= csz;
    }
  } else {
    usz cr = RNK(x)-1;
    ShArr* csh = m_shArr(cr);
    usz* xsh = SH(x);
    shcpy(csh->a, xsh+1, cr);
    for (usz i = 0; i < cam; i++) {
      HARR_ADD(r, i, taga(arr_shSetI(slice(incG(x), p, csz), cr, csh)));
      p+= csz;
    }
    ptr_dec(csh);
  }
  decG(x);
  return HARR_FV(r);
}
B toKCells(B x, ur k) {
  assert(isArr(x) && k<=RNK(x) && k>=0);
  ur xr = RNK(x); usz* xsh = SH(x);
  ur cr = xr-k;
  usz cam = shProd(xsh, 0, k);
  usz csz = shProd(xsh, k, xr);
  
  ShArr* csh;
  if (cr>1) {
    csh = m_shArr(cr);
    shcpy(csh->a, xsh+k, cr);
  }
  
  BSS2A slice = TI(x,slice);
  M_HARR(r, cam);
  usz p = 0;
  for (usz i = 0; i < cam; i++) {
    HARR_ADD(r, i, taga(arr_shSetI(slice(incG(x), p, csz), cr, csh)));
    p+= csz;
  }
  if (cr>1) ptr_dec(csh);
  usz* rsh = HARR_FA(r, k);
  if (rsh) shcpy(rsh, xsh, k);
  decG(x);
  return HARR_O(r).b;
}

NOINLINE B m_caB(usz ia, B* a) {
  HArr_p r = m_harrUv(ia);
  for (usz i = 0; i < ia; i++) r.a[i] = a[i];
  return r.b;
}

NOINLINE void harr_pfree(B x, usz am) { // am - item after last written
  assert(TY(x)==t_harr);
  B* p = harr_ptr(x);
  for (usz i = 0; i < am; i++) dec(p[i]);
  if (RNK(x)>1) ptr_dec(shObj(x));
  mm_free(v(x));
}



static Arr* m_hslice(Arr* p, B* ptr, usz ia) {
  HSlice* r = m_arr(sizeof(HSlice), t_hslice, ia);
  r->p = p;
  r->a = ptr;
  return (Arr*)r;
}
static Arr* harr_slice  (B x, usz s, usz ia) { return m_hslice(a(x), c(HArr,x)->a+s, ia); }
static Arr* hslice_slice(B x, usz s, usz ia) { Arr* p = ptr_inc(c(Slice,x)->p); Arr* r = m_hslice(p, c(HSlice,x)->a+s, ia); decG(x); return r; }

static B harr_get   (Arr* x, usz n) { assert(PTY(x)==t_harr  ); return inc(((HArr*  )x)->a[n]); }
static B hslice_get (Arr* x, usz n) { assert(PTY(x)==t_hslice); return inc(((HSlice*)x)->a[n]); }
static B harr_getU  (Arr* x, usz n) { assert(PTY(x)==t_harr  ); return     ((HArr*  )x)->a[n] ; }
static B hslice_getU(Arr* x, usz n) { assert(PTY(x)==t_hslice); return     ((HSlice*)x)->a[n] ; }
DEF_FREE(harr) {
  decSh(x);
  B* p = ((HArr*)x)->a; // don't use harr_ptr so type isn't checked
  usz ia = PIA((Arr*)x);
  for (usz i = 0; i < ia; i++) dec(p[i]);
}
static void harr_visit(Value* x) {
  usz ia = PIA((Arr*)x); B* p = ((HArr*)x)->a;
  for (usz i = 0; i < ia; i++) mm_visit(p[i]);
}
static bool harr_canStore(B x) { return true; }



DEF_FREE(harrP) { assert(PTY(x)==t_harrPartial|PTY(x)==t_freed);
  B* p   = ((HArr*)x)->a;
  usz am = PIA((HArr*)x);
  for (usz i = 0; i < am; i++) dec(p[i]);
}
void harr_abandon_impl(HArr* p) { assert(PTY(p) == t_harrPartial);
  gsPop();
  harrP_freeO((Value*) p);
  mm_free((Value*) p);
}
static void harrP_visit(Value* x) { assert(PTY(x) == t_harrPartial);
  B* p   = ((HArr*)x)->a;
  usz am = PIA((HArr*)x);
  for (usz i = 0; i < am; i++) mm_visit(p[i]);
}
static B harrP_get(Arr* x, usz n) { err("getting item from t_harrPartial"); }
static void harrP_print(FILE* f, B x) {
  B* p = c(HArr,x)->a;
  usz am = *SH(x);
  usz ia = IA(x);
  fprintf(f, "(partial HArr "N64d"/"N64d": ⟨", (u64)am, (u64)ia);
  for (usz i = 0; i < ia; i++) {
    if (i) fprintf(f, ", ");
    if (i>=am) fprintf(f, "?");
    else fprintI(f, p[i]);
  }
  fprintf(f, "⟩)");
}

#if DEBUG
  static void harr_freeT(Value* x) {
    B* p = ((HArr*)x)->a;
    usz ia = PIA((Arr*)x);
    for (usz i = 0; i < ia; i++) assert(!isVal(p[i]));
    tyarr_freeF(x);
  }
#endif

void harr_init() {
  TIi(t_harr,get)   = harr_get;    TIi(t_hslice,get)   = hslice_get;   TIi(t_harrPartial,get)   = harrP_get;
  TIi(t_harr,getU)  = harr_getU;   TIi(t_hslice,getU)  = hslice_getU;  TIi(t_harrPartial,getU)  = harrP_get;
  TIi(t_harr,slice) = harr_slice;  TIi(t_hslice,slice) = hslice_slice;
  TIi(t_harr,freeO) = harr_freeO;  TIi(t_hslice,freeO) =  slice_freeO; TIi(t_harrPartial,freeO) = harrP_freeO;
  TIi(t_harr,freeF) = harr_freeF;  TIi(t_hslice,freeF) =  slice_freeF; TIi(t_harrPartial,freeF) = harrP_freeF;
  #if DEBUG
  TIi(t_harr,freeT) = harr_freeT;
  #else
  TIi(t_harr,freeT) = tyarr_freeF;
  #endif
  TIi(t_harr,visit) = harr_visit;  TIi(t_hslice,visit) =  slice_visit; TIi(t_harrPartial,visit) = harrP_visit;
  TIi(t_harr,print) = farr_print;  TIi(t_hslice,print) = farr_print;   TIi(t_harrPartial,print) = harrP_print;
  TIi(t_harr,isArr) = true;        TIi(t_hslice,isArr) = true;
  TIi(t_harr,canStore) = harr_canStore;
  bi_emptyHVec = m_harrUv(0).b; gc_add(bi_emptyHVec);
}
