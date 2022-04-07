#include "core.h"
#include "utils/mut.h"
#include "utils/file.h"
#include "vm.h"
#include "ns.h"
#include "builtins.h"

#define FOR_INIT(F) F(base) F(harr) F(mutF) F(fillarr) F(tyarr) F(hash) F(sfns) F(fns) F(arith) F(md1) F(md2) F(derv) F(comp) F(rtWrap) F(ns) F(nfn) F(sysfn) F(inverse) F(load) F(sysfnPost) F(dervPost)
#define F(X) void X##_init(void);
FOR_INIT(F)
#undef F

u64 mm_heapMax = HEAP_MAX;
u64 mm_heapAlloc;

// compiler result:
// [
//   [...bytecode],
//   [...objects],
//   [ // block data
//     [
//       type, // 0: function; 1: 1-modifier; 2: 2-modifier
//       immediateness, // 0: non-immediate; 1: immediate
//       ambivalentIndex OR [monadicIndices, dyadicIndices], // indexes into body data array
//     ]*
//   ],
//   [ // body data
//     [
//       bytecodeOffset,
//       variableCount, // number of variable slots needed
//       ( // optional extra info for namespace stuff
//         [...variableIDs] // a number for each variable slot; indexes into nameList
//         [...exportMask] // a unique number for each variable
//       )?
//     ]*
//   ],
//   [[...startIndices], [...endIndices]],? // optional, for each bytecode; inclusive
//   [%, %, [[...nameList], %], %]? // optional; % marks things i haven't bothered to understand
// ]

#define FA(N,X) B bi_##N; B N##_c1(B t, B x); B N##_c2(B t, B w, B x);
#define FM(N,X) B bi_##N; B N##_c1(B t, B x);
#define FD(N,X) B bi_##N; B N##_c2(B t, B w, B x);
FOR_PFN(FA,FM,FD)
#undef FA
#undef FM
#undef FD
#define FA(N,X) B bi_##N; B N##_c1(Md1D* d, B x); B N##_c2(Md1D* d, B w, B x);
#define FM(N,X) B bi_##N; B N##_c1(Md1D* d, B x);
#define FD(N,X) B bi_##N; B N##_c2(Md1D* d, B w, B x);
FOR_PM1(FA,FM,FD)
#undef FA
#undef FM
#undef FD
#define FA(N,X) B bi_##N; B N##_c1(Md2D*, B x); B N##_c2(Md2D*, B w, B x);
#define FM(N,X) B bi_##N; B N##_c1(Md2D*, B x);
#define FD(N,X) B bi_##N; B N##_c2(Md2D*, B w, B x);
FOR_PM2(FA,FM,FD)
#undef FA
#undef FM
#undef FD

#define F(N) u64 N;
CTR_FOR(F)
#undef F
char* pfn_repr(u8 u) {
  switch(u) { default: return "(unknown function)";
    #define F(N,X) case pf_##N: return X;
    FOR_PFN(F,F,F)
    #undef F
  }
}
char* pm1_repr(u8 u) {
  switch(u) { default: return"(unknown 1-modifier)";
    #define F(N,X) case pm1_##N: return X;
    FOR_PM1(F,F,F)
    #undef F
  }
}
char* pm2_repr(u8 u) {
  switch(u) { default: return"(unknown 2-modifier)";
    #define F(N,X) case pm2_##N: return X;
    FOR_PM2(F,F,F)
    #undef F
  }
}



#define F(TY,N) TY ti_##N[t_COUNT];
  FOR_TI(F)
#undef F

B r1Objs[rtLen];
B rtWrap_wrap(B x, bool nnbi); // consumes
void rtWrap_print(void);


// comp_currEnvPos/comp_currPath/comp_currArgs/comp_currSrc are only valid while evaluating through bqn_comp*; comp_currRe is valid at all times
i64 comp_currEnvPos;
B comp_currPath;
B comp_currArgs;
B comp_currSrc;
B comp_currRe;

B rt_undo, rt_select, rt_slash, rt_join, rt_ud, rt_pick, rt_take, rt_drop, rt_insert, rt_depth,
  rt_group, rt_under, rt_reverse, rt_indexOf, rt_count, rt_memberOf, rt_find, rt_cell, rt_rank, rt_transp;
Block* load_compObj(B x, B src, B path, Scope* sc) { // consumes x,src
  SGet(x)
  usz xia = a(x)->ia;
  if (xia!=6 & xia!=4) thrM("load_compObj: bad item count");
  Block* r = xia==6? compile(Get(x,0),Get(x,1),Get(x,2),Get(x,3),Get(x,4),Get(x,5), src, inc(path), sc)
                   : compile(Get(x,0),Get(x,1),Get(x,2),Get(x,3),bi_N,    bi_N,     src, inc(path), sc);
  decG(x);
  return r;
}
#include "gen/src"
#if RT_SRC
Block* load_compImport(B bc, B objs, B blocks, B bodies, B inds, B src) { // consumes all
  return compile(bc, objs, blocks, bodies, inds, bi_N, src, m_str8l("(precompiled)"), NULL);
}
#else
Block* load_compImport(B bc, B objs, B blocks, B bodies) { // consumes all
  return compile(bc, objs, blocks, bodies, bi_N, bi_N, bi_N, bi_N, NULL);
}
#endif

B load_comp;
B load_compgen;
B load_rtObj;
B load_compArg;
B load_glyphs;

#if FORMATTER
B load_fmt, load_repr;
B bqn_fmt(B x) { // consumes
  return c1(load_fmt, x);
}
B bqn_repr(B x) { // consumes
  return c1(load_repr, x);
}
#else
B bqn_fmt(B x) {
  return x;
}
B bqn_repr(B x) {
  return x;
}
#endif

void load_gcFn() {
  mm_visit(comp_currPath);
  mm_visit(comp_currArgs);
  mm_visit(comp_currSrc);
  mm_visit(comp_currRe);
  mm_visit(rt_invFnReg);
  mm_visit(rt_invFnSwap);
}
static NOINLINE Block* bqn_compc(B str, B path, B args, B comp, B compArg) { // consumes str,path,args
  B   prevPath   = comp_currPath  ; comp_currPath = path;
  B   prevArgs   = comp_currArgs  ; comp_currArgs = args;
  B   prevSrc    = comp_currSrc   ; comp_currSrc  = str;
  i64 prevEnvPos = comp_currEnvPos; comp_currEnvPos = envCurr-envStart;
  Block* r = load_compObj(c2(comp, incG(compArg), inc(str)), str, path, NULL);
  dec(path); dec(args);
  comp_currPath   = prevPath;
  comp_currArgs   = prevArgs;
  comp_currSrc    = prevSrc;
  comp_currEnvPos = prevEnvPos;
  return r;
}
Block* bqn_comp(B str, B path, B args) { // consumes all
  return bqn_compc(str, path, args, load_comp, load_compArg);
}
Block* bqn_compScc(B str, B path, B args, Scope* sc, B comp, B rt, bool repl) { // consumes str,path,args
  B   prevPath   = comp_currPath  ; comp_currPath = path;
  B   prevArgs   = comp_currArgs  ; comp_currArgs = args;
  B   prevSrc    = comp_currSrc   ; comp_currSrc  = str;
  i64 prevEnvPos = comp_currEnvPos; comp_currEnvPos = envCurr-envStart;
  B vName = emptyHVec();
  B vDepth = emptyIVec();
  if (repl && (!sc || sc->psc)) thrM("VM compiler: REPL mode must be used at top level scope");
  i32 depth = repl? -1 : 0;
  Scope* csc = sc;
  while (csc) {
    B vars = listVars(csc);
    usz am = a(vars)->ia;
    vName = vec_join(vName, vars);
    for (usz i = 0; i < am; i++) vDepth = vec_add(vDepth, m_i32(depth));
    csc = csc->psc;
    depth++;
  }
  Block* r = load_compObj(c2(comp, m_hVec4(incG(rt), incG(bi_sys), vName, vDepth), inc(str)), str, path, sc);
  dec(path); dec(args);
  comp_currPath   = prevPath;
  comp_currArgs   = prevArgs;
  comp_currSrc    = prevSrc;
  comp_currEnvPos = prevEnvPos;
  return r;
}
NOINLINE Block* bqn_compSc(B str, B path, B args, Scope* sc, bool repl) { // consumes str,path,args
  return bqn_compScc(str, path, args, sc, load_comp, load_rtObj, repl);
}

B bqn_exec(B str, B path, B args) { // consumes all
  Block* block = bqn_comp(str, path, args);
  B res = evalFunBlock(block, 0);
  ptr_dec(block);
  return res;
}

void init_comp(B* set, B prim) {
  if (q_N(prim)) {
    set[0] = inc(load_comp);
    set[1] = inc(load_rtObj);
    set[2] = inc(load_glyphs);
  } else {
    if (!isArr(prim) || rnk(prim)!=1) thrM("•ReBQN: 𝕩.primitives must be a list");
    usz pia = a(prim)->ia;
    usz np[3] = {0}; // number of functions, 1-modifiers, and 2-modifiers
    SGetU(prim);
    for (usz i = 0; i < pia; i++) { // check and count
      B p = GetU(prim, i);
      if (!isArr(p) || rnk(p)!=1 || a(p)->ia!=2) thrM("•ReBQN: 𝕩.primitives must consist of glyph-primitive pairs");
      if (!isC32(IGet(p, 0))) thrM("•ReBQN 𝕩.primitives: Glyphs must be characters");
      B v = IGetU(p, 1);
      i32 t = isFun(v)? 0 : isMd1(v)? 1 : isMd2(v)? 2 : 3;
      if (t==3) thrM("•ReBQN 𝕩.primitives: Primitives must be operations");
      np[t]+= 1;
    }
    
    M_HARR(r, 3)
    u32* gl[3];
    usz sum = 0;
    for (usz i = 0; i < 3; i++) {
      usz l = np[i];
      HARR_ADD(r, i, m_c32arrv(gl+i, l));
      np[i] = sum;
      sum+= l;
    }
    B rb = HARR_FV(r);
    
    HArr_p prh = m_harr0v(pia);
    for (usz i = 0; i < pia; i++) {
      B gv = GetU(prim, i);
      B v = IGet(gv, 1);
      i32 t = isFun(v)? 0 : isMd1(v)? 1 : isMd2(v)? 2 : 3;
      *(gl[t]++) = o2cu(IGet(gv, 0));
      prh.a[np[t]++] = v;
    }
    
    set[1] = prh.b;
    set[2] = inc(rb);
    set[0] = c1(load_compgen, rb);
  }
}
B getPrimitives() {
  B g, r;
  if (q_N(comp_currRe)) {
    g = load_glyphs; r = load_rtObj;
  } else {
    B* o = harr_ptr(comp_currRe); g = o[4]; r = o[3];
  }
  B* pr = harr_ptr(r);
  B* gg = harr_ptr(g);
  M_HARR(ph, a(r)->ia);
  for (usz gi = 0; gi < 3; gi++) {
    usz l = a(gg[gi])->ia;
    u32* gp = c32arr_ptr(gg[gi]);
    for (usz i = 0; i < l; i++) {
      HARR_ADDA(ph, m_hVec2(m_c32(gp[i]), inc(pr[i])));
    }
    pr+= l;
  }
  return HARR_FV(ph);
}

B rebqn_exec(B str, B path, B args, B o) {
  B prevRe = comp_currRe;
  if (CATCH) { comp_currRe = prevRe; rethrow(); }
  comp_currRe = inc(o);
  
  B* op = harr_ptr(o);
  i32 replMode = o2iu(op[0]);
  Scope* sc = c(Scope, op[1]);
  B res;
  if (replMode>0) {
    Block* block = bqn_compScc(str, path, args, sc, op[2], op[3], replMode==2);
    comp_currRe = prevRe;
    ptr_dec(sc->body);
    sc->body = ptr_inc(block->bodies[0]);
    res = execBlockInline(block, sc);
    ptr_dec(block);
  } else {
    B rtsys = m_hVec2(inc(op[3]), incG(bi_sys));
    Block* block = bqn_compc(str, path, args, op[2], rtsys);
    decG(rtsys);
    comp_currRe = prevRe;
    res = evalFunBlock(block, 0);
    ptr_dec(block);
  }
  decG(o);
  
  popCatch();
  return res;
}

static NOINLINE B m_lvB_0(                  ) { return emptyHVec(); }
static NOINLINE B m_lvB_1(B a               ) { return m_hVec1(a); }
static NOINLINE B m_lvB_2(B a, B b          ) { return m_hVec2(a,b); }
static NOINLINE B m_lvB_3(B a, B b, B c     ) { return m_hVec3(a,b,c); }
static NOINLINE B m_lvB_4(B a, B b, B c, B d) { return m_hVec4(a,b,c,d); }
static NOINLINE B m_lvi32_0(                          ) { return emptyIVec(); }
static NOINLINE B m_lvi32_1(i32 a                     ) { i32* rp; B r = m_i32arrv(&rp,1); rp[0]=a; return r; }
static NOINLINE B m_lvi32_2(i32 a, i32 b              ) { i32* rp; B r = m_i32arrv(&rp,2); rp[0]=a; rp[1]=b; return r; }
static NOINLINE B m_lvi32_3(i32 a, i32 b, i32 c       ) { i32* rp; B r = m_i32arrv(&rp,3); rp[0]=a; rp[1]=b; rp[2]=c; return r; }
static NOINLINE B m_lvi32_4(i32 a, i32 b, i32 c, i32 d) { i32* rp; B r = m_i32arrv(&rp,4); rp[0]=a; rp[1]=b; rp[2]=c; rp[3]=d; return r; }

void load_init() { // very last init function
  comp_currPath = bi_N;
  comp_currArgs = bi_N;
  comp_currSrc  = bi_N;
  comp_currRe   = bi_N;
  gc_addFn(load_gcFn);
  B fruntime[] = {
    /* +-×÷⋆√⌊⌈|¬  */ bi_add     , bi_sub    , bi_mul   , bi_div  , bi_pow    , bi_root     , bi_floor , bi_ceil , bi_stile  , bi_not,
    /* ∧∨<>≠=≤≥≡≢  */ bi_and     , bi_or     , bi_lt    , bi_gt   , bi_ne     , bi_eq       , bi_le    , bi_ge   , bi_feq    , bi_fne,
    /* ⊣⊢⥊∾≍⋈↑↓↕«  */ bi_ltack   , bi_rtack  , bi_shape , bi_join , bi_couple , bi_pair     , bi_take  , bi_drop , bi_ud     , bi_shifta,
    /* »⌽⍉/⍋⍒⊏⊑⊐⊒  */ bi_shiftb  , bi_reverse, bi_transp, bi_slash, bi_gradeUp, bi_gradeDown, bi_select, bi_pick , bi_indexOf, bi_count,
    /* ∊⍷⊔!˙˜˘¨⌜⁼  */ bi_memberOf, bi_find   , bi_group , bi_asrt , bi_const  , bi_swap     , bi_cell  , bi_each , bi_tbl    , bi_undo,
    /* ´˝`∘○⊸⟜⌾⊘◶  */ bi_fold    , bi_insert , bi_scan  , bi_atop , bi_over   , bi_before   , bi_after , bi_under, bi_val    , bi_cond,
    /* ⎉⚇⍟⎊        */ bi_rank    , bi_depth  , bi_repeat, bi_catch

  };
  bool rtComplete[] = { // if you unset any of these, also define WRAP_NNBI
    /* +-×÷⋆√⌊⌈|¬  */ 1,1,1,1,1,1,1,1,1,1,
    /* ∧∨<>≠=≤≥≡≢  */ 1,1,1,1,1,1,1,1,1,1,
    /* ⊣⊢⥊∾≍⋈↑↓↕«  */ 1,1,1,1,1,1,1,1,1,1,
    /* »⌽⍉/⍋⍒⊏⊑⊐⊒  */ 1,1,1,1,1,1,1,1,1,1,
    /* ∊⍷⊔!˙˜˘¨⌜⁼  */ 1,1,1,1,1,1,1,1,1,1,
    /* ´˝`∘○⊸⟜⌾⊘◶  */ 1,1,1,1,1,1,1,1,1,1,
    /* ⎉⚇⍟⎊        */ 1,1,1,1
  };
  assert(sizeof(fruntime)/sizeof(B) == rtLen);
  for (u64 i = 0; i < rtLen; i++) inc(fruntime[i]);
  B frtObj = m_caB(rtLen, fruntime);
  
  #ifndef NO_RT
    B provide[] = {bi_type,bi_fill,bi_log,bi_grLen,bi_grOrd,bi_asrt,bi_add,bi_sub,bi_mul,bi_div,bi_pow,bi_floor,bi_eq,bi_le,bi_fne,bi_shape,bi_pick,bi_ud,bi_tbl,bi_scan,bi_fillBy,bi_val,bi_catch};
    #ifndef ALL_R0
    B runtime_0[] = {bi_floor,bi_ceil,bi_stile,bi_lt,bi_gt,bi_ne,bi_ge,bi_rtack,bi_ltack,bi_join,bi_pair,bi_take,bi_drop,bi_select,bi_const,bi_swap,bi_each,bi_fold,bi_atop,bi_over,bi_before,bi_after,bi_cond,bi_repeat};
    #else
    Block* runtime0_b = load_compImport(
      #include "gen/runtime0"
    );
    B r0r = evalFunBlock(runtime0_b, 0); ptr_dec(runtime0_b);
    B* runtime_0 = toHArr(r0r)->a;
    #endif
    
    Block* runtime_b = load_compImport(
      #include "gen/runtime1"
    );
    
    #ifdef ALL_R0
    dec(r0r);
    #endif
    
    B rtRes = evalFunBlock(runtime_b, 0); ptr_dec(runtime_b);
    SGet(rtRes);
    B rtObjRaw = Get(rtRes,0);
    B setPrims = Get(rtRes,1);
    B setInv = Get(rtRes,2);
    dec(rtRes);
    dec(c1(setPrims, m_hVec2(incG(bi_decp), incG(bi_primInd)))); decG(setPrims);
    dec(c2(setInv, incG(bi_setInvSwap), incG(bi_setInvReg))); decG(setInv);
    
    
    
    if (c(Arr,rtObjRaw)->ia != rtLen) err("incorrectly defined rtLen!");
    HArr_p runtimeH = m_harrUc(rtObjRaw);
    SGet(rtObjRaw)
    
    rt_undo    = Get(rtObjRaw, n_undo    ); gc_add(rt_undo);
    rt_select  = Get(rtObjRaw, n_select  ); gc_add(rt_select);
    rt_slash   = Get(rtObjRaw, n_slash   ); gc_add(rt_slash);
    rt_join    = Get(rtObjRaw, n_join    ); gc_add(rt_join);
    rt_ud      = Get(rtObjRaw, n_ud      ); gc_add(rt_ud);
    rt_pick    = Get(rtObjRaw, n_pick    ); gc_add(rt_pick);
    rt_take    = Get(rtObjRaw, n_take    ); gc_add(rt_take);
    rt_drop    = Get(rtObjRaw, n_drop    ); gc_add(rt_drop);
    rt_group   = Get(rtObjRaw, n_group   ); gc_add(rt_group);
    rt_under   = Get(rtObjRaw, n_under   ); gc_add(rt_under);
    rt_reverse = Get(rtObjRaw, n_reverse ); gc_add(rt_reverse);
    rt_indexOf = Get(rtObjRaw, n_indexOf ); gc_add(rt_indexOf);
    rt_count   = Get(rtObjRaw, n_count   ); gc_add(rt_count);
    rt_memberOf= Get(rtObjRaw, n_memberOf); gc_add(rt_memberOf);
    rt_find    = Get(rtObjRaw, n_find    ); gc_add(rt_find);
    rt_cell    = Get(rtObjRaw, n_cell    ); gc_add(rt_cell);
    rt_rank    = Get(rtObjRaw, n_rank    ); gc_add(rt_rank);
    rt_transp  = Get(rtObjRaw, n_transp  ); gc_add(rt_transp);
    rt_depth   = Get(rtObjRaw, n_depth   ); gc_add(rt_depth);
    rt_insert  = Get(rtObjRaw, n_insert  ); gc_add(rt_insert);
    
    for (usz i = 0; i < rtLen; i++) {
      #ifdef RT_WRAP
      r1Objs[i] = Get(rtObjRaw, i); gc_add(r1Objs[i]);
      #endif
      #ifdef ALL_R1
        bool nnbi = true;
        B r = Get(rtObjRaw, i);
      #else
        bool nnbi = !rtComplete[i];
        #if !defined(WRAP_NNBI)
        if (nnbi) err("Refusing to load non-native builtin into runtime without -DWRAP_NNBI");
        #endif
        B r = nnbi? Get(rtObjRaw, i) : inc(fruntime[i]);
      #endif
      if (q_N(r)) err("· in runtime!\n");
      if (isVal(r)) v(r)->flags|= i+1;
      #if defined(RT_WRAP) || defined(WRAP_NNBI)
        r = rtWrap_wrap(r, nnbi);
        if (isVal(r)) v(r)->flags|= i+1;
      #endif
      runtimeH.a[i] = r;
    }
    decG(rtObjRaw);
    B* runtime = runtimeH.a;
    B rtObj = runtimeH.b;
    load_rtObj = FAKE_RUNTIME? frtObj : rtObj;
    load_compArg = m_hVec2(load_rtObj, incG(bi_sys)); gc_add(FAKE_RUNTIME? rtObj : frtObj);
    gc_add(load_compArg);
  #else
    B* runtime = fruntime;
    (void)frtObj;
    (void)rtComplete;
    (void)runtime;
  #endif
  
  
  
  
  #ifdef PRECOMP
    Block* c = compile(
      #include "gen/interp"
      , bi_N, bi_N, bi_N, bi_N, NULL
    );
    B interp = evalFunBlock(c, 0); ptr_dec(c);
    print(interp);
    printf("\n");
    dec(interp);
    #ifdef HEAP_VERIFY
      heapVerify();
    #endif
    rtWrap_print();
    CTR_FOR(CTR_PRINT)
    print_allocStats();
    exit(0);
  #else // use compiler
    B prevAsrt = runtime[n_asrt];
    runtime[n_asrt] = bi_casrt; // horrible but GC is off so it's fiiiiiine
    Block* comp_b = load_compImport(
      #include "gen/compiles"
    );
    runtime[n_asrt] = prevAsrt;
    load_glyphs = m_hVec3(m_str32(U"+-×÷⋆√⌊⌈|¬∧∨<>≠=≤≥≡≢⊣⊢⥊∾≍⋈↑↓↕«»⌽⍉/⍋⍒⊏⊑⊐⊒∊⍷⊔!"), m_str32(U"˙˜˘¨⌜⁼´˝`"), m_str32(U"∘○⊸⟜⌾⊘◶⎉⚇⍟⎊"));
    load_compgen = evalFunBlock(comp_b, 0); ptr_dec(comp_b);
    load_comp = c1(load_compgen, inc(load_glyphs));
    gc_add(load_compgen); gc_add(load_comp); gc_add(load_glyphs);
    
    
    #if FORMATTER
    Block* fmt_b = load_compImport(
      #include "gen/formatter"
    );
    B fmtM = evalFunBlock(fmt_b, 0); ptr_dec(fmt_b);
    B fmtR = c1(fmtM, m_caB(4, (B[]){incG(bi_type), incG(bi_decp), incG(bi_glyph), incG(bi_repr)}));
    decG(fmtM);
    SGet(fmtR)
    load_fmt  = Get(fmtR, 0); gc_add(load_fmt);
    load_repr = Get(fmtR, 1); gc_add(load_repr);
    decG(fmtR);
    #endif
    gc_enable();
  #endif // PRECOMP
}

B bqn_execFile(B path, B args) { // consumes both
  return bqn_exec(path_chars(inc(path)), path, args);
}

void bqn_exit(i32 code) {
  rtWrap_print();
  CTR_FOR(CTR_PRINT)
  print_allocStats();
  exit(code);
}


static void freed_visit(Value* x) {
  #if CATCH_ERRORS
  err("visiting t_freed\n");
  #endif
}
static void empty_free(Value* x) { err("FREEING EMPTY\n"); }
static void builtin_free(Value* x) { err("FREEING BUILTIN\n"); }
DEF_FREE(def) { }
static void def_visit(Value* x) { printf("(no visit for %d=%s)\n", x->type, type_repr(x->type)); }
static void def_print(FILE* f, B x) { fprintf(f, "(%d=%s)", v(x)->type, type_repr(v(x)->type)); }
static bool def_canStore(B x) { return false; }
static B def_identity(B f) { return bi_N; }
static B def_get(Arr* x, usz n) { err("def_get"); }
static B def_getU(Arr* x, usz n) { err("def_getU"); }
static B def_m1_d(B m, B f     ) { thrM("cannot derive this"); }
static B def_m2_d(B m, B f, B g) { thrM("cannot derive this"); }
static Arr* def_slice(B x, usz s, usz ia) { thrM("cannot slice non-array!"); }

B rt_invFnReg, rt_invFnSwap;
BB2B rt_invFnRegFn;
BB2B rt_invFnSwapFn;
B def_fn_im(B t,      B x) { B fn =  rt_invFnRegFn(rt_invFnReg,  inc(t)); B r = c1(fn,    x); dec(fn); return r; }
B def_fn_is(B t,      B x) { B fn = rt_invFnSwapFn(rt_invFnSwap, inc(t)); B r = c1(fn,    x); dec(fn); return r; }
B def_fn_iw(B t, B w, B x) { B fn = rt_invFnSwapFn(rt_invFnSwap, inc(t)); B r = c2(fn, w, x); dec(fn); return r; }
B def_fn_ix(B t, B w, B x) { B fn =  rt_invFnRegFn(rt_invFnReg,  inc(t)); B r = c2(fn, w, x); dec(fn); return r; }
B def_m1_im(Md1D* t,      B x) { return def_fn_im(tag(t,FUN_TAG),    x); }
B def_m1_iw(Md1D* t, B w, B x) { return def_fn_iw(tag(t,FUN_TAG), w, x); }
B def_m1_ix(Md1D* t, B w, B x) { return def_fn_ix(tag(t,FUN_TAG), w, x); }
B def_m2_im(Md2D* t,      B x) { return def_fn_im(tag(t,FUN_TAG),    x); }
B def_m2_iw(Md2D* t, B w, B x) { return def_fn_iw(tag(t,FUN_TAG), w, x); }
B def_m2_ix(Md2D* t, B w, B x) { return def_fn_ix(tag(t,FUN_TAG), w, x); }

#ifdef DONT_FREE
static B empty_get(Arr* x, usz n) {
  x->type = x->flags;
  if (x->type==t_empty) err("getting from empty without old type data");
  B r = TIv(x,get)(x, n);
  x->type = t_empty;
  return r;
}
static B empty_getU(Arr* x, usz n) {
  x->type = x->flags;
  if (x->type==t_empty) err("getting from empty without old type data");
  B r = TIv(x,getU)(x, n);
  x->type = t_empty;
  return r;
}
#endif

static void funBI_visit(Value* x) {
  mm_visit(((BFn*)x)->rtInvReg);
}
static B funBI_imRt(B t, B x) {
  return c1(c(BFn, t)->rtInvReg, x);
}
static B funBI_imInit(B t, B x) {
  B f = c(BFn, t)->rtInvReg = c1(rt_invFnReg, inc(t));
  c(BFn, t)->im = funBI_imRt;
  return c1(f, x);
}



static NOINLINE B m_bfn(BB2B c1, BBB2B c2, u8 id) {
  BFn* f = mm_alloc(sizeof(BFn), t_funBI);
  f->c1 = c1;
  f->c2 = c2;
  f->extra = id;
  f->ident = bi_N;
  f->uc1 = def_fn_uc1;
  f->ucw = def_fn_ucw;
  f->im = funBI_imInit;
  f->rtInvReg = m_f64(0);
  B r = tag(f,FUN_TAG); gc_add(r);
  return r;
}
static NOINLINE B m_bm1(D1C1 c1, D1C2 c2, u8 id) {
  BMd1* m = mm_alloc(sizeof(BMd1), t_md1BI);
  m->c1 = c1;
  m->c2 = c2;
  m->extra = id;
  m->im = def_m1_im;
  m->iw = def_m1_iw;
  m->ix = def_m1_ix;
  B r = tag(m,MD1_TAG); gc_add(r);
  return r;
}

static NOINLINE B m_bm2(D2C1 c1, D2C2 c2, u8 id) {
  BMd2* m = mm_alloc(sizeof(BMd2), t_md2BI);
  m->c1 = c1;
  m->c2 = c2;
  m->extra = id;
  m->im = def_m2_im;
  m->iw = def_m2_iw;
  m->ix = def_m2_ix;
  m->uc1 = def_m2_uc1;
  m->ucw = def_m2_ucw;
  B r = tag(m,MD2_TAG); gc_add(r);
  return r;
}

void base_init() { // very first init function
  for (u64 i = 0; i < t_COUNT; i++) {
    TIi(i,freeO)  = def_freeO;
    TIi(i,freeF)  = def_freeF;
    TIi(i,visit) = def_visit;
    TIi(i,get)   = def_get;
    TIi(i,getU)  = def_getU;
    TIi(i,print) = def_print;
    TIi(i,m1_d)  = def_m1_d;
    TIi(i,m2_d)  = def_m2_d;
    TIi(i,isArr) = false;
    TIi(i,arrD1) = false;
    TIi(i,elType)    = el_B;
    TIi(i,identity)  = def_identity;
    TIi(i,decompose) = def_decompose;
    TIi(i,slice)     = def_slice;
    TIi(i,canStore)  = def_canStore;
    TIi(i,fn_uc1) = def_fn_uc1;
    TIi(i,fn_ucw) = def_fn_ucw;
    TIi(i,m1_uc1) = def_m1_uc1;
    TIi(i,m1_ucw) = def_m1_ucw;
    TIi(i,m2_uc1) = def_m2_uc1;
    TIi(i,m2_ucw) = def_m2_ucw;
    TIi(i,fn_im) = def_fn_im; TIi(i,m1_im) = def_m1_im; TIi(i,m2_im) = def_m2_im;
    TIi(i,fn_is) = def_fn_is;
    TIi(i,fn_iw) = def_fn_iw; TIi(i,m1_iw) = def_m1_iw; TIi(i,m2_iw) = def_m2_iw;
    TIi(i,fn_ix) = def_fn_ix; TIi(i,m1_ix) = def_m1_ix; TIi(i,m2_ix) = def_m2_ix;
  }
  TIi(t_empty,freeO) = empty_free; TIi(t_freed,freeO) = def_freeO;
  TIi(t_empty,freeF) = empty_free; TIi(t_freed,freeF) = def_freeF;
  TIi(t_freed,visit) = freed_visit;
  #ifdef DONT_FREE
    TIi(t_empty,get) = empty_get;
    TIi(t_empty,getU) = empty_getU;
  #endif
  TIi(t_shape,visit) = noop_visit;
  TIi(t_funBI,visit) = TIi(t_md1BI,visit) = TIi(t_md2BI,visit) = noop_visit;
  TIi(t_funBI,freeO) = TIi(t_md1BI,freeO) = TIi(t_md2BI,freeO) = builtin_free;
  TIi(t_funBI,freeF) = TIi(t_md1BI,freeF) = TIi(t_md2BI,freeF) = builtin_free;
  TIi(t_funBI,visit) = funBI_visit;
  TIi(t_hashmap,visit) = noop_visit;
  
  assert((MD1_TAG>>1) == (MD2_TAG>>1)); // just to be sure it isn't changed incorrectly, `isMd` depends on this
  
  #define FA(N,X) bi_##N = m_bfn(N##_c1, N##_c2, pf_##N);
  #define FM(N,X) bi_##N = m_bfn(N##_c1, c2_bad, pf_##N);
  #define FD(N,X) bi_##N = m_bfn(c1_bad, N##_c2, pf_##N);
  FOR_PFN(FA,FM,FD)
  #undef FA
  #undef FM
  #undef FD
  
  #define FA(N,X) bi_##N = m_bm1(N##_c1,   N##_c2,   pm1_##N);
  #define FM(N,X) bi_##N = m_bm1(N##_c1,   m1c2_bad, pm1_##N);
  #define FD(N,X) bi_##N = m_bm1(m1c1_bad, N##_c2,   pm1_##N);
  FOR_PM1(FA,FM,FD)
  #undef FA
  #undef FM
  #undef FD
  
  #define FA(N,X) bi_##N = m_bm2(N##_c1,   N##_c2,   pm2_##N);
  #define FM(N,X) bi_##N = m_bm2(m2c1_bad, N##_c2,   pm2_##N);
  #define FD(N,X) bi_##N = m_bm2(N##_c1,   m2c2_bad, pm2_##N);
  FOR_PM2(FA,FM,FD)
  #undef FA
  #undef FM
  #undef FD
}

bool cbqn_initialized;
void cbqn_init() {
  #define F(X) X##_init();
   FOR_INIT(F)
  #undef F
   cbqn_initialized = true;
}
#undef FOR_INIT
