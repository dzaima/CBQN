#include "core.h"
#include "vm.h"
#include "utils/file.h"

#define FA(N,X) B bi_##N; B N##_c1(B t, B x); B N##_c2(B t, B w, B x);
#define FM(N,X) B bi_##N; B N##_c1(B t, B x);
#define FD(N,X) B bi_##N; B N##_c2(B t, B w, B x);
FOR_PFN(FA,FM,FD)
FOR_PM1(FA,FM,FD)
FOR_PM2(FA,FM,FD)
#undef FA
#undef FM
#undef FD

TypeInfo ti[t_COUNT];

B r1Objs[rtLen];
B rtWrap_wrap(B x); // consumes


_Thread_local B comp_currPath;
_Thread_local B comp_currArgs;

B rt_sortDsc, rt_merge, rt_undo, rt_select, rt_slash, rt_join, rt_ud, rt_pick,rt_take,
  rt_drop, rt_group, rt_under, rt_reverse, rt_indexOf, rt_count, rt_memberOf, rt_find, rt_cell;
Block* load_compObj(B x, B src) { // consumes
  BS2B xget = TI(x).get;
  usz xia = a(x)->ia;
  if (xia!=5 & xia!=3) thrM("load_compObj: bad item count");
  Block* r = xia==5? compile(xget(x,0),xget(x,1),xget(x,2),xget(x,3),xget(x,4), src)
                   : compile(xget(x,0),xget(x,1),xget(x,2),bi_N,     bi_N,      src);
  dec(x);
  return r;
}
#include "gen/src"
#if RT_SRC
Block* load_compImport(B bc, B objs, B blocks, B inds, B src) { // consumes all
  return compile(bc, objs, blocks, inds, bi_N, src);
}
#else
Block* load_compImport(B bc, B objs, B blocks) { // consumes all
  return compile(bc, objs, blocks, bi_N, bi_N, bi_N);
}
#endif

B load_comp;
B load_compArg;

#ifdef FORMATTER
B load_fmt, load_repr;
B bqn_fmt(B x) { // consumes
  return c1(load_fmt, x);
}
B bqn_repr(B x) { // consumes
  return c1(load_repr, x);
}
#endif

void load_gcFn() {
  mm_visit(comp_currPath);
  mm_visit(comp_currArgs);
}

NOINLINE Block* bqn_comp(B str, B path, B args) { // consumes all
  comp_currPath = path;
  comp_currArgs = args;
  Block* r = load_compObj(c2(load_comp, inc(load_compArg), inc(str)), str);
  dec(path); dec(args);
  comp_currArgs = comp_currPath = bi_N;
  return r;
}

B bqn_exec(B str, B path, B args) { // consumes all
  Block* block = bqn_comp(str, path, args);
  B res = m_funBlock(block, 0);
  ptr_dec(block);
  return res;
}
void bqn_setComp(B comp) { // consumes; doesn't unload old comp, but whatever
  load_comp = comp;
  gc_add(load_comp);
}

static inline void load_init() { // very last init function
  comp_currPath = bi_N;
  comp_currArgs = bi_N;
  gc_addFn(load_gcFn);
  B fruntime[] = {
    /* +-×÷⋆√⌊⌈|¬  */ bi_add    , bi_sub   , bi_mul  , bi_div    , bi_pow      , bi_N     , bi_floor, bi_ceil   , bi_stile , bi_not,
    /* ∧∨<>≠=≤≥≡≢  */ bi_and    , bi_or    , bi_lt   , bi_gt     , bi_ne       , bi_eq    , bi_le   , bi_ge     , bi_feq   , bi_fne,
    /* ⊣⊢⥊∾≍↑↓↕«» */ bi_ltack  , bi_rtack , bi_shape, bi_join   , bi_couple   , bi_take  , bi_drop , bi_ud     , bi_shifta, bi_shiftb,
    /* ⌽⍉/⍋⍒⊏⊑⊐⊒∊  */ bi_reverse, bi_N     , bi_slash, bi_gradeUp, bi_gradeDown, bi_select, bi_pick , bi_indexOf, bi_count , bi_memberOf,
    /* ⍷⊔!˙˜˘¨⌜⁼´  */ bi_find   , bi_group , bi_asrt , bi_const  , bi_swap     , bi_cell  , bi_each , bi_tbl    , bi_N     , bi_fold,
    /* ˝`∘○⊸⟜⌾⊘◶⎉  */ bi_N      , bi_scan  , bi_atop , bi_over   , bi_before   , bi_after , bi_under, bi_val    , bi_cond  , bi_N,
    /* ⚇⍟⎊         */ bi_N      , bi_repeat, bi_catch
  };
  bool rtComplete[] = {
    /* +-×÷⋆√⌊⌈|¬  */ 1,1,1,1,1,0,1,1,1,1,
    /* ∧∨<>≠=≤≥≡≢  */ 1,1,1,1,1,1,1,1,1,1,
    /* ⊣⊢⥊∾≍↑↓↕«» */ 1,1,0,1,1,1,1,1,1,1,
    /* ⌽⍉/⍋⍒⊏⊑⊐⊒∊  */ 1,0,1,1,1,1,1,1,1,1,
    /* ⍷⊔!˙˜˘¨⌜⁼´  */ 1,1,1,1,1,1,1,1,0,1,
    /* ˝`∘○⊸⟜⌾⊘◶⎉  */ 0,1,1,1,1,1,1,1,0,0,
    /* ⚇⍟⎊         */ 0,1,1
  };
  assert(sizeof(fruntime)/sizeof(B) == rtLen);
  for (i32 i = 0; i < rtLen; i++) inc(fruntime[i]);
  B frtObj = m_caB(rtLen, fruntime);
  
  B provide[] = {bi_type,bi_fill,bi_log,bi_grLen,bi_grOrd,bi_asrt,bi_add,bi_sub,bi_mul,bi_div,bi_pow,bi_floor,bi_eq,bi_le,bi_fne,bi_shape,bi_pick,bi_ud,bi_tbl,bi_scan,bi_fillBy,bi_val,bi_catch};
  
  #ifndef ALL_R0
  B runtime_0[] = {bi_floor,bi_ceil,bi_stile,bi_lt,bi_gt,bi_ne,bi_ge,bi_rtack,bi_ltack,bi_join,bi_take,bi_drop,bi_select,bi_const,bi_swap,bi_each,bi_fold,bi_atop,bi_over,bi_before,bi_after,bi_cond,bi_repeat};
  #else
  Block* runtime0_b = load_compImport(
    #include "gen/runtime0"
  );
  B r0r = m_funBlock(runtime0_b, 0); ptr_dec(runtime0_b);
  B* runtime_0 = toHArr(r0r)->a;
  #endif
  
  Block* runtime_b = load_compImport(
    #include "gen/runtime1"
  );
  
  #ifdef ALL_R0
  dec(r0r);
  #endif
  
  B rtRes = m_funBlock(runtime_b, 0); ptr_dec(runtime_b);
  B rtObjRaw = TI(rtRes).get(rtRes,0);
  B rtFinish = TI(rtRes).get(rtRes,1);
  dec(rtRes);
  
  if (c(Arr,rtObjRaw)->ia != rtLen) err("incorrectly defined rtLen!");
  HArr_p runtimeH = m_harrUc(rtObjRaw);
  BS2B rtObjGet = TI(rtObjRaw).get;
  
  rt_sortDsc = rtObjGet(rtObjRaw, 11); gc_add(rt_sortDsc);
  rt_merge   = rtObjGet(rtObjRaw, 13); gc_add(rt_merge);
  rt_undo    = rtObjGet(rtObjRaw, 48); gc_add(rt_undo);
  rt_select  = rtObjGet(rtObjRaw, 35); gc_add(rt_select);
  rt_slash   = rtObjGet(rtObjRaw, 32); gc_add(rt_slash);
  rt_join    = rtObjGet(rtObjRaw, 23); gc_add(rt_join);
  rt_ud      = rtObjGet(rtObjRaw, 27); gc_add(rt_ud);
  rt_pick    = rtObjGet(rtObjRaw, 36); gc_add(rt_pick);
  rt_take    = rtObjGet(rtObjRaw, 25); gc_add(rt_take);
  rt_drop    = rtObjGet(rtObjRaw, 26); gc_add(rt_drop);
  rt_group   = rtObjGet(rtObjRaw, 41); gc_add(rt_group);
  rt_under   = rtObjGet(rtObjRaw, 56); gc_add(rt_under);
  rt_reverse = rtObjGet(rtObjRaw, 30); gc_add(rt_reverse);
  rt_indexOf = rtObjGet(rtObjRaw, 37); gc_add(rt_indexOf);
  rt_count   = rtObjGet(rtObjRaw, 38); gc_add(rt_count);
  rt_memberOf= rtObjGet(rtObjRaw, 39); gc_add(rt_memberOf);
  rt_find    = rtObjGet(rtObjRaw, 40); gc_add(rt_find);
  rt_cell    = rtObjGet(rtObjRaw, 45); gc_add(rt_cell);
  
  for (usz i = 0; i < rtLen; i++) {
    #ifdef RT_WRAP
    r1Objs[i] = rtObjGet(rtObjRaw, i); gc_add(r1Objs[i]);
    #endif
    #ifdef ALL_R1
      B r = rtObjGet(rtObjRaw, i);
    #else
      B r = rtComplete[i]? inc(fruntime[i]) : rtObjGet(rtObjRaw, i);
    #endif
    if (isNothing(r)) { printf("· in runtime!\n"); exit(1); }
    if (isVal(r)) v(r)->flags|= i+1;
    #ifdef RT_WRAP
      r = rtWrap_wrap(r);
      if (isVal(r)) v(r)->flags|= i+1;
    #endif
    runtimeH.a[i] = r;
  }
  dec(rtObjRaw);
  B* runtime = runtimeH.a;
  B rtObj = runtimeH.b;
  dec(c1(rtFinish, m_v2(inc(bi_decp), inc(bi_primInd)))); dec(rtFinish);
  
  
  load_compArg = m_v2(FAKE_RUNTIME? frtObj : rtObj, inc(bi_sys)); gc_add(FAKE_RUNTIME? rtObj : frtObj);
  gc_add(load_compArg);
  
  
  #ifdef NO_COMP
    Block* c = load_compObj(
      #include "gen/interp"
      , bi_N
    );
    B interp = m_funBlock(c, 0); ptr_dec(c);
    print(interp);
    printf("\n");
    dec(interp);
    exit(0);
  #else // use compiler
    Block* comp_b = load_compImport(
      #include "gen/compiler"
    );
    load_comp = m_funBlock(comp_b, 0); ptr_dec(comp_b);
    gc_add(load_comp);
    
    
    #ifdef FORMATTER
    Block* fmt_b = load_compImport(
      #include "gen/formatter"
    );
    B fmtM = m_funBlock(fmt_b, 0); ptr_dec(fmt_b);
    B fmtR = c1(fmtM, m_caB(4, (B[]){inc(bi_type), inc(bi_decp), inc(bi_fmtF), inc(bi_repr)}));
    BS2B fget = TI(fmtR).get;
    load_fmt  = fget(fmtR, 0); gc_add(load_fmt);
    load_repr = fget(fmtR, 1); gc_add(load_repr);
    dec(fmtR);
    dec(fmtM);
    #endif
    gc_enable();
  #endif // NO_COMP
}

B bqn_execFile(B path, B args) { // consumes both
  return bqn_exec(file_chars(inc(path)), path, args);
}




static void freed_visit(Value* x) {
  #ifndef CATCH_ERRORS
  err("visiting t_freed\n");
  #endif
}
static void empty_free(Value* x) { err("FREEING EMPTY\n"); }
static void builtin_free(Value* x) { err("FREEING BUILTIN\n"); }
static void def_free(Value* x) { }
static void def_visit(Value* x) { printf("(no visit for %d=%s)\n", x->type, format_type(x->type)); }
static void def_print(B x) { printf("(%d=%s)", v(x)->type, format_type(v(x)->type)); }
static bool def_canStore(B x) { return false; }
static B def_identity(B f) { return bi_N; }
static B def_get(B x, usz n) { return inc(x); }
static B def_m1_d(B m, B f     ) { thrM("cannot derive this"); }
static B def_m2_d(B m, B f, B g) { thrM("cannot derive this"); }
static B def_slice(B x, usz s) { thrM("cannot slice non-array!"); }

static inline void base_init() { // very first init function
  mm_heapMax = HEAP_MAX;
  for (i32 i = 0; i < t_COUNT; i++) {
    ti[i].free  = def_free;
    ti[i].visit = def_visit;
    ti[i].get   = def_get;
    ti[i].getU  = def_getU;
    ti[i].print = def_print;
    ti[i].m1_d  = def_m1_d;
    ti[i].m2_d  = def_m2_d;
    ti[i].isArr = false;
    ti[i].arrD1 = false;
    ti[i].elType    = el_B;
    ti[i].identity  = def_identity;
    ti[i].decompose = def_decompose;
    ti[i].slice     = def_slice;
    ti[i].canStore  = def_canStore;
    ti[i].fn_uc1 = def_fn_uc1;
    ti[i].fn_ucw = def_fn_ucw;
    ti[i].m1_uc1 = def_m1_uc1;
    ti[i].m1_ucw = def_m1_ucw;
    ti[i].m2_uc1 = def_m2_uc1;
    ti[i].m2_ucw = def_m2_ucw;
  }
  ti[t_empty].free = empty_free;
  ti[t_freed].free = def_free;
  ti[t_freed].visit = freed_visit;
  ti[t_shape].visit = noop_visit;
  ti[t_funBI].visit = ti[t_md1BI].visit = ti[t_md2BI].visit = noop_visit;
  ti[t_funBI].free  = ti[t_md1BI].free  = ti[t_md2BI].free  = builtin_free;
  bi_N = tag(0, TAG_TAG);
  bi_noVar   = tag(1, TAG_TAG);
  bi_badHdr  = tag(2, TAG_TAG);
  bi_optOut  = tag(3, TAG_TAG);
  bi_noFill  = tag(5, TAG_TAG);
  assert((MD1_TAG>>1) == (MD2_TAG>>1)); // just to be sure it isn't changed incorrectly, `isMd` depends on this
  
  #define FA(N,X) { B t=bi_##N = mm_alloc(sizeof(BFn), t_funBI, ftag(FUN_TAG)); BFn*f=c(BFn,t); f->c2=N##_c2    ; f->c1=N##_c1    ; f->extra=pf_##N; f->ident=bi_N; f->uc1=def_fn_uc1; f->ucw=def_fn_ucw; gc_add(t); }
  #define FM(N,X) { B t=bi_##N = mm_alloc(sizeof(BFn), t_funBI, ftag(FUN_TAG)); BFn*f=c(BFn,t); f->c2=c2_invalid; f->c1=N##_c1    ; f->extra=pf_##N; f->ident=bi_N; f->uc1=def_fn_uc1; f->ucw=def_fn_ucw; gc_add(t); }
  #define FD(N,X) { B t=bi_##N = mm_alloc(sizeof(BFn), t_funBI, ftag(FUN_TAG)); BFn*f=c(BFn,t); f->c2=N##_c2    ; f->c1=c1_invalid; f->extra=pf_##N; f->ident=bi_N; f->uc1=def_fn_uc1; f->ucw=def_fn_ucw; gc_add(t); }
  FOR_PFN(FA,FM,FD)
  #undef FA
  #undef FM
  #undef FD
  
  #define FA(N,X) { B t = bi_##N = mm_alloc(sizeof(Md1), t_md1BI, ftag(MD1_TAG)); c(Md1,t)->c2 = N##_c2    ; c(Md1,t)->c1 = N##_c1    ; c(Md1,t)->extra=pm1_##N; gc_add(t); }
  #define FM(N,X) { B t = bi_##N = mm_alloc(sizeof(Md1), t_md1BI, ftag(MD1_TAG)); c(Md1,t)->c2 = c2_invalid; c(Md1,t)->c1 = N##_c1    ; c(Md1,t)->extra=pm1_##N; gc_add(t); }
  #define FD(N,X) { B t = bi_##N = mm_alloc(sizeof(Md1), t_md1BI, ftag(MD1_TAG)); c(Md1,t)->c2 = N##_c2    ; c(Md1,t)->c1 = c1_invalid; c(Md1,t)->extra=pm1_##N; gc_add(t); }
  FOR_PM1(FA,FM,FD)
  #undef FA
  #undef FM
  #undef FD
  
  #define FA(N,X) { B t=bi_##N=mm_alloc(sizeof(BMd2), t_md2BI, ftag(MD2_TAG)); BMd2*m=c(BMd2,t); m->c2 = N##_c2    ; m->c1 = N##_c1;     m->extra=pm2_##N; m->uc1=def_m2_uc1; m->ucw=def_m2_ucw; gc_add(t); }
  #define FM(N,X) { B t=bi_##N=mm_alloc(sizeof(BMd2), t_md2BI, ftag(MD2_TAG)); BMd2*m=c(BMd2,t); m->c2 = N##_c2    ; m->c1 = c1_invalid; m->extra=pm2_##N; m->uc1=def_m2_uc1; m->ucw=def_m2_ucw; gc_add(t); }
  #define FD(N,X) { B t=bi_##N=mm_alloc(sizeof(BMd2), t_md2BI, ftag(MD2_TAG)); BMd2*m=c(BMd2,t); m->c2 = c2_invalid; m->c1 = N##_c1;     m->extra=pm2_##N; m->uc1=def_m2_uc1; m->ucw=def_m2_ucw; gc_add(t); }
  FOR_PM2(FA,FM,FD)
  #undef FA
  #undef FM
  #undef FD
}

#define FOR_INIT(F) F(base) F(harr) F(fillarr) F(i32arr) F(c32arr) F(f64arr) F(hash) F(sfns) F(fns) F(arith) F(md1) F(md2) F(derv) F(comp) F(rtWrap) F(ns) F(load)
#define F(X) void X##_init();
FOR_INIT(F)
#undef F
void cbqn_init() {
  #define F(X) X##_init();
   FOR_INIT(F)
  #undef F
}
#undef FOR_INIT
