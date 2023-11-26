#include "core.h"
#include "utils/file.h"
#include "vm.h"
#include "ns.h"
#include "builtins.h"
#include "load.h"

#ifndef FAKE_RUNTIME
  #define FAKE_RUNTIME 0
#endif

#define PRECOMPILED_FILE(END) STR1(../build/BYTECODE_DIR/gen/END)

#define FOR_INIT(F) \
/* initialize primary things */ F(base) F(harr) F(mutF) F(cmpA) F(fillarr) F(tyarr) F(hash) F(sfns) F(fns) F(arithm) F(arithd) F(md1) F(md2) F(derv) F(comp) F(rtWrap) F(ns) F(nfn) F(sysfn) F(inverse) F(slash) F(search) F(transp) F(ryu) F(ffi) F(mmap) \
/* first thing that executes BQN code (the precompiled stuff) */ F(load) \
/* precompiled stuff loaded; init things that need it */ F(sysfnPost) F(dervPost) F(typesFinished)

#define F(X) NOINLINE void X##_init(void);
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
//         [...variableIDs], // a number for each variable slot; indexes into nameList
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

char* pfn_repr(u8 u) {
  switch(u) { default: return "(unknown function)";
    #define F(N,X) case pf_##N: return X;
    FOR_PFN(F,F,F)
    #undef F
  }
}
char* pm1_repr(u8 u) {
  switch(u) { default: return "(unknown 1-modifier)";
    #define F(N,X) case pm1_##N: return X;
    FOR_PM1(F,F,F)
    #undef F
  }
}
char* pm2_repr(u8 u) {
  switch(u) { default: return "(unknown 2-modifier)";
    #define F(N,X) case pm2_##N: return X;
    FOR_PM2(F,F,F)
    #undef F
  }
}



#define F(TY,N) TY ti_##N[t_COUNT];
  FOR_TI(F)
#undef F

B r1Objs[RT_LEN];
B rtWrap_wrap(B x, bool nnbi); // consumes
void rtWrap_print(void);

static NOINLINE B m_lvB_0(                  ) { return emptyHVec(); }
static NOINLINE B m_lvB_1(B a               ) { return m_hvec1(a); }
static NOINLINE B m_lvB_2(B a, B b          ) { return m_hvec2(a,b); }
static NOINLINE B m_lvB_3(B a, B b, B c     ) { return m_hvec3(a,b,c); }
static NOINLINE B m_lvB_4(B a, B b, B c, B d) { return m_hvec4(a,b,c,d); }
static NOINLINE B m_lvi32_0(                          ) { return emptyIVec(); }
static NOINLINE B m_lvi32_1(i32 a                     ) { i32* rp; B r = m_i32arrv(&rp,1); rp[0]=a; return r; }
static NOINLINE B m_lvi32_2(i32 a, i32 b              ) { i32* rp; B r = m_i32arrv(&rp,2); rp[0]=a; rp[1]=b; return r; }
static NOINLINE B m_lvi32_3(i32 a, i32 b, i32 c       ) { i32* rp; B r = m_i32arrv(&rp,3); rp[0]=a; rp[1]=b; rp[2]=c; return r; }
static NOINLINE B m_lvi32_4(i32 a, i32 b, i32 c, i32 d) { i32* rp; B r = m_i32arrv(&rp,4); rp[0]=a; rp[1]=b; rp[2]=c; rp[3]=d; return r; }

static NOINLINE B evalFunBlockConsume(Block* block) {
  B r = evalFunBlock(block, NULL);
  ptr_dec(block);
  return r;
}

HArr* comps_curr;

B rt_undo, rt_select, rt_slash, rt_insert, rt_depth,
  rt_group, rt_under, rt_find;
Block* load_buildBlock(B x, B src, B path, Scope* sc, i32 nsResult) { // consumes x,src
  SGet(x)
  usz xia = IA(x);
  if (xia!=6 & xia!=4) thrM("load_buildBlock: bad item count");
  Block* r = xia==6? compileAll(Get(x,0),Get(x,1),Get(x,2),Get(x,3),Get(x,4),Get(x,5), src, inc(path), sc, nsResult)
                   : compileAll(Get(x,0),Get(x,1),Get(x,2),Get(x,3),bi_N,    bi_N,     src, inc(path), sc, nsResult);
  decG(x);
  return r;
}

#if !(ONLY_NATIVE_COMP && !FORMATTER && NO_RT && NO_EXPLAIN)
#include PRECOMPILED_FILE(src)
#endif

#if RT_SRC
Block* load_importBlock(char* name, B bc, B objs, B blocks, B bodies, B inds, B src) { // consumes all
  return compileAll(bc, objs, blocks, bodies, inds, bi_N, src, m_c8vec_0(name), NULL, 0);
}
#else
Block* load_importBlock(char* name, B bc, B objs, B blocks, B bodies) { // consumes all
  return compileAll(bc, objs, blocks, bodies, bi_N, bi_N, bi_N, m_c8vec_0(name), NULL, 0);
}
#endif

B load_compgen;
B def_re;

static void change_def_comp(B comp) { // consumes
  B* re = harr_ptr(def_re); // not pretty changing in-place, but still should be fine and it's for an unsafe feature anyway
  B prev = re[re_comp];
  re[re_comp] = comp;
  dec(prev);
}


#if NATIVE_COMPILER
#include "opt/comp.c"
B load_fullComp;
void switchComp(void) {
  change_def_comp(harr_ptr(def_re)[re_comp].u==load_fullComp.u? incG(native_comp) : incG(load_fullComp));
}
#endif
B compObj_c1(B t, B x) {
  return evalFunBlockConsume(load_buildBlock(x, bi_N, bi_N, NULL, 0));
}
B compObj_c2(B t, B w, B x) {
  change_def_comp(x);
  return w;
}

#if FORMATTER
B load_fmt, load_repr;
B bqn_fmt(B x) { // consumes
  return c1G(load_fmt, x);
}
B bqn_repr(B x) { // consumes
  return c1G(load_repr, x);
}
#else
B bqn_fmt(B x) { return x; }
B bqn_repr(B x) { return x; }
#endif

static NOINLINE void comps_push(B src, B state, B re) {
  assert(comps_curr == NULL);
  HArr* r = m_harr0v(comps_max).c;
  if (q_N(state)) {
    COMPS_REF(r,path) = COMPS_REF(r,name) = COMPS_REF(r,args) = bi_N;
  } else {
    SGet(state)
    COMPS_REF(r, path) = Get(state,0);
    COMPS_REF(r, name) = Get(state,1);
    COMPS_REF(r, args) = Get(state,2);
    decG(state);
  }
  COMPS_REF(r, src)    = inc(src);
  COMPS_REF(r, re)     = inc(re);
  COMPS_REF(r, envPos) = m_f64(envCurr-envStart);
  comps_curr = r;
}

#define COMPS_PUSH(STR, STATE, RE)   \
  comps_push(STR, STATE, RE);        \
  if (CATCH) { COMPS_POP; rethrow(); }

#define COMPS_POP ({ ptr_dec(comps_curr); comps_curr = NULL; })

static NOINLINE Block* bqn_compc(B str, B state, B re) { // consumes str,state
  str = chr_squeeze(str);
  COMPS_PUSH(str, state, re);
  B* o = harr_ptr(re);
  Block* r = load_buildBlock(c2G(o[re_comp], incG(o[re_compOpts]), inc(str)), str, COMPS_CREF(path), NULL, 0);
  COMPS_POP; popCatch();
  return r;
}
Block* bqn_comp(B str, B state) {
  return bqn_compc(str, state, def_re);
}
Block* bqn_compScc(B str, B state, B re, Scope* sc, bool loose, bool noNS) {
  str = chr_squeeze(str);
  COMPS_PUSH(str, state, re);
  B* o = harr_ptr(re);
  B vName = emptyHVec();
  B vDepth = emptyIVec();
  if (loose && (!sc || sc->psc)) thrM("VM compiler: REPL mode must be used at top level scope");
  i32 depth = loose? -1 : 0;
  Scope* csc = sc;
  while (csc) {
    B vars = listVars(csc);
    usz am = IA(vars);
    vName = vec_join(vName, vars);
    for (usz i = 0; i < am; i++) vDepth = vec_addN(vDepth, m_i32(depth));
    csc = csc->psc;
    depth++;
  }
  Block* r = load_buildBlock(c2G(o[re_comp], m_lvB_4(incG(o[re_rt]), incG(bi_sys), vName, vDepth), inc(str)), str, COMPS_CREF(path), sc, sc!=NULL? (noNS? -1 : 1) : 0);
  COMPS_POP; popCatch();
  return r;
}
NOINLINE Block* bqn_compSc(B str, B state, Scope* sc, bool repl) {
  return bqn_compScc(str, state, def_re, sc, repl, false);
}

B bqn_exec(B str, B state) { // consumes all
  return evalFunBlockConsume(bqn_comp(str, state));
}

B str_all, str_none;
void init_comp(B* new_re, B* prev_re, B prim, B sys) {
  if (q_N(prim)) {
    new_re[re_comp]     = inc(prev_re[re_comp]);
    new_re[re_compOpts] = inc(prev_re[re_compOpts]);
    new_re[re_rt]       = inc(prev_re[re_rt]);
    new_re[re_glyphs]   = inc(prev_re[re_glyphs]);
  } else {
    if (!isArr(prim) || RNK(prim)!=1) thrM("•ReBQN: 𝕩.primitives must be a list");
    usz pia = IA(prim);
    usz np[3] = {0}; // number of functions, 1-modifiers, and 2-modifiers
    SGetU(prim);
    for (usz i = 0; i < pia; i++) { // check and count
      B p = GetU(prim, i);
      if (!isArr(p) || RNK(p)!=1 || IA(p)!=2) thrM("•ReBQN: 𝕩.primitives must consist of glyph-primitive pairs");
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
      *(gl[t]++) = o2cG(IGet(gv, 0));
      prh.a[np[t]++] = v;
    }
    
    new_re[re_rt]       = prh.b;
    new_re[re_glyphs]   = inc(rb);
    new_re[re_comp]     = c1(load_compgen, rb);
    new_re[re_compOpts] = m_lvB_2(inc(prh.b), incG(bi_sys));
  }
  
  if (q_N(sys)) {
    inherit_sys:
    new_re[re_sysNames] = inc(prev_re[re_sysNames]);
    new_re[re_sysVals]  = inc(prev_re[re_sysVals]);
  } else {
    if (!isArr(sys) || RNK(sys)!=1) thrM("•ReBQN: 𝕩.system must be a list");
    if (str_all.u==0) { gc_add(str_all = m_c8vec("all",3)); gc_add(str_none = m_c8vec("none",4)); }
    if (equal(sys, str_all)) goto inherit_sys;
    if (equal(sys, str_none)) {
      new_re[re_sysNames] = new_re[re_sysVals] = emptyHVec();
    } else {
      usz ia = IA(sys); SGetU(sys)
      M_HARR(nns, ia);
      M_HARR(nvs, ia);
      i8* inh; B inhA = m_i8arrv(&inh, ia);
      for (usz i = 0; i < ia; i++) { // write defined system values, inherited ones get inserted later
        B c = GetU(sys,i);
        B nn, nv;
        if (isStr(c)) {
          nn = incG(c);
          nv = bi_N;
          inh[i] = 1;
        } else {
          if (!isArr(c) || RNK(c)!=1 || IA(c)!=2) thrM("•ReBQN: 𝕩.system must be either \"all\", \"none\", or a list of names or pairs of name & value");
          SGet(c)
          nn = Get(c,0);
          nv = Get(c,1);
          inh[i] = 0;
        }
        HARR_ADD(nns, i, nn);
        HARR_ADD(nvs, i, nv);
      }
      B nnsA = HARR_FV(nns);
      
      // system value inheriting
      B inhNs = C2(slash, incG(inhA), incG(nnsA));
      usz inhN = IA(inhNs);
      if (inhN>0) {
        B prevN = prev_re[re_sysNames];
        B prevV = prev_re[re_sysVals];
        B idxs = C2(indexOf, incG(prevN), inhNs);
        B inhNIs = C1(slash, inhA);
        SGetU(idxs) SGet(prevV) SGetU(inhNIs)
        for (usz i = 0; i < inhN; i++) {
          usz oi = o2sG(GetU(inhNIs,i));
          usz idx = o2sG(GetU(idxs,i));
          if (idx == IA(prevN)) thrF("•ReBQN: No system value \"%R\" to inherit", IGet(nnsA, oi));
          HARR_O(nvs).a[oi] = Get(prevV, idx);
        }
        decG(inhNIs); decG(idxs);
      } else {
        decG(inhNs);
        decG(inhA);
      }
      
      new_re[re_sysNames] = nnsA;
      new_re[re_sysVals]  = HARR_FV(nvs);
    }
  }
}
B comps_getPrimitives(void) {
  B* o = harr_ptr(COMPS_CREF(re));
  B r = o[re_rt];
  B g = o[re_glyphs];
  B* pr = harr_ptr(r);
  B* gg = harr_ptr(g);
  M_HARR(ph, IA(r));
  for (usz gi = 0; gi < 3; gi++) {
    usz l = IA(gg[gi]);
    u32* gp = c32arr_ptr(gg[gi]);
    for (usz i = 0; i < l; i++) {
      HARR_ADDA(ph, m_lvB_2(m_c32(gp[i]), inc(pr[i])));
    }
    pr+= l;
  }
  return HARR_FV(ph);
}

void comps_getSysvals(B* res) {
  B* o = harr_ptr(COMPS_CREF(re));
  res[0] = o[re_sysNames];
  res[1] = o[re_sysVals];
}

B rebqn_exec(B str, B state, B re) {
  return evalFunBlockConsume(bqn_compc(str, state, re));
}
B repl_exec(B str, B state, B re) {
  B* op = harr_ptr(re);
  i32 replMode = o2iG(op[re_mode]);
  if (replMode>0) {
    Scope* sc = c(Scope, op[re_scope]);
    Block* block = bqn_compScc(str, state, re, sc, replMode==2, true);
    ptr_dec(sc->body);
    sc->body = ptr_inc(block->bodies[0]);
    B res = execBlockInplace(block, sc);
    ptr_dec(block);
    return res;
  } else {
    return rebqn_exec(str, state, re);
  }
}

B invalidFn_c1(B t, B x);

void comps_gcFn() {
  if (comps_curr!=NULL) mm_visitP(comps_curr);
}
void load_init() { // very last init function
  comps_curr = NULL;
  gc_addFn(comps_gcFn);
  gc_add_ref(&rt_invFnReg);
  gc_add_ref(&rt_invFnSwap);
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
  assert(sizeof(fruntime)/sizeof(B) == RT_LEN);
  for (u64 i = 0; i < RT_LEN; i++) inc(fruntime[i]);
  B frtObj = m_caB(RT_LEN, fruntime);
  
  B load_compOpts, load_rt;
  #if !NO_RT
    B provide[] = {
      /* actual provide: */
      bi_type,bi_fill,bi_log,bi_grLen,bi_grOrd,bi_asrt,bi_add,bi_sub,bi_mul,bi_div,bi_pow,bi_floor,bi_eq,bi_le,bi_fne,bi_shape,bi_pick,bi_ud,bi_tbl,bi_scan,bi_fillBy,bi_val,bi_catch
      /* result list from commented-out •Out line in cc.bqn: */,
      bi_root,bi_not,bi_and,bi_or,bi_feq,bi_couple,bi_shifta,bi_shiftb,bi_reverse,bi_transp,bi_gradeUp,bi_gradeDown,bi_indexOf,bi_count,bi_memberOf,bi_cell,bi_rank
    };
    
    #if !ALL_R0
      B runtime_0[] = {bi_floor,bi_ceil,bi_stile,bi_lt,bi_gt,bi_ne,bi_ge,bi_rtack,bi_ltack,bi_join,bi_pair,bi_take,bi_drop,bi_select,bi_const,bi_swap,bi_each,bi_fold,bi_atop,bi_over,bi_before,bi_after,bi_cond,bi_repeat};
    #else
      Block* runtime0_b = load_importBlock("(self-hosted runtime0)",
        #include PRECOMPILED_FILE(runtime0)
      );
      HArr* r0r = toHArr(evalFunBlockConsume(runtime0_b));
      B* runtime_0 = r0r->a;
    #endif
    
    Block* runtime_b = load_importBlock("(self-hosted runtime1)",
      #if ALL_R0 || ALL_R1 || NO_EXTENDED_PROVIDE || RT_VERIFY
        #include PRECOMPILED_FILE(runtime1)
      #else
        #include PRECOMPILED_FILE(runtime1x)
      #endif
    );
    
    #if ALL_R0
      ptr_dec(r0r);
    #endif
    
    B rtRes = evalFunBlockConsume(runtime_b);
    SGet(rtRes);
    B rtObjRaw = Get(rtRes,0);
    B setPrims = Get(rtRes,1);
    B setInv = Get(rtRes,2);
    dec(rtRes);
    dec(c1G(setPrims, m_lvB_2(incG(bi_decp), incG(bi_primInd)))); decG(setPrims);
    dec(c2G(setInv, incG(bi_setInvSwap), incG(bi_setInvReg))); decG(setInv);
    
    
    
    if (IA(rtObjRaw) != RT_LEN) fatal("incorrectly defined RT_LEN!");
    HArr_p runtimeH = m_harrUc(rtObjRaw);
    SGet(rtObjRaw)
    
    gc_add(rt_undo    = Get(rtObjRaw, n_undo  ));
    gc_add(rt_select  = Get(rtObjRaw, n_select));
    gc_add(rt_slash   = Get(rtObjRaw, n_slash ));
    gc_add(rt_group   = Get(rtObjRaw, n_group ));
    gc_add(rt_under   = Get(rtObjRaw, n_under ));
    gc_add(rt_find    = Get(rtObjRaw, n_find  ));
    gc_add(rt_depth   = Get(rtObjRaw, n_depth ));
    gc_add(rt_insert  = Get(rtObjRaw, n_insert));
    
    for (usz i = 0; i < RT_LEN; i++) {
      #ifdef RT_WRAP
      r1Objs[i] = Get(rtObjRaw, i); gc_add(r1Objs[i]);
      #endif
      #if ALL_R1
        bool nnbi = true;
        B r = Get(rtObjRaw, i);
      #else
        bool nnbi = !rtComplete[i];
        #if !defined(WRAP_NNBI)
        if (nnbi) fatal("Refusing to load non-native builtin into runtime without -DWRAP_NNBI");
        #endif
        B r = nnbi? Get(rtObjRaw, i) : inc(fruntime[i]);
      #endif
      if (q_N(r)) fatal("· in runtime!\n");
      if (isVal(r)) v(r)->flags|= i+1;
      #if defined(RT_WRAP) || defined(WRAP_NNBI)
        r = rtWrap_wrap(r, nnbi);
        if (isVal(r)) v(r)->flags|= i+1;
      #endif
      runtimeH.a[i] = r;
    }
    NOGC_E;
    decG(rtObjRaw);
    B* runtime = runtimeH.a;
    B rtObj = runtimeH.b;
    load_rt = FAKE_RUNTIME? frtObj : rtObj;
    gc_add(   FAKE_RUNTIME? rtObj : frtObj);
  #else // NO_RT
    B* runtime = fruntime;
    (void)frtObj;
    (void)rtComplete;
    (void)runtime;
    for (usz i = 0; i < RT_LEN; i++) {
      B r = fruntime[i];
      if (isVal(r)) v(r)->flags|= i+1;
    }
    load_rt = frtObj;
    
    rt_select = rt_slash = rt_group = rt_find = bi_invalidFn;
    rt_undo = rt_insert = bi_invalidMd1;
    rt_under = rt_depth = bi_invalidMd2;
    
    rt_invFnRegFn = rt_invFnSwapFn = invalidFn_c1;
    rt_invFnReg   = rt_invFnSwap   = incByG(bi_invalidFn, 2);
  #endif
  load_compOpts = m_lvB_2(load_rt, incG(bi_sys));
  
  
  #ifdef PRECOMP
    decG(load_compOpts);
    Block* c = compileAll(
      #include "../build/interp"
      , bi_N, bi_N, bi_N, bi_N, NULL, 0
    );
    B interp = evalFunBlockConsume(c);
    printI(interp);
    printf("\n");
    dec(interp);
    #if HEAP_VERIFY
      cbqn_heapVerify();
    #endif
    bqn_exit(0);
  #else // use compiler
    B load_glyphs = m_lvB_3(m_c32vec_0(U"+-×÷⋆√⌊⌈|¬∧∨<>≠=≤≥≡≢⊣⊢⥊∾≍⋈↑↓↕«»⌽⍉/⍋⍒⊏⊑⊐⊒∊⍷⊔!"), m_c32vec_0(U"˙˜˘¨⌜⁼´˝`"), m_c32vec_0(U"∘○⊸⟜⌾⊘◶⎉⚇⍟⎊"));
    
    B load_comp;
    #if ONLY_NATIVE_COMP
      load_comp = m_f64(0);
    #else
      B prevAsrt = runtime[n_asrt];
      runtime[n_asrt] = bi_casrt; // horrible but GC is off so it's fiiiiiine
      Block* comp_b = load_importBlock("(compiler)",
        #include PRECOMPILED_FILE(compiles)
      );
      runtime[n_asrt] = prevAsrt;
      gc_add(load_compgen = evalFunBlockConsume(comp_b));
      
      load_comp = c1(load_compgen, incG(load_glyphs));
      #if NATIVE_COMPILER
        load_fullComp = load_comp;
      #endif
    #endif
    HArr_p ps = m_harr0v(re_max);
    ps.a[re_comp] = load_comp;
    ps.a[re_compOpts] = load_compOpts;
    ps.a[re_rt] = incG(load_rt);
    ps.a[re_glyphs] = load_glyphs;
    ps.a[re_sysNames] = incG(def_sysNames);
    ps.a[re_sysVals] = incG(def_sysVals);
    gc_add(def_re = ps.b);
    
    #if FORMATTER
      Block* fmt_b = load_importBlock("(formatter)",
        #include PRECOMPILED_FILE(formatter)
      );
      B fmtM = evalFunBlockConsume(fmt_b);
      B fmtR = c1(fmtM, m_caB(4, (B[]){incG(bi_type), incG(bi_decp), incG(bi_glyph), incG(bi_repr)}));
      decG(fmtM); SGet(fmtR)
      gc_add(load_fmt  = Get(fmtR, 0));
      gc_add(load_repr = Get(fmtR, 1));
      decG(fmtR);
    #endif
    
  #endif // PRECOMP
}

NOINLINE B m_state(B path, B name, B args) { return m_lvB_3(path, name, args); }
B bqn_execFile(B path, B args) { // consumes both
  B state = m_state(path_parent(inc(path)), path_name(inc(path)), args);
  return bqn_exec(path_chars(path), state);
}

void before_exit(void);
void bqn_exit(i32 code) {
  #ifdef DUMP_ON_EXIT
    cbqn_heapDump(NULL);
  #endif
  rtWrap_print();
  print_allocStats();
  before_exit();
  exit(code);
}

static B load_explain;
B bqn_explain(B str) {
  #if NO_EXPLAIN
    thrM("Explainer not included in this CBQN build");
  #else
    B* o = harr_ptr(def_re);
    if (load_explain.u==0) {
      B* runtime = harr_ptr(o[re_rt]);
      Block* expl_b = load_importBlock("(explain)",
        #include PRECOMPILED_FILE(explain)
      );
      gc_add(load_explain = evalFunBlockConsume(expl_b));
    }
    
    COMPS_PUSH(str, bi_N, def_re);
    B c = c2(o[re_comp], incG(o[re_compOpts]), inc(str));
    COMPS_POP;
    B ret = c2(load_explain, c, str);
    return ret;
  #endif
}



static void freed_visit(Value* x) {
  #if CATCH_ERRORS
  fatal("visiting t_freed\n");
  #endif
}
static void empty_free(Value* x) { fatal("FREEING EMPTY\n"); }
static void builtin_free(Value* x) { fatal("FREEING BUILTIN\n"); }
DEF_FREE(def) { }
static void def_visit(Value* x) { fatal("undefined visit for object\n"); }
static void def_print(FILE* f, B x) { fprintf(f, "(%d=%s)", v(x)->type, type_repr(v(x)->type)); }
static bool def_canStore(B x) { return false; }
static B def_identity(B f) { return bi_N; }
static B def_get(Arr* x, usz n) { fatal("def_get"); }
static B def_getU(Arr* x, usz n) { fatal("def_getU"); }
static B def_m1_d(B m, B f     ) { thrM("cannot derive this"); }
static B def_m2_d(B m, B f, B g) { thrM("cannot derive this"); }
static Arr* def_slice(B x, usz s, usz ia) { fatal("cannot slice non-array!"); }

B rt_invFnReg, rt_invFnSwap;
FC1 rt_invFnRegFn;
FC1 rt_invFnSwapFn;
B def_fn_im(B t,      B x) { B fn =  rt_invFnRegFn(rt_invFnReg,  inc(t)); SLOW2("!runtime 𝕎⁼𝕩",  t, x);    B r = c1(fn,    x); dec(fn); return r; }
B def_fn_is(B t,      B x) { B fn = rt_invFnSwapFn(rt_invFnSwap, inc(t)); SLOW2("!runtime 𝕎⁼𝕩",  t, x);    B r = c1(fn,    x); dec(fn); return r; }
B def_fn_iw(B t, B w, B x) { B fn = rt_invFnSwapFn(rt_invFnSwap, inc(t)); SLOW3("!runtime 𝕨F⁼𝕩", w, x, t); B r = c2(fn, w, x); dec(fn); return r; }
B def_fn_ix(B t, B w, B x) { B fn =  rt_invFnRegFn(rt_invFnReg,  inc(t)); SLOW3("!runtime 𝕨F⁼𝕩", w, x, t); B r = c2(fn, w, x); dec(fn); return r; }
B def_m1_im(Md1D* d,      B x) { return def_fn_im(tag(d,FUN_TAG),    x); }
B def_m1_iw(Md1D* d, B w, B x) { return def_fn_iw(tag(d,FUN_TAG), w, x); }
B def_m1_ix(Md1D* d, B w, B x) { return def_fn_ix(tag(d,FUN_TAG), w, x); }
B def_m2_im(Md2D* d,      B x) { return def_fn_im(tag(d,FUN_TAG),    x); }
B def_m2_iw(Md2D* d, B w, B x) { return def_fn_iw(tag(d,FUN_TAG), w, x); }
B def_m2_ix(Md2D* d, B w, B x) { return def_fn_ix(tag(d,FUN_TAG), w, x); }

#if DONT_FREE
static B empty_get(Arr* x, usz n) {
  x->type = x->flags;
  if (x->type==t_empty) fatal("getting from empty without old type data");
  B r = TIv(x,get)(x, n);
  x->type = t_empty;
  return r;
}
static B empty_getU(Arr* x, usz n) {
  x->type = x->flags;
  if (x->type==t_empty) fatal("getting from empty without old type data");
  B r = TIv(x,getU)(x, n);
  x->type = t_empty;
  return r;
}
#endif

static void funBI_visit(Value* x) {
  mm_visit(((BFn*)x)->rtInvReg);
  mm_visit(((BFn*)x)->rtInvSwap);
}
static B funBI_imRt(B t,      B x) { return c1(c(BFn, t)->rtInvReg,     x); }
static B funBI_isRt(B t,      B x) { return c1(c(BFn, t)->rtInvSwap,    x); }
static B funBI_iwRt(B t, B w, B x) { return c2(c(BFn, t)->rtInvSwap, w, x); }
static B funBI_ixRt(B t, B w, B x) { return c2(c(BFn, t)->rtInvReg,  w, x); }
static B funBI_imInit(B t,      B x) { B f=c(BFn,t)->rtInvReg;  if(f.u==0) f=c(BFn,t)->rtInvReg =c1rt(invFnReg,  incG(t)); c(BFn,t)->im=funBI_imRt; return c1(f, x); }
static B funBI_isInit(B t,      B x) { B f=c(BFn,t)->rtInvSwap; if(f.u==0) f=c(BFn,t)->rtInvSwap=c1rt(invFnSwap, incG(t)); c(BFn,t)->is=funBI_isRt; return c1(f, x); }
static B funBI_ixInit(B t, B w, B x) { B f=c(BFn,t)->rtInvReg;  if(f.u==0) f=c(BFn,t)->rtInvReg =c1rt(invFnReg,  incG(t)); c(BFn,t)->ix=funBI_ixRt; return c2(f, w, x); }
static B funBI_iwInit(B t, B w, B x) { B f=c(BFn,t)->rtInvSwap; if(f.u==0) f=c(BFn,t)->rtInvSwap=c1rt(invFnSwap, incG(t)); c(BFn,t)->iw=funBI_iwRt; return c2(f, w, x); }


void* m_customObj(u64 size, V2v visit, V2v freeO) {
  CustomObj* r = mm_alloc(size, t_customObj);
  r->visit = visit;
  r->freeO = freeO;
  return r;
}
void customObj_visit(Value* v) { ((CustomObj*)v)->visit(v); }
void customObj_freeO(Value* v) { ((CustomObj*)v)->freeO(v); }
void customObj_freeF(Value* v) { ((CustomObj*)v)->freeO(v); mm_free(v); }

void def_fallbackTriv(Value* v) { // used while vtables aren't yet fully loaded; should become completely unused after typesFinished_init
  TIv(v,freeF)(v);
}

static NOINLINE B m_bfn(FC1 c1, FC2 c2, u8 id) {
  BFn* f = mm_alloc(sizeof(BFn), t_funBI);
  f->c1 = c1;
  f->c2 = c2;
  f->extra = id;
  f->ident = bi_N;
  f->uc1 = def_fn_uc1;
  f->ucw = def_fn_ucw;
  f->im = funBI_imInit;
  f->iw = funBI_iwInit;
  f->ix = funBI_ixInit;
  f->is = funBI_isInit;
  f->rtInvReg  = m_f64(0);
  f->rtInvSwap = m_f64(0);
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
    TIi(i,freeT)  = def_fallbackTriv;
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
  TIi(t_empty,freeO) = empty_free; TIi(t_invalid,freeO) = empty_free; TIi(t_freed,freeO) = def_freeO;
  TIi(t_empty,freeF) = empty_free; TIi(t_invalid,freeF) = empty_free; TIi(t_freed,freeF) = def_freeF;
  TIi(t_invalid,visit) = freed_visit;
  TIi(t_freed,visit) = freed_visit;
  #if DONT_FREE
    TIi(t_empty,get) = empty_get;
    TIi(t_empty,getU) = empty_getU;
  #endif
  TIi(t_shape,visit) = noop_visit;
  TIi(t_temp,visit) = noop_visit;
  TIi(t_talloc,visit) = noop_visit;
  TIi(t_funBI,visit) = TIi(t_md1BI,visit) = TIi(t_md2BI,visit) = noop_visit;
  TIi(t_funBI,freeO) = TIi(t_md1BI,freeO) = TIi(t_md2BI,freeO) = builtin_free;
  TIi(t_funBI,freeF) = TIi(t_md1BI,freeF) = TIi(t_md2BI,freeF) = builtin_free;
  TIi(t_funBI,visit) = funBI_visit;
  TIi(t_hashmap,visit) = noop_visit;
  TIi(t_customObj,freeO) = customObj_freeO;
  TIi(t_customObj,freeF) = customObj_freeF;
  TIi(t_customObj,visit) = customObj_visit;
  TIi(t_arbObj,visit) = noop_visit;
  
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
void typesFinished_init() {
  for (u64 i = 0; i < t_COUNT; i++) {
    if (TIi(i,freeT) == def_fallbackTriv) TIi(i,freeT) = TIi(i,freeF);
  }
  #if NATIVE_COMPILER
    nativeCompiler_init();
    #if ONLY_NATIVE_COMP
      change_def_comp(incG(native_comp));
    #endif
  #endif
}

bool cbqn_initialized;
void cbqn_init() {
  if (cbqn_initialized) return;
  #define F(X) X##_init();
    FOR_INIT(F)
  #undef F
  cbqn_initialized = true;
  gc_enable();
}
#undef FOR_INIT
