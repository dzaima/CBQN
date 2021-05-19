#include "h.h"

Block* load_compObj(B x, B src) { // consumes
  BS2B xget = TI(x).get;
  usz xia = a(x)->ia;
  if (xia!=5 & xia!=3) thrM("load_compObj: bad item count");
  Block* r = xia==5? compile(xget(x,0),xget(x,1),xget(x,2),xget(x,3),xget(x,4), src)
                   : compile(xget(x,0),xget(x,1),xget(x,2),bi_N,     bi_N,      src);
  dec(x);
  return r;
}
#ifdef RT_SRC
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

Block* bqn_comp(B str, B path, B args) { // consumes all
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

static inline void load_init() {
  comp_currPath = bi_N;
  comp_currArgs = bi_N;
  gc_addFn(load_gcFn);
  B fruntime[] = {
    /* +-×÷⋆√⌊⌈|¬  */ bi_add  , bi_sub   , bi_mul  , bi_div    , bi_pow   , bi_N     , bi_floor, bi_ceil, bi_stile , bi_not,
    /* ∧∨<>≠=≤≥≡≢  */ bi_and  , bi_or    , bi_lt   , bi_gt     , bi_ne    , bi_eq    , bi_le   , bi_ge  , bi_feq   , bi_fne,
    /* ⊣⊢⥊∾≍↑↓↕«» */ bi_ltack, bi_rtack , bi_shape, bi_join   , bi_couple, bi_take  , bi_drop , bi_ud  , bi_shifta, bi_shiftb,
    /* ⌽⍉/⍋⍒⊏⊑⊐⊒∊  */ bi_N    , bi_N     , bi_slash, bi_gradeUp, bi_N     , bi_select, bi_pick , bi_N   , bi_N     , bi_N,
    /* ⍷⊔!˙˜˘¨⌜⁼´  */ bi_N    , bi_group , bi_asrt , bi_const  , bi_swap  , bi_N     , bi_each , bi_tbl , bi_N     , bi_fold,
    /* ˝`∘○⊸⟜⌾⊘◶⎉  */ bi_N    , bi_scan  , bi_atop , bi_over   , bi_before, bi_after , bi_under, bi_val , bi_cond  , bi_N,
    /* ⚇⍟⎊         */ bi_N    , bi_repeat, bi_catch
  };
  bool rtComplete[] = {
    /* +-×÷⋆√⌊⌈|¬  */ 1,1,1,1,1,0,1,1,1,1,
    /* ∧∨<>≠=≤≥≡≢  */ 1,1,1,1,1,1,1,1,1,1,
    /* ⊣⊢⥊∾≍↑↓↕«» */ 1,1,0,1,1,1,1,1,1,1,
    /* ⌽⍉/⍋⍒⊏⊑⊐⊒∊  */ 0,0,1,1,0,1,1,0,0,0,
    /* ⍷⊔!˙˜˘¨⌜⁼´  */ 0,1,1,1,1,0,1,1,0,1,
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
    #include "runtime0"
  );
  B r0r = m_funBlock(runtime0_b, 0); ptr_dec(runtime0_b);
  B* runtime_0 = toHArr(r0r)->a;
  #endif
  
  Block* runtime_b = load_compImport(
    #include "runtime1"
  );
  
  #ifdef ALL_R0
  dec(r0r);
  #endif
  
  B rtRes = m_funBlock(runtime_b, 0); ptr_dec(runtime_b);
  B rtObjRaw = TI(rtRes).get(rtRes,0);
  B rtFinish = TI(rtRes).get(rtRes,1);
  dec(rtRes);
  
  runtimeLen = c(Arr,rtObjRaw)->ia;
  HArr_p runtimeH = m_harrUc(rtObjRaw);
  BS2B rtObjGet = TI(rtObjRaw).get;
  
  rt_sortAsc = rtObjGet(rtObjRaw, 10); gc_add(rt_sortAsc);
  rt_sortDsc = rtObjGet(rtObjRaw, 11); gc_add(rt_sortDsc);
  rt_merge   = rtObjGet(rtObjRaw, 13); gc_add(rt_merge);
  rt_undo    = rtObjGet(rtObjRaw, 48); gc_add(rt_undo);
  rt_select  = rtObjGet(rtObjRaw, 35); gc_add(rt_select);
  rt_slash   = rtObjGet(rtObjRaw, 32); gc_add(rt_slash);
  rt_join    = rtObjGet(rtObjRaw, 23); gc_add(rt_join);
  rt_gradeUp = rtObjGet(rtObjRaw, 33); gc_add(rt_gradeUp);
  rt_ud      = rtObjGet(rtObjRaw, 27); gc_add(rt_ud);
  rt_pick    = rtObjGet(rtObjRaw, 36); gc_add(rt_pick);
  rt_take    = rtObjGet(rtObjRaw, 25); gc_add(rt_take);
  rt_drop    = rtObjGet(rtObjRaw, 26); gc_add(rt_drop);
  rt_group   = rtObjGet(rtObjRaw, 41); gc_add(rt_group);
  rt_under   = rtObjGet(rtObjRaw, 56); gc_add(rt_under);
  
  for (usz i = 0; i < runtimeLen; i++) {
    #ifdef ALL_R1
      B r = rtObjGet(rtObjRaw, i);
    #else
      B r = rtComplete[i]? inc(fruntime[i]) : rtObjGet(rtObjRaw, i);
    #endif
    if (isNothing(r)) { printf("· in runtime!\n"); exit(1); }
    if (isVal(r)) v(r)->flags|= i+1;
    #ifdef RT_PERF
      r = rtPerf_wrap(r);
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
      #include "interp"
    );
    B interp = m_funBlock(c, 0); ptr_dec(c);
    print(interp);
    printf("\n");
    dec(interp);
    exit(0);
  #else // use compiler
    Block* comp_b = load_compImport(
      #include "compiler"
    );
    load_comp = m_funBlock(comp_b, 0); ptr_dec(comp_b);
    gc_add(load_comp);
    
    
    #ifdef FORMATTER
    Block* fmt_b = load_compImport(
      #include "formatter"
    );
    B fmtM = m_funBlock(fmt_b, 0); ptr_dec(fmt_b);
    B fmtR = c1(fmtM, m_caB(4, (B[]){inc(bi_type), inc(bi_decp), inc(bi_fmtF), inc(bi_repr)}));
    BS2B fget = TI(fmtR).get;
    load_fmt  = fget(fmtR, 0); gc_add(load_fmt);
    load_repr = fget(fmtR, 1); gc_add(load_repr);
    dec(fmtR);
    #endif
    gc_enable();
  #endif // NO_COMP
}

B bqn_execFile(B path, B args) { // consumes both
  return bqn_exec(file_chars(inc(path)), path, args);
}
