#include <unistd.h>
#include "core.h"
#include "vm.h"
#include "jit/nvm.h"
#include "ns.h"
#include "utils/utf.h"
#include "utils/talloc.h"
#include "utils/mut.h"

#ifndef UNWIND_COMPILER // whether to hide stackframes of the compiler in compiling errors
  #define UNWIND_COMPILER 1
#endif

#define FOR_BC(F) F(PUSH) F(VARO) F(VARM) F(ARRO) F(ARRM) F(FN1C) F(FN2C) F(OP1D) F(OP2D) F(TR2D) \
                  F(TR3D) F(SETN) F(SETU) F(SETM) F(POPS) F(DFND) F(FN1O) F(FN2O) F(CHKV) F(TR3O) \
                  F(OP2H) F(LOCO) F(LOCM) F(VFYM) F(SETH) F(RETN) F(FLDO) F(FLDM) F(NSPM) F(RETD) F(SYSV) F(LOCU) \
                  F(EXTO) F(EXTM) F(EXTU) F(ADDI) F(ADDU) F(FN1Ci)F(FN1Oi)F(FN2Ci)F(FN2Oi) \
                  F(SETNi)F(SETUi)F(SETMi)F(SETNv)F(SETUv)F(SETMv)F(FAIL)

u32* nextBC(u32* p) {
  i32 off;
  switch(*p) {
    case FN1C: case FN2C: case FN1O: case FN2O:
    case OP1D: case OP2D: case OP2H:
    case TR2D: case TR3D: case TR3O:
    case SETN: case SETU: case SETM: case SETH:
    case POPS: case CHKV: case VFYM: case RETN: case RETD:
    case FAIL:
      off = 1; break;
    case PUSH: case DFND: case ARRO: case ARRM:
    case VARO: case VARM: case FLDO: case FLDM:
    case SYSV: case NSPM:
      off = 2; break;
    case LOCO: case LOCM: case LOCU:
    case EXTO: case EXTM: case EXTU:
    case ADDI: case ADDU:
    case FN1Ci: case FN1Oi: case FN2Ci: case SETNi: case SETUi: case SETMi: case SETNv: case SETUv: case SETMv:
      off = 3; break;
    case FN2Oi:
      off = 5; break;
    default: UD;
  }
  return p+off;
}
i32 stackDiff(u32* p) {
  if (*p==ARRO|*p==ARRM) return 1-p[1];
  switch(*p) { default: UD; // case ARRO: case ARRM: return 1-p[1];
    case PUSH: case VARO: case VARM: case DFND: case LOCO: case LOCM: case LOCU: case EXTO: case EXTM: case EXTU: case SYSV: case ADDI: case ADDU: return 1;
    case FN1Ci:case FN1Oi:case CHKV: case VFYM: case FLDO: case FLDM: case RETD: case NSPM: return 0;
    case FN2Ci:case FN2Oi:case FN1C: case FN1O: case OP1D: case TR2D: case POPS: case OP2H: case SETH: case RETN: return -1;
    case OP2D: case TR3D: case FN2C: case FN2O: case TR3O: return -2;
    
    case SETN: return -1; case SETNi:return  0; case SETNv:return -1;
    case SETU: return -1; case SETUi:return  0; case SETUv:return -1;
    case SETM: return -2; case SETMi:return -1; case SETMv:return -2;
    case FAIL: return 0;
  }
}
i32 stackConsumed(u32* p) {
  if (*p==ARRO|*p==ARRM) return p[1];
  switch(*p) { default: UD; // case ARRO: case ARRM: return -p[1];
    case PUSH: case VARO: case VARM: case DFND: case LOCO: case LOCM: case LOCU: case EXTO: case EXTM: case EXTU: case SYSV: case ADDI: case ADDU: return 0;
    case CHKV: case VFYM: case RETD: return 0;
    case FN1Ci:case FN1Oi:case FLDO: case FLDM: case NSPM: case RETN: case POPS: return 1;
    case FN2Ci:case FN2Oi:case FN1C: case FN1O: case OP1D: case TR2D: case OP2H: case SETH: return 2;
    case OP2D: case TR3D: case FN2C: case FN2O: case TR3O: return 3;
    
    case SETN: return 2; case SETNi: case SETNv: return 1;
    case SETU: return 2; case SETUi: case SETUv: return 1;
    case SETM: return 3; case SETMi: case SETMv: return 2;
    case FAIL: return 0;
  }
}
i32 stackAdded(u32* p) {
  if (*p==ARRO|*p==ARRM) return 1;
  return stackDiff(p)+stackConsumed(p);
}
char* nameBC(u32* p) {
  switch(*p) { default: return "(unknown)";
    #define F(X) case X: return #X;
    FOR_BC(F)
    #undef F
  }
}
void printBC(u32* p) {
  printf("%s", nameBC(p));
  u32* n = nextBC(p);
  p++;
  i64 am = n-p;
  i32 len = 0;
  for (i64 i = 0; i < am; i++) printf(" %d", p[i]);
  while(p!=n) {
    i32 c = *p++;
    i64 pow = 10;
    i32 log = 1;
    while (pow<=c) { pow*=10; log++; }
    len+= log+1;
  }
  len = 6-len;
  while(len-->0) printf(" ");
}


B catchMessage;
u64 envPrevHeight;

Env* envCurr; // pointer to current environment; included to make for simpler current position updating
Env* envStart;
Env* envEnd;

B* gStack; // points to after end
B* gStackStart;
B* gStackEnd;
NOINLINE void gsReserveR(u64 am) { gsReserve(am); }
void gsPrint() {
  B* c = gStackStart;
  i32 i = 0;
  printf("gStack %p, height "N64d":\n", gStackStart, gStack-gStackStart);
  while (c!=gStack) {
    printf("  %d: ", i); fflush(stdout);
    print(*c); fflush(stdout);
    if (isVal(*c)) printf(", refc=%d", v(*c)->refc);
    printf("\n");
    fflush(stdout);
    c++;
    i++;
  }
}

static Body* m_body(i32 vam, i32 pos, u16 maxStack, u16 maxPSC) { // leaves varIDs and nsDesc uninitialized
  Body* body = mm_alloc(fsizeof(Body,varIDs,i32,vam), t_body);
  
  #if JIT_START != -1
    body->nvm = NULL;
    body->nvmRefs = m_f64(0);
  #endif
  #if JIT_START > 0
    body->callCount = 0;
  #endif
  body->bcTmp = pos;
  body->maxStack = maxStack;
  body->maxPSC = maxPSC;
  body->bl = NULL;
  body->varAm = (u16)vam;
  return body;
}

Block* compileBlock(B block, Comp* comp, bool* bDone, u32* bc, usz bcIA, B allBlocks, B allBodies, B nameList, Scope* sc, i32 depth) {
  usz blIA = a(block)->ia;
  if (blIA!=3) thrM("VM compiler: Bad block info size");
  BS2B blGetU = TI(block,getU);
  usz  ty  = o2s(blGetU(block,0)); if (ty>2) thrM("VM compiler: Bad type");
  bool imm = o2b(blGetU(block,1));
  B    bodyObj = blGetU(block,2);
  i32* bodyI;
  i32 bodyAm1, bodyAm2, bodyILen;
  if (isArr(bodyObj)) {
    if (a(bodyObj)->ia!=2) thrM("VM compiler: Unexpected body list length");
    // print(bodyObj); putchar('\n');
    BS2B boGetU = TI(bodyObj,getU);
    B b1 = boGetU(bodyObj,0);
    B b2 = boGetU(bodyObj,1);
    if (!isArr(b1) || !isArr(b2)) thrM("VM compiler: Body list contained non-arrays");
    bodyAm1 = a(b1)->ia; BS2B b1GetU = TI(b1,getU);
    bodyAm2 = a(b2)->ia; BS2B b2GetU = TI(b2,getU);
    bodyILen = bodyAm1+bodyAm2;
    TALLOC(i32, bodyInds_, bodyILen+2); bodyI = bodyInds_; i32* bodyI2 = bodyInds_+bodyAm1+1;
    for (i32 i = 0; i < bodyAm1; i++) bodyI [i] = o2i(b1GetU(b1, i));
    for (i32 i = 0; i < bodyAm2; i++) bodyI2[i] = o2i(b2GetU(b2, i));
    for (i32 i = 1; i < bodyAm1; i++) if (bodyI [i]<=bodyI [i-1]) thrM("VM compiler: Expected body indices to be sorted");
    for (i32 i = 1; i < bodyAm2; i++) if (bodyI2[i]<=bodyI2[i-1]) thrM("VM compiler: Expected body indices to be sorted");
    bodyI[bodyAm1] = bodyI[bodyILen+1] = I32_MAX;
  } else {
    bodyILen = 2;
    TALLOC(i32, bodyInds_, bodyILen+2); bodyI = bodyInds_;
    bodyI[0] = bodyI[2] = o2i(bodyObj);
    bodyI[1] = bodyI[3] = I32_MAX;
    bodyAm1 = 1;
    bodyAm2 = 1;
  }
  // for (int i = 0; i < bodyILen+2; i++) printf("%d ", bodyI[i]); putchar('\n'); printf("things: %d %d\n", bodyAm1, bodyAm2);
  TSALLOC(Block*, usedBlocks, 2); // list of blocks to be referenced by DFND, stored in result->blocks
  TSALLOC(Body*, bodies, 2); // list of bodies of this block
  TSALLOC(i32, newBC, 20); // transformed bytecode
  TSALLOC(i32, mapBC, 20); // map of original bytecode to transformed
  
  i32 pos1 = 0;
  i32 pos2 = bodyAm1+1;
  i32 index1 = -1;
  i32 index2 = -1;
  if (bodyAm1==0 || bodyAm2==0) {
    i32 sz = TSSIZE(bodies);
    if (bodyAm1==0) index1 = sz;
    if (bodyAm2==0) index2 = sz;
    i32 bcStart = TSSIZE(newBC);
    TSADD(newBC, FAIL);
    TSADD(mapBC, 0);
    
    Body* body = m_body(6, bcStart, 1, 0);
    body->nsDesc = NULL;
    TSADD(bodies, body);
  }
  
  while (true) {
    i32 curr1 = bodyI[pos1];
    i32 curr2 = bodyI[pos2];
    i32 currBody = curr1<curr2? curr1 : curr2;
    if (currBody==I32_MAX) break;
    // printf("step %d %d:  %d %d %d\n", pos1, pos2, curr1, curr2, currBody);
    if (curr1==currBody) { if (index1==-1) index1=TSSIZE(bodies); pos1++; }
    if (curr2==currBody) { if (index2==-1) index2=TSSIZE(bodies); pos2++; }
    // printf("idxs: %d %d\n", index1, index2);
    
    
    B bodyRepr = TI(allBodies,getU)(allBodies, currBody); if (!isArr(bodyRepr)) thrM("VM compiler: Body array contained non-array");
    usz boIA = a(bodyRepr)->ia; if (boIA!=2 && boIA!=4) thrM("VM compiler: Body array had invalid length");
    BS2B biGetU = TI(bodyRepr,getU);
    usz idx = o2s(biGetU(bodyRepr,0)); if (idx>=bcIA) thrM("VM compiler: Bytecode index out of bounds");
    usz vam = o2s(biGetU(bodyRepr,1)); if (vam!=(u16)vam) thrM("VM compiler: >2⋆16 variables not supported"); // TODO any reason for this? 2⋆32 vars should just work, no? // oh, some size fields are u16s. but i doubt those change much, or even make things worse
    
    i32 h = 0; // stack height
    i32 hM = 0; // max stack height
    i32 mpsc = 0;
    if (depth==0 && sc && vam > sc->varAm) {
      if (boIA==2) thrM("VM compiler: Full block info must be provided for extending scopes");
      u32 regAm = sc->varAm;
      ScopeExt* oE = sc->ext;
      if (oE==NULL || vam > regAm+oE->varAm) {
        i32 nSZ = vam - regAm;
        ScopeExt* nE = mm_alloc(fsizeof(ScopeExt, vars, B, nSZ*2), t_scopeExt);
        nE->varAm = nSZ;
        i32 oSZ = 0;
        if (oE) {
          oSZ = oE->varAm;
          memcpy(nE->vars    , oE->vars    , oSZ*sizeof(B));
          memcpy(nE->vars+nSZ, oE->vars+oSZ, oSZ*sizeof(B));
          mm_free((Value*)oE);
        }
        B varIDs = biGetU(bodyRepr,2);
        for (i32 i = oSZ; i < nSZ; i++) {
          nE->vars[i] = bi_noVar;
          nE->vars[i+nSZ] = TI(nameList,get)(nameList, o2s(TI(varIDs,getU)(varIDs, regAm+i)));
        }
        sc->ext = nE;
      }
    }
    i32 bcStart = TSSIZE(newBC);
    u32* c = bc+idx;
    while (true) {
      u32* n = nextBC(c);
      if (n-bc-1 >= bcIA) thrM("VM compiler: No RETN/RETD found before end of bytecode");
      bool ret = false;
      #define A64(X) { u64 a64=(X); TSADD(newBC, (u32)a64); TSADD(newBC, a64>>32); }
      switch (*c) {
        case PUSH:;
          B obj = comp->objs->a[c[1]];
          TSADD(newBC, isVal(obj)? ADDI : ADDU);
          A64(obj.u);
          break;
        case RETN: if(h!=1) thrM("VM compiler: Wrong stack size before RETN");
          TSADD(newBC, RETN);
          ret = true;
          break;
        case RETD: if(h!=1&h!=0) thrM("VM compiler: Wrong stack size before RETD");
          if (h==1) TSADD(newBC, POPS);
          TSADD(newBC, RETD);
          ret = true;
          break;
        case DFND: {
          u32 id = c[1];
          if ((u32)id >= a(allBlocks)->ia) thrM("VM compiler: DFND index out-of-bounds");
          if (bDone[id]) thrM("VM compiler: DFND of the same block in multiple places");
          bDone[id] = true;
          TSADD(newBC, DFND);
          TSADD(newBC, TSSIZE(usedBlocks));
          TSADD(usedBlocks, compileBlock(TI(allBlocks,getU)(allBlocks,id), comp, bDone, bc, bcIA, allBlocks, allBodies, nameList, sc, depth+1));
          break;
        }
        case LOCO: case LOCM: case LOCU: {
          i32 ins = c[0];
          i32 cdepth = c[1];
          i32 cpos = c[2];
          if (cdepth+1 > mpsc) mpsc = cdepth+1;
          if (sc && cdepth>=depth) {
            Scope* csc = sc;
            for (i32 i = depth; i < cdepth; i++) if (!(csc = csc->psc)) thrM("VM compiler: LOC_ has an out-of-bounds depth");
            if (cpos >= csc->varAm) {
              cpos-= csc->varAm;
              ins = ins==LOCO? EXTO : ins==LOCM? EXTM : EXTO;
            }
          }
          TSADD(newBC, ins);
          TSADD(newBC, cdepth);
          TSADD(newBC, cpos);
          break;
        }
        default: {
          u32* ccpy = c;
          while (ccpy!=n) TSADD(newBC, *ccpy++);
          break;
        }
      }
      #undef A64
      usz nlen = TSSIZE(newBC)-TSSIZE(mapBC);
      for (usz i = 0; i < nlen; i++) TSADD(mapBC, c-bc);
      h+= stackDiff(c);
      if (h<0) thrM("VM compiler: Stack size goes negative");
      if (h>hM) hM = h;
      if (ret) break;
      c = n;
    }
    
    if (mpsc>U16_MAX) thrM("VM compiler: Block too deep");
    
    Body* body = m_body(vam, bcStart, hM, mpsc);
    if (boIA>2) {
      m_nsDesc(body, imm, ty, inc(nameList), biGetU(bodyRepr,2), biGetU(bodyRepr,3));
    } else {
      body->nsDesc = NULL;
      for (u64 i = 0; i < vam; i++) body->varIDs[i] = -1;
    }
    
    TSADD(bodies, body);
  }
  TFREE(bodyI);
  
  usz blC = TSSIZE(usedBlocks);
  BlBlocks* nBl = NULL;
  if (blC) {
    nBl = mm_alloc(fsizeof(BlBlocks,a,Block*,blC), t_blBlocks);
    nBl->am = blC;
    memcpy(nBl->a, usedBlocks, blC*sizeof(Block*));
  }
  TSFREE(usedBlocks);
  
  usz nbcC = TSSIZE(newBC); i32* nbc; m_i32arrv(&nbc, nbcC); memcpy(nbc, newBC, nbcC*4); TSFREE(newBC);
  usz mapC = TSSIZE(mapBC); i32* map; m_i32arrv(&map, mapC); memcpy(map, mapBC, mapC*4); TSFREE(mapBC);
  
  i32 bodyCount = TSSIZE(bodies);
  Block* bl = mm_alloc(fsizeof(Block,bodies,Body*,bodyCount), t_block);
  bl->comp = comp; ptr_inc(comp);
  bl->ty = (u8)ty;
  bl->bc = nbc;
  bl->blocks = nBl==NULL? NULL : nBl->a;
  bl->map = map;
  bl->imm = imm;
  
  bl->bodyCount = bodyCount;
  if (index1 != 0) { // this is a _mess_
    i32 sw0 = 0; i32 sw1 = index1;
    
    Body* t = bodies[sw0]; bodies[sw0] = bodies[sw1]; bodies[sw1] = t;
    index1 = sw0;
    
    if      (index2==sw0) index2 = sw1;
    else if (index2==sw1) index2 = sw0;
  }
  for (i32 i = 0; i < bodyCount; i++) {
    bl->bodies[i] = bodies[i];
    bodies[i]->bc = (u32*)nbc + bodies[i]->bcTmp;
    bodies[i]->bl = bl;
  }
  bl->dyBody = bodies[index2];
  TSFREE(bodies);
  return bl;
}

// consumes all; assumes arguments are valid (verifies some stuff, but definitely not everything)
// if sc isn't NULL, this block must only be evaluated directly in that scope precisely once
NOINLINE Block* compile(B bcq, B objs, B allBlocks, B allBodies, B indices, B tokenInfo, B src, B path, Scope* sc) {
  usz bIA = a(allBlocks)->ia;
  I32Arr* bca = toI32Arr(bcq);
  u32* bc = (u32*)bca->a;
  usz bcIA = bca->ia;
  Comp* comp = mm_alloc(sizeof(Comp), t_comp);
  comp->bc = taga(bca);
  comp->indices = indices;
  comp->src = src;
  comp->path = path;
  comp->objs = toHArr(objs);
  comp->blockAm = 0;
  B nameList;
  if (q_N(tokenInfo)) {
    nameList = bi_emptyHVec;
  } else {
    B t = TI(tokenInfo,getU)(tokenInfo,2);
    nameList = TI(t,getU)(t,0);
  }
  if (!q_N(src) && !q_N(indices)) {
    if (isAtm(indices) || rnk(indices)!=1 || a(indices)->ia!=2) thrM("VM compiler: Bad indices");
    for (i32 i = 0; i < 2; i++) {
      B ind = TI(indices,getU)(indices,i);
      if (isAtm(ind) || rnk(ind)!=1 || a(ind)->ia!=bcIA) thrM("VM compiler: Bad indices");
      BS2B indGetU = TI(ind,getU);
      for (usz j = 0; j < bcIA; j++) o2i(indGetU(ind,j));
    }
  }
  TALLOC(bool,bDone,bIA);
  for (usz i = 0; i < bIA; i++) bDone[i] = false;
  Block* ret = compileBlock(TI(allBlocks,getU)(allBlocks, 0), comp, bDone, bc, bcIA, allBlocks, allBodies, nameList, sc, 0);
  TFREE(bDone);
  ptr_dec(comp); dec(allBlocks); dec(allBodies); dec(tokenInfo);
  return ret;
}




NOINLINE void v_setR(Scope* pscs[], B s, B x, bool upd) {
  if (isExt(s)) {
    Scope* sc = pscs[(u16)(s.u>>32)];
    B prev = sc->ext->vars[(u32)s.u];
    if (upd) {
      if (prev.u==bi_noVar.u) thrM("↩: Updating undefined variable");
      dec(prev);
    } else dec(prev);
    sc->ext->vars[(u32)s.u] = inc(x);
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
          } else if (isExt(c)) {
            ScopeExt* ext = pscs[(u16)(c.u>>32)]->ext;
            v_set(pscs, c, ns_getNU(x, ext->vars[(u32)c.u + ext->varAm]), upd);
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
    BS2B xgetU = TI(x,getU);
    for (u64 i = 0; i < ia; i++) v_set(pscs, sp[i], xgetU(x,i), upd);
  }
}



NOINLINE B v_getR(Scope* pscs[], B s) {
  if (isExt(s)) {
    Scope* sc = pscs[(u16)(s.u>>32)];
    B r = sc->ext->vars[(u32)s.u];
    sc->ext->vars[(u32)s.u] = bi_optOut;
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
#define BCPOS(B,P) (B->bl->map[(P)-B->bc])
B evalBC(Block* bl, Body* b, Scope* sc) { // doesn't consume
  #ifdef DEBUG_VM
    bcDepth+= 2;
    if (!vmStack) vmStack = malloc(400);
    i32 stackNum = bcDepth>>1;
    vmStack[stackNum] = -1;
    printf("new eval\n");
  #endif
  B* objs = bl->comp->objs->a;
  Block** blocks = bl->blocks;
  u32* bc = b->bc;
  pushEnv(sc, bc);
  gsReserve(b->maxStack);
  Scope* pscs[b->maxPSC];
  if (b->maxPSC) {
    pscs[0] = sc;
    for (i32 i = 1; i < b->maxPSC; i++) pscs[i] = pscs[i-1]->psc;
  }
  #ifdef GS_REALLOC
    #define POP (*--gStack)
    #define P(N) B N=POP;
    #define ADD(X) { B tr=X; *(gStack++) = tr; }
    #define PEEK(X) gStack[-(X)]
    #define GS_UPD
  #else
    B* lgStack = gStack;
    #define POP (*--lgStack)
    #define P(N) B N=POP;
    #define ADD(X) { *(lgStack++) = X; } // fine, as, if an error occurs, lgStack is ignored anyways
    #define PEEK(X) lgStack[-(X)]
    #define GS_UPD { gStack = lgStack; }
  #endif
  #define L64 ({ u64 r = bc[0] | ((u64)bc[1])<<32; bc+= 2; r; })
  #if VM_POS
    #define POS_UPD envCurr->pos = (u64)(bc-1);
  #else
    #define POS_UPD
  #endif
  
  while(true) {
    #ifdef DEBUG_VM
      u32* sbc = bc;
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
      case ADDI: {
        B o = b(L64);
        ptr_inc(v(o));
        ADD(o);
        break;
      }
      case ADDU: {
        ADD(b(L64));
        break;
      }
      case FN1C: { P(f)P(x)
        GS_UPD;POS_UPD;
        ADD(c1(f, x); dec(f));
        break;
      }
      case FN1O: { P(f)P(x)
        GS_UPD;POS_UPD;
        ADD(q_N(x)? x : c1(f, x)); dec(f);
        break;
      }
      case FN2C: { P(w)P(f)P(x)
        GS_UPD;POS_UPD;
        ADD(c2(f, w, x); dec(f));
        break;
      }
      case FN2O: { P(w)P(f)P(x)
        GS_UPD;POS_UPD;
        if (q_N(x)) { dec(w); ADD(x); }
        else ADD(q_N(w)? c1(f, x) : c2(f, w, x));
        dec(f);
        break;
      }
      case ARRO: case ARRM: {
        u32 sz = *bc++;
        if (sz==0) {
          ADD(emptyHVec());
        } else {
          HArr_p r = m_harrUv(sz);
          bool allNum = true;
          for (i64 i = 0; i < sz; i++) if (!isNum(r.a[sz-i-1] = POP)) allNum = false;
          if (allNum) {
            GS_UPD;
            ADD(withFill(r.b, m_f64(0)));
          } else ADD(r.b);
        }
        break;
      }
      case DFND: {
        GS_UPD;POS_UPD;
        Block* cbl = blocks[*bc++];
        switch(cbl->ty) { default: UD;
          case 0: ADD(m_funBlock(cbl, sc)); break;
          case 1: ADD(m_md1Block(cbl, sc)); break;
          case 2: ADD(m_md2Block(cbl, sc)); break;
        }
        break;
      }
      case OP1D: { P(f)P(m)     GS_UPD;POS_UPD; ADD(m1_d  (m,f  )); break; }
      case OP2D: { P(f)P(m)P(g) GS_UPD;POS_UPD; ADD(m2_d  (m,f,g)); break; }
      case OP2H: {     P(m)P(g)                 ADD(m2_h  (m,  g)); break; }
      case TR2D: {     P(g)P(h)                 ADD(m_atop(  g,h)); break; }
      case TR3D: { P(f)P(g)P(h)                 ADD(m_fork(f,g,h)); break; }
      case TR3O: { P(f)P(g)P(h)
        if (q_N(f)) { ADD(m_atop(g,h)); dec(f); }
        else ADD(m_fork(f,g,h));
        break;
      }
      case LOCM: { u32 d = *bc++; u32 p = *bc++;
        ADD(tag((u64)d<<32 | (u32)p, VAR_TAG));
        break;
      }
      case LOCO: { u32 d = *bc++; u32 p = *bc++;
        B l = pscs[d]->vars[p];
        if(l.u==bi_noVar.u) { POS_UPD; thrM("Reading variable before its defined"); }
        ADD(inc(l));
        break;
      }
      case LOCU: { u32 d = *bc++; u32 p = *bc++;
        B* vars = pscs[d]->vars;
        ADD(vars[p]);
        vars[p] = bi_optOut;
        break;
      }
      case EXTM: { u32 d = *bc++; u32 p = *bc++;
        ADD(tag((u64)d<<32 | (u32)p, EXT_TAG));
        break;
      }
      case EXTO: { u32 d = *bc++; u32 p = *bc++;
        B l = pscs[d]->ext->vars[p];
        if(l.u==bi_noVar.u) { POS_UPD; thrM("Reading variable before its defined"); }
        ADD(inc(l));
        break;
      }
      case EXTU: { u32 d = *bc++; u32 p = *bc++;
        B* vars = pscs[d]->ext->vars;
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
      case FLDO: { P(ns) GS_UPD; u32 p = *bc++; POS_UPD;
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
      case NSPM: { P(o) u32 l = *bc++;
        FldAlias* a = mm_alloc(sizeof(FldAlias), t_fldAlias);
        a->obj = o;
        a->p = l;
        ADD(tag(a,OBJ_TAG));
        break;
      }
      case RETN: goto end;
      case CHKV: {
        if (q_N(PEEK(1))) { GS_UPD; POS_UPD; thrM("Unexpected Nothing (·)"); }
        break;
      }
      case FAIL: thrM("No body matched");
      // not implemented: VARO VARM VFYM SETH FLDM SYSV
      default:
        #ifdef DEBUG
          printf("todo %d\n", bc[-1]); bc++; break;
        #else
          UD;
        #endif
    }
    #ifdef DEBUG_VM
      for(i32 i = 0; i < bcDepth; i++) printf(" ");
      printBC(sbc); printf("@"N64d":   ", BCPOS(b, sbc));
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
  #undef L64
  #undef P
  #undef ADD
  #undef POP
  #undef POS_UPD
  #undef GS_UPD
}

Scope* m_scope(Body* body, Scope* psc, u16 varAm, i32 initVarAm, B* initVars) { // doesn't consume
  Scope* sc = mm_alloc(fsizeof(Scope, vars, B, varAm), t_scope);
  sc->body = body; ptr_inc(body);
  sc->psc = psc; if(psc) ptr_inc(psc);
  sc->varAm = varAm;
  sc->ext = NULL;
  i32 i = 0;
  while (i<initVarAm) { sc->vars[i] = initVars[i]; i++; }
  while (i<varAm) sc->vars[i++] = bi_noVar;
  return sc;
}
static void scope_dec(Scope* sc) {
  i32 varAm = sc->varAm;
  if (sc->refc>1) {
    i32 innerRef = 1;
    for (i32 i = 0; i < varAm; i++) {
      B c = sc->vars[i];
      if (isVal(c) && v(c)->refc==1) {
        u8 t = v(c)->type;
        if      (t==t_fun_block && c(FunBlock,c)->sc==sc) innerRef++;
        else if (t==t_md1_block && c(Md1Block,c)->sc==sc) innerRef++;
        else if (t==t_md2_block && c(Md2Block,c)->sc==sc) innerRef++;
      }
    }
    assert(innerRef <= sc->refc);
    if (innerRef==sc->refc) {
      value_free((Value*)sc);
      return;
    }
  }
  ptr_dec(sc);
}

FORCE_INLINE B execBodyInlineI(Block* block, Body* body, Scope* sc) {
  #if JIT_START != -1
    if (body->nvm) { toJIT: return evalJIT(body, sc, body->nvm); }
    bool jit = true;
    #if JIT_START > 0
      jit = body->callCount++ >= JIT_START;
    #endif
    // jit = body->bc[2]==m_f64(123456).u>>32; // enable JIT for blocks starting with `123456⋄`
    if (jit) {
      Nvm_res r = m_nvm(body);
      body->nvm = r.p;
      body->nvmRefs = r.refs;
      goto toJIT;
    }
  #endif
  return evalBC(block, body, sc);
}
B execBlockInline(Block* block, Scope* sc) { return execBodyInlineI(block, block->bodies[0], sc); }

FORCE_INLINE B execBlock(Block* block, Body* body, Scope* psc, i32 ga, B* svar) { // consumes svar contents
  u16 varAm = body->varAm;
  assert(varAm>=ga);
  Scope* sc = m_scope(body, psc, varAm, ga, svar);
  B r = execBodyInlineI(block, body, sc);
  scope_dec(sc);
  return r;
}

B funBl_c1(B t,      B x) {                    FunBlock* b=c(FunBlock, t    ); ptr_inc(b); return execBlock(b->bl, b->bl->bodies[0], b->sc, 3, (B[]){t, x, bi_N                                  }); }
B funBl_c2(B t, B w, B x) {                    FunBlock* b=c(FunBlock, t    ); ptr_inc(b); return execBlock(b->bl, b->bl->dyBody,    b->sc, 3, (B[]){t, x, w                                     }); }
B md1Bl_c1(B D,      B x) { Md1D* d=c(Md1D,D); Md1Block* b=c(Md1Block, d->m1); ptr_inc(d); return execBlock(b->bl, b->bl->bodies[0], b->sc, 5, (B[]){D, x, bi_N, inc(d->m1), inc(d->f)           }); }
B md1Bl_c2(B D, B w, B x) { Md1D* d=c(Md1D,D); Md1Block* b=c(Md1Block, d->m1); ptr_inc(d); return execBlock(b->bl, b->bl->dyBody,    b->sc, 5, (B[]){D, x, w   , inc(d->m1), inc(d->f)           }); }
B md2Bl_c1(B D,      B x) { Md2D* d=c(Md2D,D); Md2Block* b=c(Md2Block, d->m2); ptr_inc(d); return execBlock(b->bl, b->bl->bodies[0], b->sc, 6, (B[]){D, x, bi_N, inc(d->m2), inc(d->f), inc(d->g)}); }
B md2Bl_c2(B D, B w, B x) { Md2D* d=c(Md2D,D); Md2Block* b=c(Md2Block, d->m2); ptr_inc(d); return execBlock(b->bl, b->bl->dyBody,    b->sc, 6, (B[]){D, x, w   , inc(d->m2), inc(d->f), inc(d->g)}); }
B m_funBlock(Block* bl, Scope* psc) { // doesn't consume anything
  if (bl->imm) return execBlock(bl, bl->bodies[0], psc, 0, NULL);
  FunBlock* r = mm_alloc(sizeof(FunBlock), t_fun_block);
  r->bl = bl; ptr_inc(bl);
  r->sc = psc; ptr_inc(psc);
  r->c1 = funBl_c1;
  r->c2 = funBl_c2;
  return tag(r, FUN_TAG);
}
B m_md1Block(Block* bl, Scope* psc) {
  Md1Block* r = mm_alloc(sizeof(Md1Block), t_md1_block);
  r->bl = bl; ptr_inc(bl);
  r->sc = psc; ptr_inc(psc);
  r->c1 = md1Bl_c1;
  r->c2 = md1Bl_c2;
  return tag(r, MD1_TAG);
}
B m_md2Block(Block* bl, Scope* psc) {
  Md2Block* r = mm_alloc(sizeof(Md2Block), t_md2_block);
  r->bl = bl; ptr_inc(bl);
  r->sc = psc; ptr_inc(psc);
  r->c1 = md2Bl_c1;
  r->c2 = md2Bl_c2;
  return tag(r, MD2_TAG);
}

DEF_FREE(scope) {
  Scope* c = (Scope*)x;
  if (LIKELY(c->psc!=NULL)) ptr_decR(c->psc);
  if (RARE  (c->ext!=NULL)) ptr_decR(c->ext);
  ptr_decR(c->body);
  u16 am = c->varAm;
  for (u32 i = 0; i < am; i++) dec(c->vars[i]);
}
DEF_FREE(body) {
  Body* c = (Body*)x;
  #if JIT_START!=-1
    if(c->nvm) nvm_free(c->nvm);
    dec(c->nvmRefs);
  #endif
  if(c->nsDesc) ptr_decR(c->nsDesc);
}
DEF_FREE(block) {
  Block* c = (Block*)x;
  ptr_decR(c->comp);
  if(c->blocks) ptr_decR(RFLD(c->blocks,BlBlocks,a));
  ptr_decR(RFLD(c->bc, I32Arr,a));
  ptr_decR(RFLD(c->map,I32Arr,a));
  i32 am = c->bodyCount;
  for (i32 i = 0; i < am; i++) ptr_decR(c->bodies[i]);
}
DEF_FREE(comp) { Comp*     c = (Comp    *)x; ptr_decR(c->objs); decR(c->bc); decR(c->src); decR(c->indices); decR(c->path); }
DEF_FREE(funBl) { FunBlock* c = (FunBlock*)x; ptr_dec(c->sc); ptr_decR(c->bl); }
DEF_FREE(md1Bl) { Md1Block* c = (Md1Block*)x; ptr_dec(c->sc); ptr_decR(c->bl); }
DEF_FREE(md2Bl) { Md2Block* c = (Md2Block*)x; ptr_dec(c->sc); ptr_decR(c->bl); }
DEF_FREE(alias) { dec(((FldAlias*)x)->obj); }
DEF_FREE(bBlks) { BlBlocks* c = (BlBlocks*)x; u16 am = c->am; for (i32 i = 0; i < am; i++) ptr_dec(c->a[i]); }
DEF_FREE(scExt) { ScopeExt* c = (ScopeExt*)x; u16 am = c->varAm*2; for (i32 i = 0; i < am; i++) dec(c->vars[i]); }

void scope_visit(Value* x) {
  Scope* c = (Scope*)x;
  if (c->psc) mm_visitP(c->psc);
  if (c->ext) mm_visitP(c->ext);
  mm_visitP(c->body);
  u16 am = c->varAm;
  for (u32 i = 0; i < am; i++) mm_visit(c->vars[i]);
}
void body_visit(Value* x) {
  Body* c = (Body*) x;
  #if JIT_START != -1
    mm_visit(c->nvmRefs);
  #endif
  if(c->nsDesc) mm_visitP(c->nsDesc);
}
void block_visit(Value* x) {
  Block* c = (Block*)x;
  mm_visitP(c->comp);
  if(c->blocks) mm_visitP(RFLD(c->blocks,BlBlocks,a));
  mm_visitP(RFLD(c->bc, I32Arr,a));
  mm_visitP(RFLD(c->map,I32Arr,a));
  i32 am = c->bodyCount;
  for (i32 i = 0; i < am; i++) mm_visitP(c->bodies[i]);
}
void  comp_visit(Value* x) { Comp*     c = (Comp    *)x; mm_visitP(c->objs); mm_visit(c->bc); mm_visit(c->src); mm_visit(c->indices); mm_visit(c->path); }
void funBl_visit(Value* x) { FunBlock* c = (FunBlock*)x; mm_visitP(c->sc); mm_visitP(c->bl); }
void md1Bl_visit(Value* x) { Md1Block* c = (Md1Block*)x; mm_visitP(c->sc); mm_visitP(c->bl); }
void md2Bl_visit(Value* x) { Md2Block* c = (Md2Block*)x; mm_visitP(c->sc); mm_visitP(c->bl); }
void alias_visit(Value* x) { mm_visit(((FldAlias*)x)->obj); }
void bBlks_visit(Value* x) { BlBlocks* c = (BlBlocks*)x; u16 am = c->am; for (i32 i = 0; i < am; i++) mm_visitP(c->a[i]); }
void scExt_visit(Value* x) { ScopeExt* c = (ScopeExt*)x; u16 am = c->varAm*2; for (i32 i = 0; i < am; i++) mm_visit(c->vars[i]); }

void comp_print (B x) { printf("(%p: comp)",v(x)); }
void body_print (B x) { printf("(%p: body varam=%d)",v(x),c(Body,x)->varAm); }
void block_print(B x) { printf("(%p: block)",v(x)); }
void scope_print(B x) { printf("(%p: scope; vars:",v(x));Scope*sc=c(Scope,x);for(u64 i=0;i<sc->varAm;i++){printf(" ");print(sc->vars[i]);}printf(")"); }
void alias_print(B x) { printf("(alias %d of ", c(FldAlias,x)->p); print(c(FldAlias,x)->obj); printf(")"); }
void bBlks_print(B x) { printf("(block list)"); }
void scExt_print(B x) { printf("(scope extension with %d vars)", c(ScopeExt,x)->varAm); }

// void funBl_print(B x) { printf("(%p: function"" block bl=%p sc=%p)",v(x),c(FunBlock,x)->bl,c(FunBlock,x)->sc); }
// void md1Bl_print(B x) { printf("(%p: 1-modifier block bl=%p sc=%p)",v(x),c(Md1Block,x)->bl,c(Md1Block,x)->sc); }
// void md2Bl_print(B x) { printf("(%p: 2-modifier block bl=%p sc=%p)",v(x),c(Md2Block,x)->bl,c(Md2Block,x)->sc); }
// void funBl_print(B x) { printf("(function"" block @%d)",c(FunBlock,x)->bl->body->map[0]); }
// void md1Bl_print(B x) { printf("(1-modifier block @%d)",c(Md1Block,x)->bl->body->map[0]); }
// void md2Bl_print(B x) { printf("(2-modifier block @%d)",c(Md2Block,x)->bl->body->map[0]); }
void funBl_print(B x) { printf("{function"" block}"); }
void md1Bl_print(B x) { printf("{1-modifier block}"); }
void md2Bl_print(B x) { printf("{2-modifier block}"); }

B block_decompose(B x) { return m_v2(m_i32(1), x); }

B bl_m1d(B m, B f     ) { Md1Block* c = c(Md1Block,m); Block* bl=c(Md1Block, m)->bl; return c->bl->imm? execBlock(bl, bl->bodies[0], c(Md1Block, m)->sc, 2, (B[]){m, f   }) : m_md1D(m,f  ); }
B bl_m2d(B m, B f, B g) { Md2Block* c = c(Md2Block,m); Block* bl=c(Md2Block, m)->bl; return c->bl->imm? execBlock(bl, bl->bodies[0], c(Md2Block, m)->sc, 3, (B[]){m, f, g}) : m_md2D(m,f,g); }

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
  TIi(t_comp     ,freeO) =  comp_freeO; TIi(t_comp     ,freeF) =  comp_freeF; TIi(t_comp     ,visit) = comp_visit;  TIi(t_comp     ,print) =  comp_print;
  TIi(t_body     ,freeO) =  body_freeO; TIi(t_body     ,freeF) =  body_freeF; TIi(t_body     ,visit) = body_visit;  TIi(t_body     ,print) =  body_print;
  TIi(t_block    ,freeO) = block_freeO; TIi(t_block    ,freeF) = block_freeF; TIi(t_block    ,visit) = block_visit; TIi(t_block    ,print) = block_print;
  TIi(t_scope    ,freeO) = scope_freeO; TIi(t_scope    ,freeF) = scope_freeF; TIi(t_scope    ,visit) = scope_visit; TIi(t_scope    ,print) = scope_print;
  TIi(t_scopeExt ,freeO) = scExt_freeO; TIi(t_scopeExt ,freeF) = scExt_freeF; TIi(t_scopeExt ,visit) = scExt_visit; TIi(t_scopeExt ,print) = scExt_print;
  TIi(t_blBlocks ,freeO) = bBlks_freeO; TIi(t_blBlocks ,freeF) = bBlks_freeF; TIi(t_blBlocks ,visit) = bBlks_visit; TIi(t_blBlocks ,print) = bBlks_print;
  TIi(t_fldAlias ,freeO) = alias_freeO; TIi(t_fldAlias ,freeF) = alias_freeF; TIi(t_fldAlias ,visit) = alias_visit; TIi(t_fldAlias ,print) = alias_print;
  TIi(t_fun_block,freeO) = funBl_freeO; TIi(t_fun_block,freeF) = funBl_freeF; TIi(t_fun_block,visit) = funBl_visit; TIi(t_fun_block,print) = funBl_print; TIi(t_fun_block,decompose) = block_decompose;
  TIi(t_md1_block,freeO) = md1Bl_freeO; TIi(t_md1_block,freeF) = md1Bl_freeF; TIi(t_md1_block,visit) = md1Bl_visit; TIi(t_md1_block,print) = md1Bl_print; TIi(t_md1_block,decompose) = block_decompose; TIi(t_md1_block,m1_d)=bl_m1d;
  TIi(t_md2_block,freeO) = md2Bl_freeO; TIi(t_md2_block,freeF) = md2Bl_freeF; TIi(t_md2_block,visit) = md2Bl_visit; TIi(t_md2_block,print) = md2Bl_print; TIi(t_md2_block,decompose) = block_decompose; TIi(t_md2_block,m2_d)=bl_m2d;
  #ifndef GS_REALLOC
    allocStack((void**)&gStack, (void**)&gStackStart, (void**)&gStackEnd, sizeof(B), GS_SIZE);
  #endif
  allocStack((void**)&envCurr, (void**)&envStart, (void**)&envEnd, sizeof(Env), ENV_SIZE);
  envCurr--;
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
  cf->envDepth = (envCurr+1)-envStart;
  return &(cf++)->jmp;
}
void popCatch() {
  #if CATCH_ERRORS
    assert(cf>cfStart);
    cf--;
  #endif
}

NOINLINE B vm_fmtPoint(B src, B prepend, B path, usz cs, usz ce) { // consumes prepend
  BS2B srcGetU = TI(src,getU);
  usz srcL = a(src)->ia;
  usz srcS = cs;
  while (srcS>0 && o2cu(srcGetU(src,srcS-1))!='\n') srcS--;
  usz srcE = srcS;
  while (srcE<srcL) { if(o2cu(srcGetU(src, srcE))=='\n') break; srcE++; }
  if (ce>srcE) ce = srcE;
  
  i64 ln = 1;
  for (usz i = 0; i < srcS; i++) if(o2cu(srcGetU(src, i))=='\n') ln++;
  B s = prepend;
  if (isArr(path) && (a(path)->ia>1 || (a(path)->ia==1 && TI(path,getU)(path,0).u!=m_c32('.').u))) AFMT("%R:%l:\n  ", path, ln);
  else AFMT("at ");
  i64 padEnd = (i64)a(s)->ia;
  i64 padStart = padEnd;
  while (padStart>0 && o2cu(srcGetU(s,padStart-1))!='\n') padStart--;
  
  Arr* slice = TI(src,slice)(inc(src),srcS, srcE-srcS); arr_shVec(slice);
  AJOIN(taga(slice));
  cs-= srcS;
  ce-= srcS;
  ACHR('\n');
  for (i64 i = padStart; i < padEnd; i++) ACHR(' ');
  for (u64 i = 0; i < cs; i++) ACHR(o2cu(srcGetU(src, srcS+i))=='\t'? '\t' : ' '); // ugh tabs
  for (u64 i = cs; i < ce; i++) ACHR('^');
  return s;
}

NOINLINE void vm_printPos(Comp* comp, i32 bcPos, i64 pos) {
  B src = comp->src;
  if (!q_N(src) && !q_N(comp->indices)) {
    B inds = TI(comp->indices,getU)(comp->indices, 0); usz cs = o2s(TI(inds,getU)(inds,bcPos));
    B inde = TI(comp->indices,getU)(comp->indices, 1); usz ce = o2s(TI(inde,getU)(inde,bcPos))+1;
    // printf("  bcPos=%d\n", bcPos);       // in case the pretty error generator is broken
    // printf(" inds:%d…%d\n", cs, ce);
    // int start = pos==-1? 0 : printf(N64d": ", pos);
    // usz srcL = a(src)->ia;
    // BS2B srcGetU = TI(src,getU);
    // usz srcS = cs;   while (srcS>0 && o2cu(srcGetU(src,srcS-1))!='\n') srcS--;
    // usz srcE = srcS; while (srcE<srcL) { u32 chr = o2cu(srcGetU(src, srcE)); if(chr=='\n')break; printUTF8(chr); srcE++; }
    // if (ce>srcE) ce = srcE;
    // cs-= srcS; ce-= srcS;
    // putchar('\n');
    // for (i32 i = 0; i < cs+start; i++) putchar(' ');
    // for (i32 i = cs; i < ce; i++) putchar('^');
    // putchar('\n');
    B s = emptyCVec();
    printRaw(vm_fmtPoint(src, s, comp->path, cs, ce));
    putchar('\n');
  } else {
    if (pos!=-1) printf(N64d": ", pos);
    printf("source unknown\n");
  }
}

NOINLINE void vm_pst(Env* s, Env* e) { // e not included
  assert(s<=e);
  i64 l = e-s;
  i64 i = l-1;
  while (i>=0) {
    Env* c = s+i;
    if (l>30 && i==l-10) {
      printf("("N64d" entries omitted)\n", l-20);
      i = 10;
    }
    Comp* comp = c->sc->body->bl->comp;
    i32 bcPos = c->pos&1? ((u32)c->pos)>>1 : BCPOS(c->sc->body, (u32*)c->pos);
    vm_printPos(comp, bcPos, i);
    i--;
  }
}
NOINLINE void vm_pstLive() {
  vm_pst(envStart, envCurr+1);
}

void unwindEnv(Env* envNew) {
  assert(envNew<=envCurr);
  while (envCurr!=envNew) {
    if ((envCurr->pos&1) == 0) envCurr->pos = (BCPOS(envCurr->sc->body, (u32*)envCurr->pos)<<1) | 1;
    envCurr--;
  }
}
void unwindCompiler() {
  #if UNWIND_COMPILER
    unwindEnv(envStart+comp_currEnvPos);
  #endif
}

NOINLINE void printErrMsg(B msg) {
  if (isArr(msg)) {
    BS2B msgGetU = TI(msg,getU);
    usz msgLen = a(msg)->ia;
    for (usz i = 0; i < msgLen; i++) if (!isC32(msgGetU(msg,i))) goto base;
    printRaw(msg);
    return;
  }
  base:
  print(msg);
}


NOINLINE NORETURN void thr(B msg) {
  // printf("gStack %p-%p:\n", gStackStart, gStack); B* c = gStack;
  // while (c>gStackStart) { print(*--c); putchar('\n'); } printf("gStack printed\n");
  if (cf>cfStart) {
    catchMessage = msg;
    cf--;
    
    B* gStackNew = gStackStart + cf->gsDepth;
    assert(gStackNew<=gStack);
    while (gStack!=gStackNew) dec(*--gStack);
    envPrevHeight = envCurr-envStart + 1;
    unwindEnv(envStart + cf->envDepth - 1);
    
    
    if (cfStart+cf->cfDepth > cf) err("bad catch cfDepth");
    cf = cfStart+cf->cfDepth;
    longjmp(cf->jmp, 1);
  }
  assert(cf==cfStart);
  printf("Error: "); printErrMsg(msg); putchar('\n'); fflush(stdout);
  Env* envEnd = envCurr+1;
  unwindEnv(envStart-1);
  vm_pst(envCurr+1, envEnd);
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
