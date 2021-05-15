#include "vm.h"
#include "ns.h"

// #define GS_REALLOC // whether to dynamically realloc gStack
#ifndef GS_SIZE
#define GS_SIZE 65536 // if !GS_REALLOC, size in number of B objects of the global object stack
#endif
#ifndef ENV_SIZE
#define ENV_SIZE 4096 // max recursion depth; GS_SIZE and C stack size may limit this
#endif

enum {
  PUSH =  0, // N; push object from objs[N]
  VARO =  1, // N; push variable with name strs[N]
  VARM =  2, // N; push mutable variable with name strs[N]
  ARRO =  3, // N; create a vector of top N items
  ARRM =  4, // N; create a mutable vector of top N items
  FN1C =  5, // monadic function call ⟨…,x,f  ⟩ → F x
  FN2C =  6, //  dyadic function call ⟨…,x,f,w⟩ → w F x
  OP1D =  7, // derive 1-modifier to function; ⟨…,  _m,f⟩ → (f _m)
  OP2D =  8, // derive 2-modifier to function; ⟨…,g,_m,f⟩ → (f _m_ g)
  TR2D =  9, // derive 2-train aka atop; ⟨…,  g,f⟩ → (f g)
  TR3D = 10, // derive 3-train aka fork; ⟨…,h,g,f⟩ → (f g h)
  SETN = 11, // set new; _  ←_; ⟨…,x,  mut⟩ → mut←x
  SETU = 12, // set upd; _  ↩_; ⟨…,x,  mut⟩ → mut↩x
  SETM = 13, // set mod; _ F↩_; ⟨…,x,F,mut⟩ → mut F↩x
  POPS = 14, // pop object from stack
  DFND = 15, // N; push dfns[N], derived to current scope
  FN1O = 16, // optional monadic call (FN1C but checks for · at 𝕩)
  FN2O = 17, // optional  dyadic call (FN2C but checks for · at 𝕩 & 𝕨)
  CHKV = 18, // throw error if top of stack is ·
  TR3O = 19, // TR3D but creates an atop if F is ·
  OP2H = 20, // derive 2-modifier to 1-modifier ⟨…,g,_m_⟩ → (_m_ g)
  LOCO = 21, // N0,N1; push variable at depth N0 and position N1
  LOCM = 22, // N0,N1; push mutable variable at depth N0 and position N1
  VFYM = 23, // push a mutable version of ToS that fails if set to a non-equal value (for header assignment)
  SETH = 24, // set header; acts like SETN, but it doesn't push to stack, and, instead of erroring in cases it would, it skips to the next body
  RETN = 25, // returns top of stack
  FLDO = 26, // N; get field nameList[N] from ToS
  FLDM = 27, // N; get mutable field nameList[N] from ToS
  NSPM = 28, // N; replace ToS with one with a namespace field alias N
  RETD = 29, // return a namespace of exported items
  SYSV = 30, // N; get system function N
  LOCU = 31, // N0,N1; like LOCO but overrides the slot with bi_optOut
  BC_SIZE = 32
};

#define FOR_BC(F) F(PUSH) F(VARO) F(VARM) F(ARRO) F(ARRM) F(FN1C) F(FN2C) F(OP1D) F(OP2D) F(TR2D) \
                  F(TR3D) F(SETN) F(SETU) F(SETM) F(POPS) F(DFND) F(FN1O) F(FN2O) F(CHKV) F(TR3O) \
                  F(OP2H) F(LOCO) F(LOCM) F(VFYM) F(SETH) F(RETN) F(FLDO) F(FLDM) F(NSPM) F(RETD) F(SYSV) F(LOCU)

i32* nextBC(i32* p) {
  switch(*p) {
    case FN1C: case FN2C: case FN1O: case FN2O:
    case OP1D: case OP2D: case OP2H:
    case TR2D: case TR3D: case TR3O:
    case SETN: case SETU: case SETM: case SETH:
    case POPS: case CHKV: case VFYM: case RETN: case RETD:
      return p+1;
    case PUSH: case DFND: case ARRO: case ARRM:
    case VARO: case VARM: case FLDO: case FLDM:
    case SYSV: case NSPM:
      return p+2;
    case LOCO: case LOCM: case LOCU:
      return p+3;
    default: return 0;
  }
}
i32 stackDiff(i32* p) {
  switch(*p) {
    case PUSH: case VARO: case VARM: case DFND: case LOCO: case LOCM: case LOCU: case SYSV: return 1;
    case CHKV: case VFYM: case FLDO: case FLDM: case RETD: case NSPM: return 0;
    case FN1C: case OP1D: case TR2D: case SETN: case SETU: case POPS: case FN1O: case OP2H: case SETH: case RETN: return -1;
    case FN2C: case OP2D: case TR3D: case SETM: case FN2O: case TR3O: return -2;
    case ARRO: case ARRM: return 1-p[1];
    default: return 9999999;
  }
}
char* nameBC(i32* p) {
  switch(*p) { default: return "(unknown)";
    #define F(X) case X: return #X;
    FOR_BC(F)
    #undef F
  }
}
void printBC(i32* p) {
  printf("%s", nameBC(p));
  i32* n = nextBC(p);
  p++;
  i64 am = n-p;
  i32 len = 0;
  for (i64 i = 0; i < am; i++) printf(" %d", p[i]);
  while(p!=n) {
    i32 c = *p++;
    i32 pow = 10;
    i32 log = 1;
    while (pow<=c) { pow*=10; log++; }
    len+= log+1;
  }
  len = 6-len;
  while(len-->0) printf(" ");
}



Block* compile(B bcq, B objs, B blocksq, B indices, B tokenInfo, B src) { // consumes all
  HArr* blocksH = toHArr(blocksq);
  usz bam = blocksH->ia;
  
  // B* objPtr = harr_ptr(objs); usz objIA = a(objs)->ia;
  // for (usz i = 0; i < objIA; i++) objPtr[i] = c2(bi_fill, c1(bi_pick, inc(objPtr[i])), objPtr[i]);
  
  I32Arr* bca = toI32Arr(bcq);
  i32* bc = bca->a;
  usz bcl = bca->ia;
  Comp* comp = mm_allocN(fsizeof(Comp,blocks,Block*,bam), t_comp);
  comp->bc = tag(bca, ARR_TAG);
  comp->indices = indices;
  comp->src = src;
  comp->objs = toHArr(objs);
  comp->blockAm = bam;
  B* blockDefs = blocksH->a;
  B nameList;
  if (isNothing(tokenInfo)) {
    nameList = bi_emptyHVec;
  } else {
    B t = TI(tokenInfo).getU(tokenInfo,2);
    nameList = TI(t).getU(t,0);
  }
  if (!isNothing(src) && !isNothing(indices)) {
    if (isAtm(indices) || rnk(indices)!=1 || a(indices)->ia!=2) thrM("compile: bad indices");
    for (i32 i = 0; i < 2; i++) {
      B ind = TI(indices).getU(indices,i);
      if (isAtm(ind) || rnk(ind)!=1 || a(ind)->ia!=bcl) thrM("compile: bad indices");
      BS2B indGetU = TI(ind).getU;
      for (usz j = 0; j < bcl; j++) o2i(indGetU(ind,j));
    }
  }
  
  for (usz i = 0; i < bam; i++) {
    B cbld = blockDefs[i];
    usz cbia = a(cbld)->ia;
    if (cbia!=4 && cbia!=6) thrM("bad compile block");
    BS2B bgetU = TI(cbld).getU;
    usz  ty  = o2s(bgetU(cbld,0)); if (ty>2) thrM("bad block type");
    bool imm = o2s(bgetU(cbld,1)); // todo o2b or something
    usz  idx = o2s(bgetU(cbld,2)); if (idx>=bcl) thrM("oob bytecode index");
    usz  vam = o2s(bgetU(cbld,3)); if (vam!=(u16)vam) thrM("too many variables");
    i32* cbc = bc+idx;
    i32* scan = cbc;
    i32 ssz = 0, mssz=0;
    i32 mpsc = 0;
    while (true) {
      if (*scan==RETN) { if(ssz!=1)thrM("Wrong stack size before RETN"); break; }
      if (*scan==RETD) { if(ssz!=1&ssz!=0)thrM("Wrong stack size before RETN"); break; }
      ssz+= stackDiff(scan);
      if (ssz>mssz) mssz = ssz;
      if (*scan==LOCO | *scan==LOCM | *scan==LOCU) {
        if (scan[1]>mpsc) mpsc = scan[1];
      }
      scan = nextBC(scan);
      if (scan-bc >= bcl) thrM("no RETN/RETD found at end of bytecode");
    }
    if (mpsc>U16_MAX) thrM("LOC_ too deep");
    
    Body* body = mm_allocN(fsizeof(Body,varIDs,i32,vam), t_body);
    body->comp = comp;
    body->bc = cbc;
    body->maxStack = mssz;
    body->maxPSC = (u16)mpsc;
    body->endStack = ssz;
    body->varAm = (u16)vam;
    if (cbia==4) {
      body->nsDesc = NULL;
      for (i32 i = 0; i < vam; i++) body->varIDs[i] = -1;
    } else m_nsDesc(body, imm, ty, inc(nameList), bgetU(cbld,4), bgetU(cbld,5));
    ptr_inc(comp);
    
    Block* bl = mm_allocN(sizeof(Block), t_block);
    bl->body = body;
    bl->imm = imm;
    bl->ty = (u8)ty;
    comp->blocks[i] = bl;
  }
  
  Block* ret = c(Block,inc(tag(comp->blocks[0], OBJ_TAG)));
  // TODO store blocks in each body, then decrement each of comp->blocks; also then move temp block list out of Comp as it's useless then
  // for (usz i = 0; i < bam; i++) ptr_dec(comp->blocks[i]);
  ptr_dec(comp);
  ptr_dec(blocksH);
  dec(tokenInfo);
  return ret;
}



typedef struct FldAlias {
  struct Value;
  B obj;
  i32 p;
} FldAlias;
void v_set(Scope* pscs[], B s, B x, bool upd) { // doesn't consume
  if (isVar(s)) {
    Scope* sc = pscs[(u16)(s.u>>32)];
    B prev = sc->vars[(u32)s.u];
    if (upd) {
      if (prev.u==bi_noVar.u) thrM("↩: Updating undefined variable");
      dec(prev);
    } else {
      if (prev.u!=bi_noVar.u) thrM("←: Redefining variable");
    }
    sc->vars[(u32)s.u] = inc(x);
  } else {
    VT(s, t_harr);
    B* sp = harr_ptr(s);
    usz ia = a(s)->ia;
    if (isAtm(x) || !eqShape(s, x)) {
      if (isNsp(x)) {
        for (u64 i = 0; i < ia; i++) {
          B c = sp[i];
          if (isVar(c)) {
            Scope* sc = pscs[(u16)(c.u>>32)];
            i32 nameID = sc->body->varIDs[(u32)c.u];
            v_set(pscs, c, ns_getU(x, sc->body->nsDesc->nameList, nameID), upd);
          } else if (isObj(c)) {
            assert(v(c)->type == t_fldAlias);
            Scope* sc = pscs[0];
            FldAlias* cf = c(FldAlias,c);
            v_set(pscs, cf->obj, ns_getU(x, sc->body->nsDesc->nameList, cf->p), upd);
          } else thrM("Assignment: extracting non-name from namespace");
        }
        return;
      }
      thrM("Assignment: Mismatched shape for spread assignment");
    }
    BS2B xgetU = TI(x).getU;
    for (u64 i = 0; i < ia; i++) v_set(pscs, sp[i], xgetU(x,i), upd);
  }
}
B v_get(Scope* pscs[], B s) { // get value representing s, replacing with bi_optOut; doesn't consume
  if (isVar(s)) {
    Scope* sc = pscs[(u16)(s.u>>32)];
    B r = sc->vars[(u32)s.u];
    sc->vars[(u32)s.u] = bi_optOut;
    return r;
  } else {
    VT(s, t_harr);
    usz ia = a(s)->ia;
    B* sp = harr_ptr(s);
    HArr_p r = m_harrUv(ia);
    for (u64 i = 0; i < ia; i++) r.a[i] = v_get(pscs, sp[i]);
    return r.b;
  }
}



// all don't consume anything
B m_funBlock(Block* bl, Scope* psc); // may return evaluated result, whatever
B m_md1Block(Block* bl, Scope* psc);
B m_md2Block(Block* bl, Scope* psc);
#ifdef DEBUG_VM
i32 bcDepth=-2;
i32* vmStack;
i32 bcCtr = 0;
#endif




B* gStack; // points to after end
B* gStackStart;
B* gStackEnd;
void gsReserve(u64 am) {
  #ifdef GS_REALLOC
    if (am>gStackEnd-gStack) {
      u64 n = gStackEnd-gStackStart + am + 500;
      u64 d = gStack-gStackStart;
      gStackStart = realloc(gStackStart, n*sizeof(B));
      gStack    = gStackStart+d;
      gStackEnd = gStackStart+n;
    }
  #elif DEBUG
    if (am>gStackEnd-gStack) thrM("Stack overflow");
  #endif
}
#ifdef GS_REALLOC
NOINLINE
#endif
void gsReserveR(u64 am) { gsReserve(am); }
void gsAdd(B x) {
  #ifdef GS_REALLOC
    if (gStack==gStackEnd) gsReserveR(1);
  #else
    if (gStack==gStackEnd) thrM("Stack overflow");
  #endif
  *(gStack++) = x;
}
B gsPop() {
  return *--gStack;
}
void gsPrint() {
  B* c = gStackStart;
  i32 i = 0;
  while (c!=gStack) {
    printf("%d: ", i);
    print(*c);
    printf(", refc=%d\n", v(*c)->refc);
    c++;
    i++;
  }
}



typedef struct Env {
  Scope* sc;
  union { i32** bcP; i32* bcL; i32 bcV; };
} Env;

Env* envCurr;
Env* envStart;
Env* envEnd;

static inline void pushEnv(Scope* sc, i32** bc) {
  if (envCurr==envEnd) thrM("Stack overflow");
  envCurr->sc = sc;
  #if VM_POS
  envCurr->bcP = bc;
  #else
  envCurr->bcL = *bc;
  #endif
  envCurr++;
}
static inline void popEnv() {
  assert(envCurr>envStart);
  envCurr--;
}

B evalBC(Body* b, Scope* sc) { // doesn't consume
  #ifdef DEBUG_VM
    bcDepth+= 2;
    if (!vmStack) vmStack = malloc(400);
    i32 stackNum = bcDepth>>1;
    vmStack[stackNum] = -1;
    printf("new eval\n");
  #endif
  B* objs = b->comp->objs->a;
  Block** blocks = b->comp->blocks;
  i32* bc = b->bc;
  pushEnv(sc, &bc);
  gsReserve(b->maxStack);
  Scope* pscs[b->maxPSC+1];
  pscs[0] = sc;
  for (i32 i = 0; i < b->maxPSC; i++) pscs[i+1] = pscs[i]->psc;
  #ifdef GS_REALLOC
    #define POP (*--gStack)
    #define P(N) B N=POP;
    #define ADD(X) { B tr=X; *(gStack++) = tr; }
    #define GS_UPD
  #else
    B* lgStack = gStack;
    #define POP (*--lgStack)
    #define P(N) B N=POP;
    #define ADD(X) { *(lgStack++) = X; } // fine, as, if an error occurs, lgStack is ignored anyways
    #define GS_UPD { gStack = lgStack; }
  #endif
  
  while(true) {
    #ifdef DEBUG_VM
      i32* sbc = bc;
      i32 bcPos = sbc-c(I32Arr,b->comp->bc)->a;
      vmStack[stackNum] = bcPos;
      for(i32 i = 0; i < bcDepth; i++) printf(" ");
      printBC(sbc); printf("@%d << ", bcPos);
      for (i32 i = 0; i < b->maxStack; i++) { if(i)printf(" ⋄ "); print(gStack[i]); } puts(""); fflush(stdout);
      bcCtr++;
      for (i32 i = 0; i < sc->varAm; i++) VALIDATE(sc->vars[i]);
    #endif
    switch(*bc++) {
      case POPS: dec(POP); break;
      case PUSH: {
        ADD(inc(objs[*bc++]));
        break;
      }
      case FN1C: { P(f)P(x)
        GS_UPD;
        ADD(c1(f, x); dec(f));
        break;
      }
      case FN1O: { P(f)P(x)
        GS_UPD;
        ADD(isNothing(x)? x : c1(f, x)); dec(f);
        break;
      }
      case FN2C: { P(w)P(f)P(x)
        GS_UPD;
        ADD(c2(f, w, x); dec(f));
        break;
      }
      case FN2O: { P(w)P(f)P(x)
        GS_UPD;
        if (isNothing(x)) { dec(w); ADD(x); }
        else ADD(isNothing(w)? c1(f, x) : c2(f, w, x));
        dec(f);
        break;
      }
      case ARRO: case ARRM: {
        i32 sz = *bc++;
        if (sz==0) {
          ADD(inc(bi_emptyHVec));
        } else {
          HArr_p r = m_harrUv(sz);
          bool allNum = true;
          for (i32 i = 0; i < sz; i++) if (!isNum(r.a[sz-i-1] = POP)) allNum = false;
          if (allNum) {
            GS_UPD;
            ADD(withFill(r.b, m_f64(0)));
          } else ADD(r.b);
        }
        break;
      }
      case DFND: {
        GS_UPD;
        Block* bl = blocks[*bc++];
        switch(bl->ty) { default: UD;
          case 0: ADD(m_funBlock(bl, sc)); break;
          case 1: ADD(m_md1Block(bl, sc)); break;
          case 2: ADD(m_md2Block(bl, sc)); break;
        }
        break;
      }
      case OP1D: { P(f)P(m)     GS_UPD; ADD(m1_d  (m,f  )); break; }
      case OP2D: { P(f)P(m)P(g) GS_UPD; ADD(m2_d  (m,f,g)); break; }
      case OP2H: {     P(m)P(g)         ADD(m2_h  (m,  g)); break; }
      case TR2D: {     P(g)P(h)         ADD(m_atop(  g,h)); break; }
      case TR3D: { P(f)P(g)P(h)         ADD(m_fork(f,g,h)); break; }
      case TR3O: { P(f)P(g)P(h)
        if (isNothing(f)) { ADD(m_atop(g,h)); dec(f); }
        else ADD(m_fork(f,g,h));
        break;
      }
      case LOCM: { i32 d = *bc++; i32 p = *bc++;
        ADD(tag((u64)d<<32 | (u32)p, VAR_TAG));
        break;
      }
      case LOCO: { i32 d = *bc++; i32 p = *bc++;
        B l = pscs[d]->vars[p];
        if(l.u==bi_noVar.u) thrM("Reading variable before its defined");
        ADD(inc(l));
        break;
      }
      case LOCU: { i32 d = *bc++; i32 p = *bc++;
        B* vars = pscs[d]->vars;
        ADD(vars[p]);
        vars[p] = bi_optOut;
        break;
      }
      case SETN: { P(s)    P(x) GS_UPD; v_set(pscs, s, x, false); dec(s); ADD(x); break; }
      case SETU: { P(s)    P(x) GS_UPD; v_set(pscs, s, x, true ); dec(s); ADD(x); break; }
      case SETM: { P(s)P(f)P(x) GS_UPD;
        B w = v_get(pscs, s);
        B r = c2(f,w,x); dec(f);
        v_set(pscs, s, r, true); dec(s);
        ADD(r);
        break;
      }
      case FLDO: { P(ns) GS_UPD; i32 p = *bc++;
        if (!isNsp(ns)) thrM("Trying to read a field from non-namespace");
        ADD(inc(ns_getU(ns, sc->body->nsDesc->nameList, p)));
        dec(ns);
        break;
      }
      case RETD: {
        if (b->endStack) dec(POP);
        ptr_inc(sc);
        ptr_inc(b->nsDesc);
        ADD(m_ns(sc, b->nsDesc));
        goto end;
      }
      case NSPM: { P(o) i32 l = *bc++;
        B a = mm_alloc(sizeof(FldAlias), t_fldAlias, ftag(OBJ_TAG));
        c(FldAlias,a)->obj = o;
        c(FldAlias,a)->p = l;
        ADD(a);
        break;
      }
      case RETN: goto end;
      // not implemented: VARO VARM CHKV VFYM SETH FLDO FLDM NSPM SYSV
      default:
        #ifdef DEBUG
          printf("todo %d\n", bc[-1]); bc++; break;
        #else
          UD;
        #endif
    }
    #ifdef DEBUG_VM
      for(i32 i = 0; i < bcDepth; i++) printf(" ");
      printBC(sbc); printf("@%ld:   ", sbc-c(I32Arr,b->comp->bc)->a);
      for (i32 i = 0; i < b->maxStack; i++) { if(i)printf(" ⋄ "); print(gStack[i]); } puts(""); fflush(stdout);
    #endif
  }
  end:;
  #ifdef DEBUG_VM
    bcDepth-= 2;
  #endif
  B r = POP;
  GS_UPD;
  popEnv();
  return r;
  #undef P
  #undef ADD
  #undef POP
  #undef GS_UPD
}

B actualExec(Block* bl, Scope* psc, i32 ga, B* svar) { // consumes svar contents
  Body* body = bl->body;
  Scope* sc = mm_allocN(fsizeof(Scope, vars, B, body->varAm), t_scope);
  sc->body = body; ptr_inc(body);
  sc->psc = psc; if(psc) ptr_inc(psc);
  u16 varAm = sc->varAm = body->varAm;
  assert(varAm>=ga);
  i32 i = 0;
  while (i<ga) { sc->vars[i] = svar[i]; i++; }
  while (i<varAm) sc->vars[i++] = bi_noVar;
  B r = evalBC(body, sc);
  ptr_dec(sc);
  return r;
}

B funBl_c1(B t,      B x) {                    FunBlock* b=c(FunBlock, t    ); return actualExec(b->bl, b->sc, 3, (B[]){inc(t), x, bi_N                                  }); }
B funBl_c2(B t, B w, B x) {                    FunBlock* b=c(FunBlock, t    ); return actualExec(b->bl, b->sc, 3, (B[]){inc(t), x, w                                     }); }
B md1Bl_c1(B D,      B x) { Md1D* d=c(Md1D,D); Md1Block* b=c(Md1Block, d->m1); return actualExec(b->bl, b->sc, 5, (B[]){inc(D), x, bi_N, inc(d->m1), inc(d->f)           }); }
B md1Bl_c2(B D, B w, B x) { Md1D* d=c(Md1D,D); Md1Block* b=c(Md1Block, d->m1); return actualExec(b->bl, b->sc, 5, (B[]){inc(D), x, w   , inc(d->m1), inc(d->f)           }); }
B md2Bl_c1(B D,      B x) { Md2D* d=c(Md2D,D); Md2Block* b=c(Md2Block, d->m2); return actualExec(b->bl, b->sc, 6, (B[]){inc(D), x, bi_N, inc(d->m2), inc(d->f), inc(d->g)}); }
B md2Bl_c2(B D, B w, B x) { Md2D* d=c(Md2D,D); Md2Block* b=c(Md2Block, d->m2); return actualExec(b->bl, b->sc, 6, (B[]){inc(D), x, w   , inc(d->m2), inc(d->f), inc(d->g)}); }
B m_funBlock(Block* bl, Scope* psc) { // doesn't consume anything
  if (bl->imm) return actualExec(bl, psc, 0, NULL);
  B r = mm_alloc(sizeof(FunBlock), t_fun_block, ftag(FUN_TAG));
  c(FunBlock, r)->bl = bl; ptr_inc(bl);
  c(FunBlock, r)->sc = psc; ptr_inc(psc);
  c(FunBlock, r)->c1 = funBl_c1;
  c(FunBlock, r)->c2 = funBl_c2;
  return r;
}
B m_md1Block(Block* bl, Scope* psc) {
  B r = mm_alloc(sizeof(Md1Block), t_md1_block, ftag(MD1_TAG));
  c(Md1Block, r)->bl = bl; ptr_inc(bl);
  c(Md1Block, r)->sc = psc; ptr_inc(psc);
  c(Md1Block, r)->c1 = md1Bl_c1;
  c(Md1Block, r)->c2 = md1Bl_c2;
  return r;
}
B m_md2Block(Block* bl, Scope* psc) {
  B r = mm_alloc(sizeof(Md2Block), t_md2_block, ftag(MD2_TAG));
  c(Md2Block, r)->bl = bl; ptr_inc(bl);
  c(Md2Block, r)->sc = psc; ptr_inc(psc);
  c(Md2Block, r)->c1 = md2Bl_c1;
  c(Md2Block, r)->c2 = md2Bl_c2;
  return r;
}

void comp_free(Value* x) {
  Comp* c = (Comp*)x;
  ptr_decR(c->objs); decR(c->bc); decR(c->src); decR(c->indices);
  u32 am = c->blockAm; for(u32 i = 0; i < am; i++) ptr_dec(c->blocks[i]);
}
void scope_free(Value* x) {
  Scope* c = (Scope*)x;
  if (c->psc) ptr_decR(c->psc);
  ptr_decR(c->body);
  u16 am = c->varAm;
  for (u32 i = 0; i < am; i++) dec(c->vars[i]);
}
void  body_free(Value* x) { Body*     c = (Body    *)x; ptr_decR(c->comp); if(c->nsDesc)ptr_decR(c->nsDesc); }
void block_free(Value* x) { Block*    c = (Block   *)x; ptr_decR(c->body); }
void funBl_free(Value* x) { FunBlock* c = (FunBlock*)x; ptr_decR(c->sc); ptr_decR(c->bl); }
void md1Bl_free(Value* x) { Md1Block* c = (Md1Block*)x; ptr_decR(c->sc); ptr_decR(c->bl); }
void md2Bl_free(Value* x) { Md2Block* c = (Md2Block*)x; ptr_decR(c->sc); ptr_decR(c->bl); }
void alias_free(Value* x) { dec(((FldAlias*)x)->obj); }

void comp_visit(Value* x) {
  Comp* c = (Comp*)x;
  mm_visitP(c->objs); mm_visit(c->bc); mm_visit(c->src); mm_visit(c->indices);
  u32 am = c->blockAm; for(u32 i = 0; i < am; i++) mm_visitP(c->blocks[i]);
}
void scope_visit(Value* x) {
  Scope* c = (Scope*)x;
  if (c->psc) mm_visitP(c->psc);
  mm_visitP(c->body);
  u16 am = c->varAm;
  for (u32 i = 0; i < am; i++) mm_visit(c->vars[i]);
}
void  body_visit(Value* x) { Body*     c = (Body    *)x; mm_visitP(c->comp); if(c->nsDesc)mm_visitP(c->nsDesc); }
void block_visit(Value* x) { Block*    c = (Block   *)x; mm_visitP(c->body); }
void funBl_visit(Value* x) { FunBlock* c = (FunBlock*)x; mm_visitP(c->sc); mm_visitP(c->bl); }
void md1Bl_visit(Value* x) { Md1Block* c = (Md1Block*)x; mm_visitP(c->sc); mm_visitP(c->bl); }
void md2Bl_visit(Value* x) { Md2Block* c = (Md2Block*)x; mm_visitP(c->sc); mm_visitP(c->bl); }
void alias_visit(Value* x) { mm_visit(((FldAlias*)x)->obj); }

void comp_print (B x) { printf("(%p: comp)",v(x)); }
void body_print (B x) { printf("(%p: body varam=%d)",v(x),c(Body,x)->varAm); }
void block_print(B x) { printf("(%p: block for %p)",v(x),c(Block,x)->body); }
void scope_print(B x) { printf("(%p: scope; vars:",v(x));Scope*sc=c(Scope,x);for(u64 i=0;i<sc->varAm;i++){printf(" ");print(sc->vars[i]);}printf(")"); }
void alias_print(B x) { printf("(alias %d of ", c(FldAlias,x)->p); print(c(FldAlias,x)->obj); printf(")"); }

// void funBl_print(B x) { printf("(%p: function"" block bl=%p sc=%p)",v(x),c(FunBlock,x)->bl,c(FunBlock,x)->sc); }
// void md1Bl_print(B x) { printf("(%p: 1-modifier block bl=%p sc=%p)",v(x),c(Md1Block,x)->bl,c(Md1Block,x)->sc); }
// void md2Bl_print(B x) { printf("(%p: 2-modifier block bl=%p sc=%p)",v(x),c(Md2Block,x)->bl,c(Md2Block,x)->sc); }
void funBl_print(B x) { printf("(function"" block @%ld)",c(FunBlock,x)->bl->body->bc-c(I32Arr,c(FunBlock,x)->bl->body->comp->bc)->a); }
void md1Bl_print(B x) { printf("(1-modifier block @%ld)",c(Md1Block,x)->bl->body->bc-c(I32Arr,c(Md1Block,x)->bl->body->comp->bc)->a); }
void md2Bl_print(B x) { printf("(2-modifier block @%ld)",c(Md2Block,x)->bl->body->bc-c(I32Arr,c(Md2Block,x)->bl->body->comp->bc)->a); }
// void funBl_print(B x) { printf("{function""}"); }
// void md1Bl_print(B x) { printf("{1-modifier}"); }
// void md2Bl_print(B x) { printf("{2-modifier}"); }

B block_decompose(B x) { return m_v2(m_i32(1), x); }

B bl_m1d(B m, B f     ) { Md1Block* c = c(Md1Block,m); return c->bl->imm? actualExec(c(Md1Block, m)->bl, c(Md1Block, m)->sc, 2, (B[]){m, f   }) : m_md1D(m,f  ); }
B bl_m2d(B m, B f, B g) { Md2Block* c = c(Md2Block,m); return c->bl->imm? actualExec(c(Md2Block, m)->bl, c(Md2Block, m)->sc, 3, (B[]){m, f, g}) : m_md2D(m,f,g); }

void allocStack(void** curr, void** start, void** end, i32 elSize, i32 count) {
  usz pageSize = sysconf(_SC_PAGESIZE);
  u64 sz = (elSize*count + pageSize-1)/pageSize * pageSize;
  assert(sz%elSize == 0);
  *curr = *start = mmap(NULL, sz+pageSize, PROT_READ|PROT_WRITE, MAP_NORESERVE|MAP_PRIVATE|MAP_ANON, -1, 0);
  *end = ((char*)*start)+sz;
  mprotect(*end, pageSize, PROT_NONE); // idk first way i found to force erroring on overflow
}

static inline void comp_init() {
  ti[t_comp     ].free = comp_free;  ti[t_comp     ].visit = comp_visit;  ti[t_comp     ].print =  comp_print;
  ti[t_body     ].free = body_free;  ti[t_body     ].visit = body_visit;  ti[t_body     ].print =  body_print;
  ti[t_block    ].free = block_free; ti[t_block    ].visit = block_visit; ti[t_block    ].print = block_print;
  ti[t_scope    ].free = scope_free; ti[t_scope    ].visit = scope_visit; ti[t_scope    ].print = scope_print;
  ti[t_fldAlias ].free = alias_free; ti[t_fldAlias ].visit = alias_visit; ti[t_fldAlias ].print = alias_print;
  ti[t_fun_block].free = funBl_free; ti[t_fun_block].visit = funBl_visit; ti[t_fun_block].print = funBl_print; ti[t_fun_block].decompose = block_decompose;
  ti[t_md1_block].free = md1Bl_free; ti[t_md1_block].visit = md1Bl_visit; ti[t_md1_block].print = md1Bl_print; ti[t_md1_block].decompose = block_decompose; ti[t_md1_block].m1_d=bl_m1d;
  ti[t_md2_block].free = md2Bl_free; ti[t_md2_block].visit = md2Bl_visit; ti[t_md2_block].print = md2Bl_print; ti[t_md2_block].decompose = block_decompose; ti[t_md2_block].m2_d=bl_m2d;
  #ifndef GS_REALLOC
    allocStack((void**)&gStack, (void**)&gStackStart, (void**)&gStackEnd, sizeof(B), GS_SIZE);
  #endif
  allocStack((void**)&envCurr, (void**)&envStart, (void**)&envEnd, sizeof(Env), ENV_SIZE);
}

void print_vmStack() {
  #ifdef DEBUG_VM
    printf("vm stack:");
    for (i32 i = 0; i < (bcDepth>>1) + 1; i++) { printf(" %d", vmStack[i]); fflush(stdout); }
    printf("\n"); fflush(stdout);
  #endif
}



typedef struct CatchFrame {
  jmp_buf jmp;
  u64 gsDepth;
  u64 envDepth;
  u64 cfDepth;
} CatchFrame;
CatchFrame* cf; // points to after end
CatchFrame* cfStart;
CatchFrame* cfEnd;

jmp_buf* prepareCatch() { // in the case of returning false, must call popCatch();
  if (cf==cfEnd) {
    u64 n = cfEnd-cfStart;
    n = n<8? 8 : n*2;
    u64 d = cf-cfStart;
    cfStart = realloc(cfStart, n*sizeof(CatchFrame));
    cf    = cfStart+d;
    cfEnd = cfStart+n;
  }
  cf->cfDepth = cf-cfStart;
  cf->gsDepth = gStack-gStackStart;
  cf->envDepth = envCurr-envStart;
  return &(cf++)->jmp;
}
void popCatch() {
  #ifdef CATCH_ERRORS
    assert(cf>cfStart);
    cf--;
  #endif
}

NOINLINE NORETURN void thr(B msg) {
  if (cf>cfStart) {
    catchMessage = msg;
    cf--;
    
    B* gStackNew = gStackStart + cf->gsDepth;
    assert(gStackNew<=gStack);
    while (gStack!=gStackNew) dec(*--gStack);
    envPrevHeight = envCurr-envStart;
    Env* envNew = envStart + cf->envDepth;
    assert(envNew<=envCurr);
    while (envCurr!=envNew) {
      envCurr--;
      #if VM_POS
        envCurr->bcV = *envCurr->bcP - i32arr_ptr(envCurr->sc->body->comp->bc) - 1;
      #else
        envCurr->bcV = envCurr->bcL - i32arr_ptr(envCurr->sc->body->comp->bc);
      #endif
    }
    
    if (cfStart+cf->cfDepth > cf) err("bad catch cfDepth");
    cf = cfStart+cf->cfDepth;
    longjmp(cf->jmp, 1);
  }
  assert(cf==cfStart);
  printf("Error: ");
  print(msg);
  puts("");
  #ifdef DEBUG
  __builtin_trap();
  #else
  exit(1);
  #endif
}

NOINLINE NORETURN void thrM(char* s) {
  thr(fromUTF8(s, strlen(s)));
}
NOINLINE NORETURN void thrOOM() { thrM("Out of memory"); }


NOINLINE void vm_pst(Env* s, Env* e) {
  assert(s<=e);
  i64 l = e-s;
  i64 i = l-1;
  while (i>=0) {
    Env* c = s+i;
    if (l>30 && i==l-10) {
      printf("(%ld entries omitted)\n", l-20);
      i = 10;
    }
    i32 bcPos = c->bcV;
    Comp* comp = c->sc->body->comp;
    B src = comp->src;
    if (!isNothing(src) && !isNothing(comp->indices)) {
      B inds = TI(comp->indices).getU(comp->indices, 0); usz cs = o2s(TI(inds).getU(inds,bcPos));
      B inde = TI(comp->indices).getU(comp->indices, 1); usz ce = o2s(TI(inde).getU(inde,bcPos))+1;
      int start = printf("%ld: ", i);
      usz srcL = a(src)->ia;
      BS2B srcGetU = TI(src).getU;
      usz srcS = cs;
      while (srcS>0 && o2cu(srcGetU(src,srcS-1))!='\n') srcS--;
      usz srcE = srcS;
      while (srcE<srcL) { u32 chr = o2cu(srcGetU(src, srcE)); if(chr=='\n')break; printUTF8(chr); srcE++; }
      if (ce>srcE) ce = srcE;
      cs-= srcS;
      ce-= srcS;
      putchar('\n');
      for (i32 i = 0; i < cs+start; i++) putchar(' ');
      for (i32 i = cs; i < ce; i++) putchar('^');
      putchar('\n');
      // printf("  inds:%d…%d\n", cinds, cinde);
    } else {
      printf("%ld: source unknown\n", i);
    }
    i--;
  }
}
