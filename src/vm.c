#include <unistd.h>
#include "core.h"
#include "vm.h"
#include "ns.h"
#include "utils/utf.h"

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


B catchMessage;
u64 envPrevHeight;

Env* envCurr;
Env* envStart;
Env* envEnd;

B* gStack; // points to after end
B* gStackStart;
B* gStackEnd;
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

Block* compileBlock(B block, Comp* comp, bool* bDone, i32* bc, usz bcIA, B blocks, B nameList, Scope* sc, i32 depth) {
  usz cIA = a(block)->ia;
  if (cIA!=4 && cIA!=6) thrM("VM compiler: Bad block info size");
  BS2B bgetU = TI(block).getU;
  usz  ty  = o2s(bgetU(block,0)); if (ty>2) thrM("VM compiler: Bad type");
  bool imm = o2b(bgetU(block,1));
  usz  idx = o2s(bgetU(block,2)); if (idx>=bcIA) thrM("VM compiler: Bytecode index out of bounds");
  usz  vam = o2s(bgetU(block,3)); if (vam!=(u16)vam) thrM("VM compiler: >2⋆16 variables not supported"); // TODO any reason for this? 2⋆32 vars should just work, no?
  i32 h = 0; // stack height
  i32 hM = 0; // max stack height
  i32 mpsc = 0;
  TSALLOC(Block*, nBlT, 2);
  TSALLOC(i32, nBCT, 20);
  TSALLOC(i32, mapT, 20);
  i32* c = bc+idx;
  while (true) {
    i32* n = nextBC(c);
    if (n-bc-1 >= bcIA) thrM("VM compiler: No RETN/RETD found before end of bytecode");
    bool ret = false;
    switch (*c) {
      case RETN: if(h!=1) thrM("VM compiler: Wrong stack size before RETN");
        TSADD(nBCT, RETN);
        ret = true;
        break;
      case RETD: if(h!=1&h!=0) thrM("VM compiler: Wrong stack size before RETD");
        if (h==1) TSADD(nBCT, POPS);
        TSADD(nBCT, RETD);
        ret = true;
        break;
      case DFND: {
        u32 id = c[1];
        if ((u32)id >= a(blocks)->ia) thrM("VM compiler: DFND index out-of-bounds");
        if (bDone[id]) thrM("VM compiler: DFND of the same block in multiple places");
        bDone[id] = true;
        TSADD(nBCT, DFND);
        TSADD(nBCT, TSSIZE(nBlT));
        TSADD(nBlT, compileBlock(TI(blocks).getU(blocks,id), comp, bDone, bc, bcIA, blocks, nameList, sc, depth+1));
        break;
      }
      case LOCO: case LOCM: case LOCU: {
        if (c[1]>mpsc) mpsc = c[1];
        TSADD(nBCT, c[0]);
        TSADD(nBCT, c[1]);
        TSADD(nBCT, c[2]);
        break;
      }
      default: {
        i32* ccpy = c;
        while (ccpy!=n) TSADD(nBCT, *ccpy++);
        break;
      }
    }
    usz nlen = TSSIZE(nBCT)-TSSIZE(mapT);
    for (usz i = 0; i < nlen; i++) TSADD(mapT, c-bc);
    h+= stackDiff(c);
    if (h<0) thrM("VM compiler: Stack size goes negative");
    if (h>hM) hM = h;
    if (ret) break;
    c = n;
  }
  
  if (mpsc>U16_MAX) thrM("VM compiler: LOC_ too deep");
  
  usz blC = TSSIZE(nBlT);
  BlBlocks* nBl = mm_allocN(fsizeof(BlBlocks,a,Block*,blC), t_blBlocks);
  nBl->am = blC;
  memcpy(nBl->a, nBlT, blC*sizeof(Block*));
  TSFREE(nBlT);
  
  usz nbcC = TSSIZE(nBCT); i32* nbc; m_i32arrv(&nbc, nbcC); memcpy(nbc, nBCT, nbcC*4); TSFREE(nBCT);
  usz mapC = TSSIZE(mapT); i32* map; m_i32arrv(&map, mapC); memcpy(map, mapT, mapC*4); TSFREE(mapT);
  
  Body* body = mm_allocN(fsizeof(Body,varIDs,i32,vam), t_body);
  body->comp = comp; ptr_inc(comp);
  body->bc = nbc;
  body->map = map;
  body->blocks = nBl;
  body->maxStack = hM;
  body->maxPSC = (u16)mpsc;
  body->varAm = (u16)vam;
  if (cIA==4) {
    body->nsDesc = NULL;
    for (i32 i = 0; i < vam; i++) body->varIDs[i] = -1;
  } else m_nsDesc(body, imm, ty, inc(nameList), bgetU(block,4), bgetU(block,5));
  
  Block* bl = mm_allocN(sizeof(Block), t_block);
  bl->body = body;
  bl->imm = imm;
  bl->ty = (u8)ty;
  return bl;
}

NOINLINE Block* compile(B bcq, B objs, B blocks, B indices, B tokenInfo, B src, Scope* sc) { // consumes all; assumes arguments are valid (verifies some stuff, but definitely not everything)
  usz bIA = a(blocks)->ia;
  
  I32Arr* bca = toI32Arr(bcq);
  i32* bc = bca->a;
  usz bcIA = bca->ia;
  Comp* comp = mm_allocN(sizeof(Comp), t_comp);
  comp->bc = tag(bca, ARR_TAG);
  comp->indices = indices;
  comp->src = src;
  comp->objs = toHArr(objs);
  comp->blockAm = 0;
  B nameList;
  if (isNothing(tokenInfo)) {
    nameList = bi_emptyHVec;
  } else {
    B t = TI(tokenInfo).getU(tokenInfo,2);
    nameList = TI(t).getU(t,0);
  }
  if (!isNothing(src) && !isNothing(indices)) {
    if (isAtm(indices) || rnk(indices)!=1 || a(indices)->ia!=2) thrM("VM compiler: Bad indices");
    for (i32 i = 0; i < 2; i++) {
      B ind = TI(indices).getU(indices,i);
      if (isAtm(ind) || rnk(ind)!=1 || a(ind)->ia!=bcIA) thrM("VM compiler: Bad indices");
      BS2B indGetU = TI(ind).getU;
      for (usz j = 0; j < bcIA; j++) o2i(indGetU(ind,j));
    }
  }
  TALLOC(bool,bDone,bIA);
  for (usz i = 0; i < bIA; i++) bDone[i] = false;
  Block* ret = compileBlock(TI(blocks).getU(blocks, 0), comp, bDone, bc, bcIA, blocks, nameList, sc, 0);
  TFREE(bDone);
  ptr_dec(comp); dec(blocks); dec(tokenInfo);
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
    VTY(s, t_harr);
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
    VTY(s, t_harr);
    usz ia = a(s)->ia;
    B* sp = harr_ptr(s);
    HArr_p r = m_harrUv(ia);
    for (u64 i = 0; i < ia; i++) r.a[i] = v_get(pscs, sp[i]);
    return r.b;
  }
}




#ifdef DEBUG_VM
i32 bcDepth=-2;
i32* vmStack;
i32 bcCtr = 0;
#endif
#define BCPOS(B,P) (B->map[P-B->bc])
B evalBC(Body* b, Scope* sc) { // doesn't consume
  #ifdef DEBUG_VM
    bcDepth+= 2;
    if (!vmStack) vmStack = malloc(400);
    i32 stackNum = bcDepth>>1;
    vmStack[stackNum] = -1;
    printf("new eval\n");
  #endif
  B* objs = b->comp->objs->a;
  Block** blocks = b->blocks->a; // b->comp->blocks;
  i32* bc = b->bc;
  pushEnv(sc, bc);
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
  #if VM_POS
    #define POS_UPD (envCurr-1)->bcL = bc-1;
  #else
    #define POS_UPD
  #endif
  
  while(true) {
    #ifdef DEBUG_VM
      i32* sbc = bc;
      i32 bcPos = BCPOS(b,sbc);
      vmStack[stackNum] = bcPos;
      for(i32 i = 0; i < bcDepth; i++) printf(" ");
      printBC(sbc); printf("@%d << ", bcPos);
      for (i32 i = 0; i < b->maxStack; i++) { if(i)printf(" ⋄ "); print(gStack[i]); } putchar('\n'); fflush(stdout);
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
        GS_UPD;POS_UPD;
        ADD(c1(f, x); dec(f));
        break;
      }
      case FN1O: { P(f)P(x)
        GS_UPD;POS_UPD;
        ADD(isNothing(x)? x : c1(f, x)); dec(f);
        break;
      }
      case FN2C: { P(w)P(f)P(x)
        GS_UPD;POS_UPD;
        ADD(c2(f, w, x); dec(f));
        break;
      }
      case FN2O: { P(w)P(f)P(x)
        GS_UPD;POS_UPD;
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
        GS_UPD;POS_UPD;
        Block* bl = blocks[*bc++];
        switch(bl->ty) { default: UD;
          case 0: ADD(m_funBlock(bl, sc)); break;
          case 1: ADD(m_md1Block(bl, sc)); break;
          case 2: ADD(m_md2Block(bl, sc)); break;
        }
        break;
      }
      case OP1D: { P(f)P(m)     GS_UPD;POS_UPD; ADD(m1_d  (m,f  )); break; }
      case OP2D: { P(f)P(m)P(g) GS_UPD;POS_UPD; ADD(m2_d  (m,f,g)); break; }
      case OP2H: {     P(m)P(g)                 ADD(m2_h  (m,  g)); break; }
      case TR2D: {     P(g)P(h)                 ADD(m_atop(  g,h)); break; }
      case TR3D: { P(f)P(g)P(h)                 ADD(m_fork(f,g,h)); break; }
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
        if(l.u==bi_noVar.u) { POS_UPD; thrM("Reading variable before its defined"); }
        ADD(inc(l));
        break;
      }
      case LOCU: { i32 d = *bc++; i32 p = *bc++;
        B* vars = pscs[d]->vars;
        ADD(vars[p]);
        vars[p] = bi_optOut;
        break;
      }
      case SETN: { P(s)    P(x) GS_UPD; POS_UPD; v_set(pscs, s, x, false); dec(s); ADD(x); break; }
      case SETU: { P(s)    P(x) GS_UPD; POS_UPD; v_set(pscs, s, x, true ); dec(s); ADD(x); break; }
      case SETM: { P(s)P(f)P(x) GS_UPD; POS_UPD;
        B w = v_get(pscs, s);
        B r = c2(f,w,x); dec(f);
        v_set(pscs, s, r, true); dec(s);
        ADD(r);
        break;
      }
      case FLDO: { P(ns) GS_UPD; i32 p = *bc++; POS_UPD;
        if (!isNsp(ns)) thrM("Trying to read a field from non-namespace");
        ADD(inc(ns_getU(ns, sc->body->nsDesc->nameList, p)));
        dec(ns);
        break;
      }
      case RETD: {
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
      printBC(sbc); printf("@%ld:   ", BCPOS(b, sbc));
      for (i32 i = 0; i < b->maxStack; i++) { if(i)printf(" ⋄ "); print(gStack[i]); } putchar('\n'); fflush(stdout);
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
  if (sc->refc>1) {
    usz innerRef = 1;
    for (i = 0; i < varAm; i++) {
      B c = sc->vars[i];
      if (isVal(c) && v(c)->refc==1) {
        u8 t = v(c)->type;
        if      (t==t_fun_block && c(FunBlock,c)->sc==sc) innerRef++;
        else if (t==t_md1_block && c(Md1Block,c)->sc==sc) innerRef++;
        else if (t==t_md2_block && c(Md2Block,c)->sc==sc) innerRef++;
      }
    }
    assert(innerRef<=sc->refc);
    if (innerRef==sc->refc) {
      value_free((Value*)sc);
      return r;
    }
  }
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

void scope_free(Value* x) {
  Scope* c = (Scope*)x;
  if (c->psc) ptr_decR(c->psc);
  ptr_decR(c->body);
  u16 am = c->varAm;
  for (u32 i = 0; i < am; i++) dec(c->vars[i]);
}
void  comp_free(Value* x) { Comp*     c = (Comp    *)x; ptr_decR(c->objs); decR(c->bc); decR(c->src); decR(c->indices); }
void  body_free(Value* x) { Body*     c = (Body    *)x; ptr_decR(c->comp); if(c->nsDesc)ptr_decR(c->nsDesc); ptr_decR(c->blocks); ptr_decR(RFLD(c->bc,I32Arr,a)); ptr_decR(RFLD(c->map,I32Arr,a)); }
void block_free(Value* x) { Block*    c = (Block   *)x; ptr_decR(c->body); }
void funBl_free(Value* x) { FunBlock* c = (FunBlock*)x; ptr_decR(c->sc); ptr_decR(c->bl); }
void md1Bl_free(Value* x) { Md1Block* c = (Md1Block*)x; ptr_decR(c->sc); ptr_decR(c->bl); }
void md2Bl_free(Value* x) { Md2Block* c = (Md2Block*)x; ptr_decR(c->sc); ptr_decR(c->bl); }
void alias_free(Value* x) { dec(((FldAlias*)x)->obj); }
void bBlks_free(Value* x) { BlBlocks* c = (BlBlocks*)x; u16 am = c->am; for (u32 i = 0; i < am; i++) ptr_dec(c->a[i]); }

void scope_visit(Value* x) {
  Scope* c = (Scope*)x;
  if (c->psc) mm_visitP(c->psc);
  mm_visitP(c->body);
  u16 am = c->varAm;
  for (u32 i = 0; i < am; i++) mm_visit(c->vars[i]);
}
void  comp_visit(Value* x) { Comp*     c = (Comp    *)x; mm_visitP(c->objs); mm_visit(c->bc); mm_visit(c->src); mm_visit(c->indices); }
void  body_visit(Value* x) { Body*     c = (Body    *)x; mm_visitP(c->comp); if(c->nsDesc)mm_visitP(c->nsDesc); mm_visitP(c->blocks); mm_visitP(RFLD(c->bc,I32Arr,a)); mm_visitP(RFLD(c->map,I32Arr,a)); }
void block_visit(Value* x) { Block*    c = (Block   *)x; mm_visitP(c->body); }
void funBl_visit(Value* x) { FunBlock* c = (FunBlock*)x; mm_visitP(c->sc); mm_visitP(c->bl); }
void md1Bl_visit(Value* x) { Md1Block* c = (Md1Block*)x; mm_visitP(c->sc); mm_visitP(c->bl); }
void md2Bl_visit(Value* x) { Md2Block* c = (Md2Block*)x; mm_visitP(c->sc); mm_visitP(c->bl); }
void alias_visit(Value* x) { mm_visit(((FldAlias*)x)->obj); }
void bBlks_visit(Value* x) { BlBlocks* c = (BlBlocks*)x; u16 am = c->am; for (u32 i = 0; i < am; i++) mm_visitP(c->a[i]); }

void comp_print (B x) { printf("(%p: comp)",v(x)); }
void body_print (B x) { printf("(%p: body varam=%d)",v(x),c(Body,x)->varAm); }
void block_print(B x) { printf("(%p: block for %p)",v(x),c(Block,x)->body); }
void scope_print(B x) { printf("(%p: scope; vars:",v(x));Scope*sc=c(Scope,x);for(u64 i=0;i<sc->varAm;i++){printf(" ");print(sc->vars[i]);}printf(")"); }
void alias_print(B x) { printf("(alias %d of ", c(FldAlias,x)->p); print(c(FldAlias,x)->obj); printf(")"); }
void bBlks_print(B x) { printf("(block list)"); }

// void funBl_print(B x) { printf("(%p: function"" block bl=%p sc=%p)",v(x),c(FunBlock,x)->bl,c(FunBlock,x)->sc); }
// void md1Bl_print(B x) { printf("(%p: 1-modifier block bl=%p sc=%p)",v(x),c(Md1Block,x)->bl,c(Md1Block,x)->sc); }
// void md2Bl_print(B x) { printf("(%p: 2-modifier block bl=%p sc=%p)",v(x),c(Md2Block,x)->bl,c(Md2Block,x)->sc); }
void funBl_print(B x) { printf("(function"" block @%d)",c(FunBlock,x)->bl->body->map[0]); }
void md1Bl_print(B x) { printf("(1-modifier block @%d)",c(Md1Block,x)->bl->body->map[0]); }
void md2Bl_print(B x) { printf("(2-modifier block @%d)",c(Md2Block,x)->bl->body->map[0]); }
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
void print_vmStack() {
  #ifdef DEBUG_VM
    printf("vm stack:");
    for (i32 i = 0; i < (bcDepth>>1) + 1; i++) { printf(" %d", vmStack[i]); fflush(stdout); }
    printf("\n"); fflush(stdout);
  #endif
}



void comp_init() {
  ti[t_comp     ].free = comp_free;  ti[t_comp     ].visit = comp_visit;  ti[t_comp     ].print =  comp_print;
  ti[t_body     ].free = body_free;  ti[t_body     ].visit = body_visit;  ti[t_body     ].print =  body_print;
  ti[t_block    ].free = block_free; ti[t_block    ].visit = block_visit; ti[t_block    ].print = block_print;
  ti[t_scope    ].free = scope_free; ti[t_scope    ].visit = scope_visit; ti[t_scope    ].print = scope_print;
  ti[t_blBlocks ].free = bBlks_free; ti[t_blBlocks ].visit = bBlks_visit; ti[t_blBlocks ].print = bBlks_print;
  ti[t_fldAlias ].free = alias_free; ti[t_fldAlias ].visit = alias_visit; ti[t_fldAlias ].print = alias_print;
  ti[t_fun_block].free = funBl_free; ti[t_fun_block].visit = funBl_visit; ti[t_fun_block].print = funBl_print; ti[t_fun_block].decompose = block_decompose;
  ti[t_md1_block].free = md1Bl_free; ti[t_md1_block].visit = md1Bl_visit; ti[t_md1_block].print = md1Bl_print; ti[t_md1_block].decompose = block_decompose; ti[t_md1_block].m1_d=bl_m1d;
  ti[t_md2_block].free = md2Bl_free; ti[t_md2_block].visit = md2Bl_visit; ti[t_md2_block].print = md2Bl_print; ti[t_md2_block].decompose = block_decompose; ti[t_md2_block].m2_d=bl_m2d;
  #ifndef GS_REALLOC
    allocStack((void**)&gStack, (void**)&gStackStart, (void**)&gStackEnd, sizeof(B), GS_SIZE);
  #endif
  allocStack((void**)&envCurr, (void**)&envStart, (void**)&envEnd, sizeof(Env), ENV_SIZE);
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

NOINLINE void vm_printPos(Comp* comp, i32 bcPos, i64 pos) {
  B src = comp->src;
  if (!isNothing(src) && !isNothing(comp->indices)) {
    B inds = TI(comp->indices).getU(comp->indices, 0); usz cs = o2s(TI(inds).getU(inds,bcPos));
    B inde = TI(comp->indices).getU(comp->indices, 1); usz ce = o2s(TI(inde).getU(inde,bcPos))+1;
    int start = pos==-1? 0 : printf("%ld: ", pos);
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
    if (pos!=-1) printf("%ld: ", pos);
    printf("source unknown\n");
  }
}

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
    Comp* comp = c->sc->body->comp;
    i32 bcPos = c>=envStart && c<envCurr? BCPOS(c->sc->body, c->bcL) : c->bcV;
    vm_printPos(comp, bcPos, i);
    i--;
  }
}
NOINLINE void vm_pstLive() {
  vm_pst(envStart, envCurr);
}

static void unwindEnv(Env* envNew) {
  assert(envNew<=envCurr);
  while (envCurr!=envNew) {
    envCurr--;
    envCurr->bcV = BCPOS(envCurr->sc->body, envCurr->bcL);
  }
}

NOINLINE NORETURN void thr(B msg) {
  if (cf>cfStart) {
    catchMessage = msg;
    cf--;
    
    B* gStackNew = gStackStart + cf->gsDepth;
    assert(gStackNew<=gStack);
    while (gStack!=gStackNew) dec(*--gStack);
    envPrevHeight = envCurr-envStart;
    unwindEnv(envStart + cf->envDepth);
    
    
    if (cfStart+cf->cfDepth > cf) err("bad catch cfDepth");
    cf = cfStart+cf->cfDepth;
    longjmp(cf->jmp, 1);
  }
  assert(cf==cfStart);
  printf("Error: "); print(msg); putchar('\n');
  Env* envPrev = envCurr;
  unwindEnv(envStart);
  vm_pst(envCurr, envPrev);
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
