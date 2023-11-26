#if PROFILE_IP
  #define _GNU_SOURCE 1
  #include <sys/ucontext.h>
#endif

#include "core.h"
#include "vm.h"
#include "ns.h"
#include "utils/utf.h"
#include "utils/talloc.h"
#include "utils/interrupt.h"
#include "load.h"
#include <unistd.h>

#ifndef UNWIND_COMPILER // whether to hide stackframes of the compiler in compiling errors
  #define UNWIND_COMPILER 1
#endif

#define FOR_BC(F) F(PUSH) F(DYNO) F(DYNM) F(LSTO) F(LSTM) F(ARMO) F(ARMM) F(FN1C) F(FN2C) F(MD1C) F(MD2C) F(TR2D) \
                  F(TR3D) F(SETN) F(SETU) F(SETM) F(SETC) F(POPS) F(DFND) F(FN1O) F(FN2O) F(CHKV) F(TR3O) \
                  F(MD2R) F(MD2L) F(VARO) F(VARM) F(VFYM) F(SETH) F(RETN) F(FLDO) F(FLDM) F(ALIM) F(NOTM) F(RETD) F(SYSV) F(VARU) F(PRED) \
                  F(EXTO) F(EXTM) F(EXTU) F(FLDG) F(ADDI) F(ADDU) F(FN1Ci)F(FN1Oi)F(FN2Ci)F(FN2Oi) \
                  F(SETNi)F(SETUi)F(SETMi)F(SETCi)F(SETNv)F(SETUv)F(SETMv)F(SETCv)F(PRED1)F(PRED2)F(SETH1)F(SETH2) \
                  F(DFND0)F(DFND1)F(DFND2)F(FAIL)


char* bc_repr(u32 p) {
  switch(p) { default: return "(unknown)";
    #define F(X) case X: return #X;
    FOR_BC(F)
    #undef F
  }
}
void print_BC(FILE* f, u32* p, i32 w) {
  char* str = bc_repr(*p);
  fprintf(f, "%s", str);
  u32* n = nextBC(p);
  p++;
  i32 len = strlen(str);
  while(p!=n) {
    u32 c = (u32)*p++;
    char buf[8];
    i32 clen = 0;
    do {
      buf[clen++] = (c&15)>9? 'A'+(c&15)-10 : '0'+(c&15);
      c>>= 4;
    } while(c);
    fputc(' ', f);
    for (i32 i = 0; i < clen; i++) fputc(buf[clen-i-1], f);
    len+= clen+1;
  }
  len = w-len;
  while(len-->0) fputc(' ', f);
}
void print_BCStream(FILE* f, u32* p) {
  while(true) {
    print_BC(f, p, 10); fputc(10, f);
    if (*p == RETD || *p == RETN) return;
    p = nextBC(p);
  }
}


B thrownMsg;
u64 envPrevHeight;

Env* envCurr; // pointer to current environment; included to make for simpler current position updating
Env* envStart;
Env* envEnd;

B* gStack; // points to after end
B* gStackStart;
B* gStackEnd;
NOINLINE void gsReserveR(u64 am) { gsReserve(am); }
void print_gStack() {
  B* c = gStackStart;
  i32 i = 0;
  printf("gStack %p, height "N64d":\n", gStackStart, (i64)(gStack-gStackStart));
  while (c!=gStack) {
    printf("  %d: ", i); fflush(stdout);
    printI(*c); fflush(stdout);
    if (isVal(*c)) printf(", refc=%d", v(*c)->refc);
    printf("\n");
    fflush(stdout);
    c++;
    i++;
  }
}

B listVars(Scope* sc) {
  #if ONLY_NATIVE_COMP
    return emptyHVec();
  #endif
  Body* b = sc->body;
  if (b==NULL) return bi_N;
  
  B r = emptyHVec();
  usz am0 = sc->varAm;
  if (am0) {
    B nameList = b->bl->comp->nameList; SGetU(nameList);
    i32* varData = b->varData; usz bam = b->varAm;
    for (u64 i = 0; i < am0; i++) {
      i32 nameID = varData[i + bam];
      r = vec_addN(r, incG(GetU(nameList, nameID)));
    }
  }
  if (sc->ext) {
    ScopeExt* scExt = sc->ext; usz am = scExt->varAm; B* vars = scExt->vars;
    for (u64 i = 0; i < am; i++) r = vec_addN(r, incG(vars[i+am]));
  }
  return r;
}


Body* m_body(i32 vam, i32 pos, u32 maxStack, u16 maxPSC) {
  Body* body = mm_alloc(fsizeof(Body, varData, i32, vam*2), t_body);
  
  #if JIT_START != -1
    body->nvm = NULL;
    body->nvmRefs = m_f64(0);
  #endif
  #if JIT_START > 0
    body->callCount = 0;
  #endif
  body->bcTmp = pos;
  body->maxStack = maxStack;
  body->exists = true;
  body->maxPSC = maxPSC;
  body->bl = NULL;
  body->varAm = (u16)vam;
  body->nsDesc = NULL;
  return body;
}



typedef struct NextRequest {
  u32 off; // offset into bytecode where the two integers must be inserted
  u32 pos1; // offset into bodyI/bodyMap of what's wanted for monadic
  u32 pos2; // ↑ for dyadic; U32_MAX if not wanted
} NextRequest;

static B emptyARMM;

Block* compileBlock(B block, Comp* comp, bool* bDone, u32* bc, usz bcIA, B allBlocks, B allBodies, B nameList, Scope* sc, i32 depth, i32 myPos, i32 nsResult) {
  assert(sc!=NULL || nsResult==0);
  usz blIA = IA(block);
  if (blIA!=3) thrM("VM compiler: Bad block info size");
  SGetU(block)
  usz  ty  = o2s(GetU(block,0)); if (ty>2) thrM("VM compiler: Bad type");
  bool imm = o2b(GetU(block,1));
  B    bodyObj = GetU(block,2);
  i32 argAm = argCount(ty, imm);
  
  TSALLOC(i32, newBC, 20); // transformed bytecode
  TSALLOC(i32, mapBC, 20); // map of original bytecode to transformed
  TSALLOC(Block*, usedBlocks, 2); // list of blocks to be referenced by DFND, stored in result->blocks
  TSALLOC(Body*, bodies, 2); // list of bodies of this block
  
  // failed match body
  TSADD(newBC, FAIL);
  TSADD(mapBC, myPos);
  Body* failBody = m_body(6, 0, 1, 0);
  failBody->nsDesc = NULL;
  failBody->exists = false;
  TSADD(bodies, failBody);
  i32 failBodyI = 0;
  Body* startBodies[6] = {failBody,failBody,failBody,failBody,failBody,failBody};
  
  bool boArr = isArr(bodyObj);
  i32 boCount = boArr? IA(bodyObj) : 1;
  if (boCount<1 || boCount>5) thrM("VM compiler: Unexpected body list length");
  // if (boArr) { printI(bodyObj); putchar('\n'); }
  i32 firstMPos = failBodyI;
  
  for (i32 i = 0; i < 6; i+= 2) {
    if (i >= boCount) break;
    
    i32* bodyPs;
    i32 mCount, dCount, mapLen;
    if (isArr(bodyObj)) {
      SGetU(bodyObj)
      B b1, b2;
      if (i==4) {
        b1 = bi_emptyHVec;
        b2 = GetU(bodyObj, 4);
      } else {
        b1 = GetU(bodyObj, i);
        b2 = i+1<boCount? GetU(bodyObj, i+1) : bi_emptyHVec;
      }
      if (!isArr(b1) || !isArr(b2)) thrM("VM compiler: Body list contained non-arrays");
      mCount = IA(b1); SGetU(b1)
      dCount = IA(b2); SGetU(b2)
      mapLen = mCount+dCount;
      bodyPs = TALLOCP(i32, mapLen+2);
      i32* bodyM = bodyPs;
      i32* bodyD = bodyPs + mCount+1;
      for (i32 i = 0; i < mCount; i++) bodyM[i] = o2i(GetU(b1, i));
      for (i32 i = 0; i < dCount; i++) bodyD[i] = o2i(GetU(b2, i));
      for (i32 i = 1; i < mCount; i++) if (bodyM[i]<=bodyM[i-1]) thrM("VM compiler: Expected body indices to be sorted");
      for (i32 i = 1; i < dCount; i++) if (bodyD[i]<=bodyD[i-1]) thrM("VM compiler: Expected body indices to be sorted");
      bodyM[mCount] = bodyD[dCount] = I32_MAX;
    } else {
      mapLen = 2;
      bodyPs = TALLOCP(i32, mapLen+2);
      bodyPs[0] = bodyPs[2] = o2i(bodyObj);
      bodyPs[1] = bodyPs[3] = I32_MAX;
      mCount = dCount = 1;
    }
    // for (int i = 0; i < mapLen+2; i++) printf("%d ", bodyPs[i]); putchar('\n'); printf("things: %d %d\n", mCount, dCount);
    
    TALLOC(Body*, bodyMap, mapLen+2); // map from index in bodyPs to the corresponding body
    TSALLOC(NextRequest, bodyReqs, 10); // list of SETH/PRED-s to fill out when bodyMap is complete
    
    i32 pos1 = 0; // pos1 and pos2 always stay valid indexes in bodyPs because bodyPs is padded with -1s
    i32 pos2 = mCount+1;
    bodyMap[mCount] = bodyMap[mapLen+1] = NULL;
    bool firstM = true;
    bool firstD = true;
    
    while (true) {
      i32 curr1 = bodyPs[pos1];
      i32 curr2 = bodyPs[pos2];
      i32 currBody = curr1<curr2? curr1 : curr2;
      if (currBody==I32_MAX) break;
      // printf("step %d %d:  %d %d %d\n", pos1, pos2, curr1, curr2, currBody);
      bool is1 = curr1==currBody; if (is1) pos1++;
      bool is2 = curr2==currBody; if (is2) pos2++;
      // printf("idxs: %d %d\n", index1, firstD);
      
      
      B bodyRepr = IGetU(allBodies, currBody); if (!isArr(bodyRepr)) thrM("VM compiler: Body array contained non-array");
      usz boIA = IA(bodyRepr); if (boIA!=2 && boIA!=4) thrM("VM compiler: Body array had invalid length");
      SGetU(bodyRepr)
      usz idx = o2s(GetU(bodyRepr,0)); if (idx>=bcIA) thrM("VM compiler: Bytecode index out of bounds");
      usz vam = o2s(GetU(bodyRepr,1)); if (vam!=(u16)vam) thrM("VM compiler: >2⋆16 variables not supported"); // TODO any reason for this? 2⋆32 vars should just work, no? // oh, some size fields are u16s. but i doubt those change much, or even make things worse
      
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
          B varIDs = GetU(bodyRepr,2);
          for (i32 i = oSZ; i < nSZ; i++) {
            nE->vars[i] = bi_noVar;
            nE->vars[i+nSZ] = IGet(nameList, o2s(IGetU(varIDs, regAm+i)));
          }
          sc->ext = nE;
        }
      }
      i32 bcStart = TSSIZE(newBC);
      u32* c;
      
      bool remapArgs = false;
      c = bc+idx;
      while (*c!=RETN & *c!=RETD) {
        if (*c==PRED) { remapArgs = true; break; }
        c = nextBC(c);
        if (c-bc-1 >= bcIA) thrM("VM compiler: No RETN/RETD found before end of bytecode");
      }
      if (remapArgs) {
        if (sc && depth==0) thrM("Predicates cannot be used directly in a REPL");
        c = bc+idx;
        bool argUsed[6] = {0,0,0,0,0,0};
        while (*c!=RETN & *c!=RETD) {
          if (*c==VARO | *c==VARM | *c==VARU) if (c[1]==0 && c[2]<argAm) argUsed[c[2]] = true;
          c = nextBC(c);
          if (c-bc-1 >= bcIA) thrM("VM compiler: No RETN/RETD found before end of bytecode");
        }
        for (i32 i = 0; i < 6; i++) if (argUsed[i]) {
          TSADDA(newBC, ((u32[]){ VARO,0,i, VARM,0,vam+i, SETN, POPS }), 8);
          TSADDA(mapBC, ((u32[]){ 0,0,0,    0,0,0,        0   , 0    }), 8);
        }
      }
      
      c = bc+idx;
      while (true) {
        u32* n = nextBC(c);
        bool ret = false;
        #define A64(X) { u64 a64=(X); TSADD(newBC, (u32)a64); TSADD(newBC, a64>>32); }
        switch (*c) {
          case ARMM: { u32 len = c[1];
            if (0 == len) {
              TSADD(newBC, ADDI);
              A64(emptyARMM.u);
            } else {
              TSADD(newBC, ARMM);
              TSADD(newBC, len);
            }
            break;
          }
          case PUSH:;
            B obj = comp->objs->a[c[1]];
            TSADD(newBC, isVal(obj)? ADDI : ADDU);
            A64(obj.u);
            break;
          case NOTM:
            TSADD(newBC, ADDU);
            A64(bi_N.u);
            break;
          case RETN: if(h!=1) thrM("VM compiler: RETN expected to be called with one item on the stack");
            TSADD(newBC, RETN);
            ret = true;
            break;
          case RETD: if(h!=1&h!=0) thrM("VM compiler: RETD expected to be called with no more than 1 item on the stack");
            if (nsResult!=0 && depth==0) {
              if (nsResult==-1) thrM("Cannot construct a namespace for a REPL result");
              assert(nsResult==1);
              if (h==0) thrM("No value for REPL expression to return");
              TSADD(newBC, RETN);
            } else {
              if (h==1) TSADD(newBC, POPS);
              TSADD(newBC, RETD);
            }
            ret = true;
            break;
          case DFND: {
            u32 id = c[1];
            if ((u32)id >= IA(allBlocks)) thrM("VM compiler: DFND index out-of-bounds");
            if (bDone[id]) thrM("VM compiler: DFND of the same block in multiple places");
            bDone[id] = true;
            Block* bl = compileBlock(IGetU(allBlocks,id), comp, bDone, bc, bcIA, allBlocks, allBodies, nameList, sc, depth+1, c-bc, 0);
            TSADD(newBC, bl->ty==0? DFND0 : bl->ty==1? DFND1 : DFND2);
            A64(ptr2u64(bl));
            TSADD(usedBlocks, bl);
            break;
          }
          case VARO: case VARM: case VARU: {
            i32 ins = c[0];
            i32 cdepth = c[1];
            i32 cpos = c[2];
            if (cdepth+1 > mpsc) mpsc = cdepth+1;
            if (sc && cdepth>=depth) {
              Scope* csc = sc;
              for (i32 i = depth; i < cdepth; i++) if (!(csc = csc->psc)) thrM("VM compiler: VAR_ has an out-of-bounds depth");
              if (cpos >= csc->varAm) {
                cpos-= csc->varAm;
                ins = ins==VARO? EXTO : ins==VARM? EXTM : EXTO;
              }
            }
            if (remapArgs && cpos<argAm && cdepth==0) cpos+= vam;
            TSADD(newBC, ins);
            TSADD(newBC, cdepth);
            TSADD(newBC, cpos);
            break;
          }
          case SETH: case PRED:
            if (*c==PRED && h!=1) thrM("VM compiler: PRED expected to be called with one item on the stack");
            if (mpsc<1) mpsc=1; // SETH and PRED may want to have a parent scope pointer
            TSADD(newBC, *c==SETH? (imm? SETH1 : SETH2) : imm? PRED1 : PRED2);
            TSADD(bodyReqs, ((NextRequest){.off = TSSIZE(newBC), .pos1 = pos1, .pos2 = imm? U32_MAX : pos2}));
            A64(0); if(!imm) A64(0); // to be filled in by later bodyReqs handling
            break;
          case FLDO: TSADD(newBC, FLDG); TSADD(newBC, str2gid(IGetU(nameList, c[1]))); break;
          case ALIM: TSADD(newBC, ALIM); TSADD(newBC, str2gid(IGetU(nameList, c[1]))); break;
          default: {
            u32* ccpy = c;
            while (ccpy!=n) TSADD(newBC, *ccpy++);
            break;
          }
        }
        #undef A64
        usz nlen = TSSIZE(newBC)-TSSIZE(mapBC);
        for (usz i = 0; i < nlen; i++) TSADD(mapBC, c-bc);
        if (h-stackConsumed(c)<0) thrM("VM compiler: Stack size goes negative");
        h+= stackDiff(c);
        if (h>hM) hM = h;
        if (ret) break;
        c = n;
      }
      
      if (mpsc>U16_MAX) thrM("VM compiler: Block too deep");
      
      i32 finalVam = vam+(remapArgs? argAm : 0);
      Body* body = m_body(finalVam, bcStart, (u32)hM, mpsc);
      if (boIA>2) {
        m_nsDesc(body, imm, ty, finalVam, nameList, GetU(bodyRepr,2), GetU(bodyRepr,3));
      } else {
        for (u64 i = 0; i < vam*2; i++) body->varData[i] = -1;
      }
      
      if (is1) { bodyMap[pos1-1] = body; if (firstM) { firstM=false; startBodies[i  ] = body; if(i==0) firstMPos = TSSIZE(bodies); } }
      if (is2) { bodyMap[pos2-1] = body; if (firstD) { firstD=false; startBodies[i+1] = body; } }
      TSADD(bodies, body);
    }
    u64 bodyReqAm = TSSIZE(bodyReqs);
    for (u64 i = 0; i < bodyReqAm; i++) {
      NextRequest r = bodyReqs[i];
      /*ugly, but whatever*/ u64 v1 = ptr2u64(bodyMap[r.pos1]); newBC[r.off+0] = (u32)v1; newBC[r.off+1] = v1>>32;
      if (r.pos2!=U32_MAX) { u64 v2 = ptr2u64(bodyMap[r.pos2]); newBC[r.off+2] = (u32)v2; newBC[r.off+3] = v2>>32; }
    }
    TSFREE(bodyReqs);
    TFREE(bodyMap);
    TFREE(bodyPs);
  }
  
  
  
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
  bl->comp = ptr_inc(comp);
  bl->ty = (u8)ty;
  bl->bc = nbc;
  bl->blocks = nBl==NULL? NULL : nBl->a;
  bl->map = map;
  bl->imm = imm;
  bl->bodyCount = bodyCount;
  
  bl->dyBody   = startBodies[1];
  bl->invMBody = startBodies[2];
  bl->invXBody = startBodies[3];
  bl->invWBody = startBodies[5];
  
  if (firstMPos != 0) { // swap body 0 and firstMPos so that the first body is the first monadic one
    Body* t = bodies[0];
    bodies[0] = bodies[firstMPos];
    bodies[firstMPos] = t;
  }
  
  for (i32 i = 0; i < bodyCount; i++) {
    bl->bodies[i] = bodies[i];
    bodies[i]->bc = (u32*)nbc + bodies[i]->bcTmp;
    bodies[i]->bl = ptr_inc(bl);
  }
  TSFREE(bodies);
  return bl;
}

// consumes all; assumes arguments are valid (verifies some stuff, but definitely not everything)
// if sc isn't NULL, this block must only be evaluated directly in that scope precisely once
NOINLINE Block* compileAll(B bcq, B objs, B allBlocks, B allBodies, B indices, B tokenInfo, B src, B path, Scope* sc, i32 nsResult) {
  usz bIA = IA(allBlocks);
  I32Arr* bca = toI32Arr(bcq);
  u32* bc = (u32*)bca->a;
  usz bcIA = PIA(bca);
  Comp* comp = mm_alloc(sizeof(Comp), t_comp);
  NOGC_S;
  comp->indices = indices;
  comp->src = src;
  comp->path = path;
  B nameList;
  if (q_N(tokenInfo)) {
    nameList = emptyHVec();
  } else {
    B t = IGetU(tokenInfo,2);
    nameList = IGet(t,0);
  }
  comp->nameList = nameList;
  comp->blockAm = 0;
  comp->objs = NULL;
  NOGC_E;
  // and now finally it's safe to allocate stuff
  HArr* objArr = (HArr*)cpyHArr(objs);
  comp->objs = objArr;
  usz objAm = PIA(objArr);
  for (usz i = 0; i < objAm; i++) { B* c=objArr->a+i; B v=*c; *c=m_f64(0); *c = squeeze_deep(v); }
  
  if (!q_N(src) && !q_N(indices)) {
    if (isAtm(indices) || RNK(indices)!=1 || IA(indices)!=2) thrM("VM compiler: Bad indices");
    for (i32 i = 0; i < 2; i++) {
      B ind = IGetU(indices,i);
      if (isAtm(ind) || RNK(ind)!=1 || IA(ind)!=bcIA) thrM("VM compiler: Bad indices");
      SGetU(ind)
      for (usz j = 0; j < bcIA; j++) o2i(GetU(ind,j));
    }
  }
  TALLOC(bool,bDone,bIA);
  for (usz i = 0; i < bIA; i++) bDone[i] = false;
  Block* ret = compileBlock(IGetU(allBlocks, 0), comp, bDone, bc, bcIA, allBlocks, allBodies, nameList, sc, 0, 0, nsResult);
  TFREE(bDone);
  ptr_dec(comp); decG(allBlocks); decG(allBodies); dec(tokenInfo); decG(taga(bca));
  return ret;
}



FORCE_INLINE bool v_merge(Scope* pscs[], B s, B x, bool upd, bool hdr) {
  assert(TY(s) == t_arrMerge);
  B o = c(WrappedObj,s)->obj;
  if (!isArr(x) || RNK(x)==0) thrF("[…]%c𝕩: 𝕩 cannot have rank 0", upd? U'↩' : U'←');
  
  B* op = harr_ptr(o);
  usz oia = IA(o);
  
  if (SH(x)[0] != oia) {
    if (hdr) return false;
    else thrF("[…]%c𝕩: Target length & leading axis of 𝕩 didn't match", upd? U'↩' : U'←');
  }
  if (oia == 0) { /*no need to do anything*/ }
  else if (RNK(x)==1) {
    SGet(x)
    for (usz i = 0; i < oia; i++) {
      B cx = m_unit(Get(x,i));
      if (!hdr) {
        v_set(pscs, op[i], cx, upd, true, false, true);
      } else {
        bool ok = v_seth(pscs, op[i], cx);
        decG(cx);
        if (!ok) return false;
      }
    }
  } else {
    B cells = toCells(incG(x));
    B* xp = harr_ptr(cells);
    for (usz i = 0; i < oia; i++) {
      if (!hdr) v_set (pscs, op[i], xp[i], upd, true, false, false);
      else if (!v_seth(pscs, op[i], xp[i])) { dec(cells); return false; }
    }
    dec(cells);
  }
  return true;
}

NOINLINE void v_setF(Scope* pscs[], B s, B x, bool upd) {
  if (isArr(s)) { VTY(s, t_harr);
    B* sp = harr_ptr(s);
    usz ia = IA(s);
    if (isAtm(x) || !eqShape(s, x)) {
      if (!isNsp(x)) thrM("Assignment: Mismatched shape for spread assignment");
      for (u64 i = 0; i < ia; i++) {
        B c = sp[i];
        if (isVar(c)) {
          Scope* sc = pscs[V_DEPTH(c)];
          v_set(pscs, c, ns_getU(x, pos2gid(sc->body, V_POS(c))), upd, true, false, false);
        } else if (isExt(c)) {
          ScopeExt* ext = pscs[V_DEPTH(c)]->ext;
          v_set(pscs, c, ns_getNU(x, ext->vars[V_POS(c) + ext->varAm], true), upd, true, false, false);
        } else if (isObj(c)) {
          assert(TY(c) == t_fldAlias);
          FldAlias* cf = c(FldAlias,c);
          v_set(pscs, cf->obj, ns_getU(x, cf->p), upd, true, false, false);
        } else thrM("Assignment: extracting non-name from namespace");
      }
      return;
    }
    SGet(x)
    for (u64 i = 0; i < ia; i++) v_set(pscs, sp[i], Get(x,i), upd, true, false, true);
  } else if (s.u == bi_N.u) {
    return;
  } else if (isObj(s)) {
    if      (TY(s) == t_arrMerge) v_merge(pscs, s, x, upd, false);
    else if (TY(s) == t_fldAlias) thrF("Assignment: Cannot assign non-namespace to a list containing aliases");
    else UD;
  } else if (isExt(s)) {
    Scope* sc = pscs[V_DEPTH(s)];
    B prev = sc->ext->vars[V_POS(s)];
    if (upd) {
      if (prev.u==bi_noVar.u) thrM("↩: Updating undefined variable");
      dec(prev);
    } else dec(prev);
    sc->ext->vars[V_POS(s)] = inc(x);
  } else UD;
}
NOINLINE bool v_sethF(Scope* pscs[], B s, B x) {
  if (isArr(s)) {
    VTY(s, t_harr);
    B* sp = harr_ptr(s);
    usz ia = IA(s);
    if (isAtm(x) || !eqShape(s, x)) {
      if (!isNsp(x)) return false;
      for (u64 i = 0; i < ia; i++) {
        B c = sp[i];
        if (isVar(c)) {
          Scope* sc = pscs[V_DEPTH(c)];
          B g = ns_qgetU(x, pos2gid(sc->body, V_POS(c)));
          if (q_N(g) || !v_seth(pscs, c, g)) return false;
        } else if (isObj(c) && TY(c)==t_fldAlias) {
          FldAlias* cf = c(FldAlias,c);
          B g = ns_qgetU(x, cf->p);
          if (q_N(g) || !v_seth(pscs, cf->obj, g)) return false;
        } else return false;
      }
      return true;
    }
    SGetU(x)
    for (u64 i = 0; i < ia; i++) if (!v_seth(pscs, sp[i], GetU(x,i))) return false;
    return true;
  }
  if (TY(s)==t_vfyObj) return equal(c(WrappedObj,s)->obj,x);
  if (TY(s)==t_arrMerge) return v_merge(pscs, s, x, false, true);
  if (TY(s)==t_fldAlias) return false;
  UD;
}



NOINLINE B v_getF(Scope* pscs[], B s) {
  if (isArr(s)) {
    VTY(s, t_harr);
    usz ia = IA(s);
    B* sp = harr_ptr(s);
    M_HARR(r, ia);
    for (u64 i = 0; i < ia; i++) HARR_ADD(r, i, v_get(pscs, sp[i], true));
    return HARR_FV(r);
  } else if (isExt(s)) {
    Scope* sc = pscs[V_DEPTH(s)];
    B r = sc->ext->vars[V_POS(s)];
    if (r.u==bi_noVar.u) thrM("↩: Reading variable that hasn't been set");
    sc->ext->vars[V_POS(s)] = bi_optOut;
    return r;
  } else {
    assert(isObj(s) && TY(s)==t_arrMerge);
    return bqn_merge(v_getF(pscs, c(WrappedObj,s)->obj), 2);
  }
}

FORCE_INLINE Scope* m_scopeI(Body* body, Scope* psc, u16 varAm, i32 initVarAm, B* initVars, bool smallInit) { // consumes initVarAm items of initVars
  Scope* sc = mm_alloc(fsizeof(Scope, vars, B, varAm), t_scope);
  sc->body = ptr_inc(body);
  sc->psc = psc; if (psc) ptr_inc(psc);
  sc->varAm = varAm;
  sc->ext = NULL;
  i32 i = 0;
  if (smallInit) {
    switch(initVarAm) { default: UD;
      case 6: sc->vars[5] = initVars[5];
      case 5: sc->vars[4] = initVars[4];
      case 4: sc->vars[3] = initVars[3];
      case 3: sc->vars[2] = initVars[2];
      case 2: sc->vars[1] = initVars[1];
      case 1: sc->vars[0] = initVars[0];
      case 0:;
    }
    i = initVarAm;
  } else {
    PLAINLOOP while (i<initVarAm) { sc->vars[i] = initVars[i]; i++; }
  }
  
  // some bit of manual unrolling, but not too much
  u32 left = varAm-i;
  if (left==1) sc->vars[i] = bi_noVar;
  else if (left>=2) {
    B* vars = sc->vars+i;
    PLAINLOOP for (u32 i = 0; i < (left>>1); i++) { *(vars++) = bi_noVar; *(vars++) = bi_noVar; }
    if (left&1) *vars = bi_noVar;
  }
  
  return sc;
}

NOINLINE void scope_decF(Scope* sc) {
  i32 varAm = sc->varAm;
  i32 innerRef = 1;
  for (i32 i = 0; i < varAm; i++) {
    B c = sc->vars[i];
    if (isVal(c) && v(c)->refc==1) {
      u8 t = TY(c);
      if      (t==t_funBl && c(FunBlock,c)->sc==sc) innerRef++;
      else if (t==t_md1Bl && c(Md1Block,c)->sc==sc) innerRef++;
      else if (t==t_md2Bl && c(Md2Block,c)->sc==sc) innerRef++;
    }
  }
  assert(innerRef <= sc->refc);
  if (innerRef==sc->refc) scope_freeF((Value*) sc);
  else sc->refc--; // refc>0 guaranteed by refc!=1 from scope_dec
}

FORCE_INLINE B gotoNextBody(Block* bl, Scope* sc, Body* body) {
  if (body==NULL) thrF("No header matched argument%S", q_N(sc->vars[2])?"":"s");
  
  popEnv();
  
  i32 ga = blockGivenVars(bl);
  
  for (u64 i = 0; i < ga; i++) inc(sc->vars[i]);
  assert(sc->psc!=NULL);
  Scope* nsc = m_scopeI(body, sc->psc, body->varAm, ga, sc->vars, true);
  scope_dec(sc);
  return execBodyInplaceI(body, nsc, bl);
}

#if DEBUG_VM
i32 bcDepth=-2;
i32* vmStack;
i32 bcCtr = 0;
#endif
#define BCPOS(B,P) (B->bl->map[(P)-(u32*)B->bl->bc])
B evalBC(Body* b, Scope* sc, Block* bl) { // doesn't consume
  #if DEBUG_VM
    bcDepth+= 2;
    if (!vmStack) vmStack = malloc(400);
    i32 stackNum = bcDepth>>1;
    vmStack[stackNum] = -1;
    fprintf(stderr,"new eval\n");
    B* origStack = gStack;
  #endif
  u32* bc = b->bc;
  pushEnv(sc, bc);
  gsReserve(b->maxStack);
  Scope* pscs[b->maxPSC]; // -fsanitize=undefined complains when this is 0. ¯\_(ツ)_/¯
  if (b->maxPSC) {
    pscs[0] = sc;
    for (i32 i = 1; i < b->maxPSC; i++) pscs[i] = pscs[i-1]->psc;
  }
  #ifdef GS_REALLOC
    #define POP (*--gStack)
    #define P(N) B N=POP;
    #define ADD(X) { B tr=X; *(gStack++) = tr; }
    #define PEEK(X) gStack[-(X)]
    #define STACK_HEIGHT
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
    #define POS_UPD envCurr->pos = ptr2u64(bc-1);
  #else
    #define POS_UPD
  #endif
  
  while(true) {
    #if DEBUG_VM
      u32* sbc = bc;
      i32 bcPos = BCPOS(b,sbc);
      vmStack[stackNum] = bcPos;
      for(i32 i = 0; i < bcDepth; i++) fprintf(stderr," ");
      print_BC(stderr,sbc,20); fprintf(stderr,"@%d  in: ",bcPos);
      for (i32 i = 0; i < lgStack-origStack; i++) { if(i)fprintf(stderr,"; "); fprintI(stderr,origStack[i]); } fputc('\n',stderr); fflush(stderr);
      bcCtr++;
      for (i32 i = 0; i < sc->varAm; i++) VALIDATE(sc->vars[i]);
    #endif
    switch(*bc++) {
      case POPS: dec(POP); break;
      case PUSH: {
        ADD(inc(bl->comp->objs->a[*bc++]));
        break;
      }
      case ADDI: {
        ADD(incG(b(L64)));
        break;
      }
      case ADDU: {
        ADD(b(L64));
        break;
      }
      case FN1C: { P(f)P(x)
        GS_UPD;POS_UPD;
        ADD(c1(f, x)); dec(f);
        break;
      }
      case FN1O: { P(f)P(x)
        GS_UPD;POS_UPD;
        ADD(q_N(x)? x : c1(f, x)); dec(f);
        break;
      }
      case FN2C: { P(w)P(f)P(x)
        GS_UPD;POS_UPD;
        ADD(c2(f, w, x)); dec(f);
        break;
      }
      case FN2O: { P(w)P(f)P(x)
        GS_UPD;POS_UPD;
        if (q_N(x)) { dec(w); ADD(x); }
        else ADD(q_N(w)? c1(f, x) : c2(f, w, x));
        dec(f);
        break;
      }
      case LSTO: case LSTM: { GS_UPD;
        u32 sz = *bc++;
        if (sz==0) {
          ADD(emptyHVec());
        } else {
          HArr_p r = m_harrUv(sz);
          bool allNum = true;
          for (i64 i = 0; i < sz; i++) if (!isNum(r.a[sz-i-1] = POP)) allNum = false;
          NOGC_E;
          if (allNum) {
            GS_UPD;
            ADD(num_squeeze(r.b));
          } else ADD(r.b);
        }
        break;
      }
      case DFND0: { GS_UPD;POS_UPD; ADD(evalFunBlock(TOPTR(Block,L64), sc)); break; }
      case DFND1: { GS_UPD;POS_UPD; ADD(m_md1Block  (TOPTR(Block,L64), sc)); break; }
      case DFND2: { GS_UPD;POS_UPD; ADD(m_md2Block  (TOPTR(Block,L64), sc)); break; }
      
      case MD1C: { P(f)P(m)     GS_UPD;POS_UPD; ADD(m1_d  (m,f  )); break; }
      case MD2C: { P(f)P(m)P(g) GS_UPD;POS_UPD; ADD(m2_d  (m,f,g)); break; }
      case TR2D: {     P(g)P(h) GS_UPD;         ADD(m_atop(  g,h)); break; }
      case TR3D: { P(f)P(g)P(h) GS_UPD;         ADD(m_fork(f,g,h)); break; }
      case TR3O: { P(f)P(g)P(h) GS_UPD;
        if (q_N(f)) { ADD(m_atop(g,h)); dec(f); }
        else ADD(m_fork(f,g,h));
        break;
      }
      
      case VARM: { u32 d = *bc++; u32 p = *bc++;
        ADD(tagu64((u64)d<<32 | (u32)p, VAR_TAG));
        break;
      }
      case VARO: { u32 d = *bc++; u32 p = *bc++;
        B l = pscs[d]->vars[p];
        if(l.u==bi_noVar.u) { POS_UPD; thrM("Reading variable before its defined"); }
        ADD(inc(l));
        break;
      }
      case VARU: { u32 d = *bc++; u32 p = *bc++;
        B* vars = pscs[d]->vars;
        ADD(vars[p]);
        vars[p] = bi_optOut;
        break;
      }
      
      case EXTM: { u32 d = *bc++; u32 p = *bc++;
        ADD(tagu64((u64)d<<32 | (u32)p, EXT_TAG));
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
      
      case SETN: { P(s)    P(x) GS_UPD; POS_UPD; v_set(pscs, s, x, false, true, true, false); ADD(x); break; }
      case SETU: { P(s)    P(x) GS_UPD; POS_UPD; v_set(pscs, s, x, true,  true, true, false); ADD(x); break; }
      case SETM: { P(s)P(f)P(x) GS_UPD; POS_UPD;
        B w = v_get(pscs, s, true);
        B r = c2(f,w,x); dec(f);
        v_set(pscs, s, r, true, false, true, false);
        ADD(r);
        break;
      }
      case SETC: { P(s)P(f) GS_UPD; POS_UPD;
        B x = v_get(pscs, s, true);
        B r = c1(f,x); dec(f);
        v_set(pscs, s, r, true, false, true, false);
        ADD(r);
        break;
      }
      
      case SETH1:{ P(s)    P(x) GS_UPD; POS_UPD; u64 v1 = L64;
        bool ok = v_seth(pscs, s, x); dec(x); dec(s);
        if (!ok) { return gotoNextBody(bl, sc, TOPTR(Body, v1)); }
        break;
      }
      case SETH2:{ P(s)    P(x) GS_UPD; POS_UPD; u64 v1 = L64; u64 v2 = L64;
        bool ok = v_seth(pscs, s, x); dec(x); dec(s);
        if (!ok) { return gotoNextBody(bl, sc, TOPTR(Body, q_N(sc->vars[2])? v1 : v2)); }
        break;
      }
      case PRED1:{ P(x) GS_UPD; POS_UPD; u64 v1 = L64;
        if (!o2b(x)) { return gotoNextBody(bl, sc, TOPTR(Body, v1)); }
        break;
      }
      case PRED2:{ P(x) GS_UPD; POS_UPD; u64 v1 = L64; u64 v2 = L64;
        if (!o2b(x)) { return gotoNextBody(bl, sc, TOPTR(Body, q_N(sc->vars[2])? v1 : v2)); }
        break;
      }
      
      case FLDG: { P(ns) GS_UPD; u32 p = *bc++; POS_UPD;
        if (!isNsp(ns)) thrM("Trying to read a field from non-namespace");
        ADD(inc(ns_getU(ns, p)));
        dec(ns);
        break;
      }
      case ALIM: { P(o) GS_UPD; u32 l = *bc++;
        FldAlias* a = mm_alloc(sizeof(FldAlias), t_fldAlias);
        a->obj = o;
        a->p = l;
        ADD(tag(a,OBJ_TAG));
        break;
      }
      case CHKV: {
        if (q_N(PEEK(1))) { GS_UPD; POS_UPD; thrM("Unexpected Nothing (·)"); }
        break;
      }
      case VFYM: { P(o) GS_UPD;
        WrappedObj* a = mm_alloc(sizeof(WrappedObj), t_vfyObj);
        a->obj = o;
        ADD(tag(a,OBJ_TAG));
        break;
      }
      case FAIL: thrM(q_N(sc->vars[2])? "This block cannot be called monadically" : "This block cannot be called dyadically");
      case ARMO: { GS_UPD;
        POS_UPD;
        u32 sz = *bc++;
        assert(sz>0);
        HArr_p r = m_harrUv(sz);
        for (i64 i = 0; i < sz; i++) r.a[sz-i-1] = POP;
        NOGC_E; GS_UPD;
        ADD(bqn_merge(r.b, 2));
        break;
      }
      case ARMM: { GS_UPD;
        u32 sz = *bc++;
        assert(sz>0);
        HArr_p r = m_harrUv(sz);
        for (i64 i = 0; i < sz; i++) r.a[sz-i-1] = POP;
        NOGC_E; GS_UPD;
        WrappedObj* a = mm_alloc(sizeof(WrappedObj), t_arrMerge);
        a->obj = r.b;
        ADD(tag(a,OBJ_TAG));
        break;
      }
      
      case RETD: { GS_UPD;
        ADD(m_ns(ptr_inc(sc), ptr_inc(b->nsDesc)));
        goto end;
      }
      case RETN: goto end;
      
      default:
        #if DEBUG
          printf("todo %d\n", bc[-1]); bc++; break;
        #else
          UD;
        #endif
    }
    #if DEBUG_VM
      for(i32 i = 0; i < bcDepth; i++) fprintf(stderr," ");
      print_BC(stderr,sbc,20); fprintf(stderr,"@%d out: ",BCPOS(b, sbc));
      for (i32 i = 0; i < lgStack-origStack; i++) { if(i)fprintf(stderr,"; "); fprintI(stderr,origStack[i]); } fputc('\n',stderr); fflush(stderr);
    #endif
  }
  end:;
  #if DEBUG_VM
    bcDepth-= 2;
  #endif
  B r = POP;
  GS_UPD;
  popEnv();
  scope_dec(sc);
  return r;
  #undef L64
  #undef P
  #undef ADD
  #undef POP
  #undef POS_UPD
  #undef GS_UPD
}

NOINLINE Scope* m_scope(Body* body, Scope* psc, u16 varAm, i32 initVarAm, B* initVars) { // consumes initVarAm items of initVars
  return m_scopeI(body, psc, varAm, initVarAm, initVars, false);
}

B execBlockInplaceImpl(Body* body, Scope* sc, Block* block) { return execBodyInplaceI(block->bodies[0], sc, block); }

#if JIT_START != -1
B mnvmExecBodyInplace(Body* body, Scope* sc) {
  Nvm_res r = m_nvm(body);
  body->nvm = r.p;
  body->nvmRefs = r.refs;
  return evalJIT(body, sc, body->nvm);
}
#endif



#if REPL_INTERRUPT
#include <signal.h>
volatile int cbqn_interrupted;
static void interrupt_sigHandler(int x) {
  if (cbqn_interrupted) abort(); // shouldn't happen
  cbqn_takeInterrupts(false);
  cbqn_interrupted = 1;
}
bool cbqn_takeInterrupts(bool b) { // returns if succeeded
  if (!b) cbqn_interrupted = 0; // can be left dangling if nothing caught it
  struct sigaction act = {};
  act.sa_handler = b? interrupt_sigHandler : SIG_DFL;
  return sigaction(SIGINT, &act, NULL) == 0;
}

NOINLINE NORETURN void cbqn_onInterrupt() {
  cbqn_interrupted = 0;
  thrM("interrupted");
}
#else
bool cbqn_takeInterrupts(bool b) { return false; }
#endif


FORCE_INLINE B execBlock(Block* block, Body* body, Scope* psc, i32 ga, B* svar) { // consumes svar contents
  CHECK_INTERRUPT;
  u16 varAm = body->varAm;
  assert(varAm>=ga);
  assert(ga == blockGivenVars(block));
  Scope* sc = m_scopeI(body, psc, varAm, ga, svar, true);
  B r = execBodyInplaceI(body, sc, block);
  return r;
}

B funBl_c1(B     t,      B x) { FunBlock* b=c(FunBlock, t);   ptr_inc(b); return execBlock(b->bl, b->bl->bodies[0], b->sc, 3, (B[]){t,              x, bi_N                                                   }); }
B funBl_c2(B     t, B w, B x) { FunBlock* b=c(FunBlock, t);   ptr_inc(b); return execBlock(b->bl, b->bl->dyBody,    b->sc, 3, (B[]){t,              x, w                                                      }); }
B md1Bl_c1(Md1D* d,      B x) { Md1Block* b=(Md1Block*)d->m1; ptr_inc(d); return execBlock(b->bl, b->bl->bodies[0], b->sc, 5, (B[]){tag(d,FUN_TAG), x, bi_N, tag(ptr_inc(d->m1),MD1_TAG), inc(d->f)           }); }
B md1Bl_c2(Md1D* d, B w, B x) { Md1Block* b=(Md1Block*)d->m1; ptr_inc(d); return execBlock(b->bl, b->bl->dyBody,    b->sc, 5, (B[]){tag(d,FUN_TAG), x, w   , tag(ptr_inc(d->m1),MD1_TAG), inc(d->f)           }); }
B md2Bl_c1(Md2D* d,      B x) { Md2Block* b=(Md2Block*)d->m2; ptr_inc(d); return execBlock(b->bl, b->bl->bodies[0], b->sc, 6, (B[]){tag(d,FUN_TAG), x, bi_N, tag(ptr_inc(d->m2),MD2_TAG), inc(d->f), inc(d->g)}); }
B md2Bl_c2(Md2D* d, B w, B x) { Md2Block* b=(Md2Block*)d->m2; ptr_inc(d); return execBlock(b->bl, b->bl->dyBody,    b->sc, 6, (B[]){tag(d,FUN_TAG), x, w   , tag(ptr_inc(d->m2),MD2_TAG), inc(d->f), inc(d->g)}); }

static NOINLINE NORETURN void noInv(Body* bo, Scope* psc, i8 type, i8 inv) {
  pushEnv(m_scope(bo, psc, 0, 0, (B[]){}), bo->bc);
  thrF("No %U undo header found for this%U block", inv==0? "monadic" : inv==1? "dyadic F˜⁼" : "dyadic F⁼", type==0? "" : type==1? " 1-modifier" : " 2-modifier");
}
B funBl_im(B     t,      B x) { FunBlock* b=c(FunBlock, t);   Body* bo=b->bl->invMBody; if (!bo->exists) noInv(bo,b->sc,0,0); ptr_inc(b); return execBlock(b->bl, bo, b->sc, 3, (B[]){t,              x, bi_N}); }
B funBl_iw(B     t, B w, B x) { FunBlock* b=c(FunBlock, t);   Body* bo=b->bl->invWBody; if (!bo->exists) noInv(bo,b->sc,0,1); ptr_inc(b); return execBlock(b->bl, bo, b->sc, 3, (B[]){t,              x, w   }); }
B funBl_ix(B     t, B w, B x) { FunBlock* b=c(FunBlock, t);   Body* bo=b->bl->invXBody; if (!bo->exists) noInv(bo,b->sc,0,2); ptr_inc(b); return execBlock(b->bl, bo, b->sc, 3, (B[]){t,              x, w   }); }
B md1Bl_im(Md1D* d,      B x) { Md1Block* b=(Md1Block*)d->m1; Body* bo=b->bl->invMBody; if (!bo->exists) noInv(bo,b->sc,1,0); ptr_inc(d); return execBlock(b->bl, bo, b->sc, 5, (B[]){tag(d,FUN_TAG), x, bi_N, tag(ptr_inc(d->m1),MD1_TAG), inc(d->f)}); }
B md1Bl_iw(Md1D* d, B w, B x) { Md1Block* b=(Md1Block*)d->m1; Body* bo=b->bl->invWBody; if (!bo->exists) noInv(bo,b->sc,1,1); ptr_inc(d); return execBlock(b->bl, bo, b->sc, 5, (B[]){tag(d,FUN_TAG), x, w   , tag(ptr_inc(d->m1),MD1_TAG), inc(d->f)}); }
B md1Bl_ix(Md1D* d, B w, B x) { Md1Block* b=(Md1Block*)d->m1; Body* bo=b->bl->invXBody; if (!bo->exists) noInv(bo,b->sc,1,2); ptr_inc(d); return execBlock(b->bl, bo, b->sc, 5, (B[]){tag(d,FUN_TAG), x, w   , tag(ptr_inc(d->m1),MD1_TAG), inc(d->f)}); }
B md2Bl_im(Md2D* d,      B x) { Md2Block* b=(Md2Block*)d->m2; Body* bo=b->bl->invMBody; if (!bo->exists) noInv(bo,b->sc,2,0); ptr_inc(d); return execBlock(b->bl, bo, b->sc, 6, (B[]){tag(d,FUN_TAG), x, bi_N, tag(ptr_inc(d->m2),MD2_TAG), inc(d->f), inc(d->g)}); }
B md2Bl_iw(Md2D* d, B w, B x) { Md2Block* b=(Md2Block*)d->m2; Body* bo=b->bl->invWBody; if (!bo->exists) noInv(bo,b->sc,2,1); ptr_inc(d); return execBlock(b->bl, bo, b->sc, 6, (B[]){tag(d,FUN_TAG), x, w   , tag(ptr_inc(d->m2),MD2_TAG), inc(d->f), inc(d->g)}); }
B md2Bl_ix(Md2D* d, B w, B x) { Md2Block* b=(Md2Block*)d->m2; Body* bo=b->bl->invXBody; if (!bo->exists) noInv(bo,b->sc,2,2); ptr_inc(d); return execBlock(b->bl, bo, b->sc, 6, (B[]){tag(d,FUN_TAG), x, w   , tag(ptr_inc(d->m2),MD2_TAG), inc(d->f), inc(d->g)}); }

B md1Bl_d(B m, B f     ) { Md1Block* c = c(Md1Block,m); Block* bl=c(Md1Block, m)->bl; return c->bl->imm? execBlock(bl, bl->bodies[0], c(Md1Block, m)->sc, 2, (B[]){m, f   }) : m_md1D((Md1*)c,f  ); }
B md2Bl_d(B m, B f, B g) { Md2Block* c = c(Md2Block,m); Block* bl=c(Md2Block, m)->bl; return c->bl->imm? execBlock(bl, bl->bodies[0], c(Md2Block, m)->sc, 3, (B[]){m, f, g}) : m_md2D((Md2*)c,f,g); }

B evalFunBlock(Block* bl, Scope* psc) { // doesn't consume anything
  if (bl->imm) return execBlock(bl, bl->bodies[0], psc, 0, NULL);
  FunBlock* r = mm_alloc(sizeof(FunBlock), t_funBl);
  r->bl = ptr_inc(bl);
  r->sc = ptr_inc(psc);
  r->c1 = funBl_c1;
  r->c2 = funBl_c2;
  return tag(r,FUN_TAG);
}
B m_md1Block(Block* bl, Scope* psc) {
  Md1Block* r = mm_alloc(sizeof(Md1Block), t_md1Bl);
  r->bl = ptr_inc(bl);
  r->sc = ptr_inc(psc);
  r->c1 = md1Bl_c1;
  r->c2 = md1Bl_c2;
  return tag(r,MD1_TAG);
}
B m_md2Block(Block* bl, Scope* psc) {
  Md2Block* r = mm_alloc(sizeof(Md2Block), t_md2Bl);
  r->bl = ptr_inc(bl);
  r->sc = ptr_inc(psc);
  r->c1 = md2Bl_c1;
  r->c2 = md2Bl_c2;
  return tag(r,MD2_TAG);
}

DEF_FREE(body) {
  Body* c = (Body*)x;
  #if JIT_START!=-1
    if(c->nvm) nvm_free(c->nvm);
    dec(c->nvmRefs);
  #endif
  if(c->nsDesc) ptr_decR(c->nsDesc);
  if(c->bl) ptr_decR(c->bl);
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
DEF_FREE(comp)  { Comp*     c = (Comp    *)x; if (c->objs!=NULL) ptr_decR(c->objs); decR(c->src); decR(c->indices); decR(c->path); decR(c->nameList); }
DEF_FREE(funBl) { FunBlock* c = (FunBlock*)x; ptr_dec(c->sc); ptr_decR(c->bl); }
DEF_FREE(md1Bl) { Md1Block* c = (Md1Block*)x; ptr_dec(c->sc); ptr_decR(c->bl); }
DEF_FREE(md2Bl) { Md2Block* c = (Md2Block*)x; ptr_dec(c->sc); ptr_decR(c->bl); }
DEF_FREE(alias) { dec(((FldAlias*  )x)->obj); }
DEF_FREE(wrobj) { dec(((WrappedObj*)x)->obj); }
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
  if(c->bl) mm_visitP(c->bl);
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
void  comp_visit(Value* x) { Comp*     c = (Comp    *)x; if (c->objs!=NULL) mm_visitP(c->objs); mm_visit(c->src); mm_visit(c->indices); mm_visit(c->path); mm_visit(c->nameList); }
void funBl_visit(Value* x) { FunBlock* c = (FunBlock*)x; mm_visitP(c->sc); mm_visitP(c->bl); }
void md1Bl_visit(Value* x) { Md1Block* c = (Md1Block*)x; mm_visitP(c->sc); mm_visitP(c->bl); }
void md2Bl_visit(Value* x) { Md2Block* c = (Md2Block*)x; mm_visitP(c->sc); mm_visitP(c->bl); }
void alias_visit(Value* x) { mm_visit(((FldAlias*  )x)->obj); }
void wrobj_visit(Value* x) { mm_visit(((WrappedObj*)x)->obj); }
void bBlks_visit(Value* x) { BlBlocks* c = (BlBlocks*)x; u16 am = c->am; for (i32 i = 0; i < am; i++) mm_visitP(c->a[i]); }
void scExt_visit(Value* x) { ScopeExt* c = (ScopeExt*)x; u16 am = c->varAm*2; for (i32 i = 0; i < am; i++) mm_visit(c->vars[i]); }

void comp_print (FILE* f, B x) { fprintf(f,"(%p: comp)",v(x)); }
void body_print (FILE* f, B x) { fprintf(f,"(%p: body varam=%d)",v(x),c(Body,x)->varAm); }
void block_print(FILE* f, B x) { fprintf(f,"(%p: block)",v(x)); }
void scope_print(FILE* f, B x) { fprintf(f,"(%p: scope; vars:",v(x));Scope*sc=c(Scope,x);for(u64 i=0;i<sc->varAm;i++){fprintf(f," ");fprintI(f,sc->vars[i]);}fprintf(f,")"); }
void alias_print(FILE* f, B x) { fprintf(f,"(alias %d of ", c(FldAlias,x)->p); fprintI(f,c(FldAlias,x)->obj); fprintf(f,")"); }
void vfymO_print(FILE* f, B x) { fprintI(f,c(FldAlias,x)->obj); }
void marrO_print(FILE* f, B x) { fprintf(f,"["); fprintI(f,c(FldAlias,x)->obj); fprintf(f,"]"); }
void bBlks_print(FILE* f, B x) { fprintf(f,"(block list)"); }
void scExt_print(FILE* f, B x) { fprintf(f,"(scope extension with %d vars)", c(ScopeExt,x)->varAm); }

// void funBl_print(FILE* f, B x) { fprintf(f,"(%p: function"" block bl=%p sc=%p)",v(x),c(FunBlock,x)->bl,c(FunBlock,x)->sc); }
// void md1Bl_print(FILE* f, B x) { fprintf(f,"(%p: 1-modifier block bl=%p sc=%p)",v(x),c(Md1Block,x)->bl,c(Md1Block,x)->sc); }
// void md2Bl_print(FILE* f, B x) { fprintf(f,"(%p: 2-modifier block bl=%p sc=%p)",v(x),c(Md2Block,x)->bl,c(Md2Block,x)->sc); }
// void funBl_print(FILE* f, B x) { fprintf(f,"(function"" block @%d)",c(FunBlock,x)->bl->body->map[0]); }
// void md1Bl_print(FILE* f, B x) { fprintf(f,"(1-modifier block @%d)",c(Md1Block,x)->bl->body->map[0]); }
// void md2Bl_print(FILE* f, B x) { fprintf(f,"(2-modifier block @%d)",c(Md2Block,x)->bl->body->map[0]); }
void funBl_print(FILE* f, B x) { fprintf(f,"{function"" block}"); }
void md1Bl_print(FILE* f, B x) { fprintf(f,"{1-modifier block}"); }
void md2Bl_print(FILE* f, B x) { fprintf(f,"{2-modifier block}"); }

B block_decompose(B x) { return m_hvec2(m_i32(1), x); }

#if !defined(_WIN32) && !defined(_WIN64)
static usz pageSizeV;
#endif

usz getPageSize() {
  #if defined(_WIN32) || defined(_WIN64)
    #if !NO_MMAP
      #error "Windows builds must have NO_MMAP=1"
    #endif
    return 1; // doesn't actually need to be accurate if NO_MMAP, which Windows builds should have
  #else
    if (pageSizeV==0) pageSizeV = sysconf(_SC_PAGESIZE);
    return pageSizeV;
  #endif
}

static void allocStack(void** curr, void** start, void** end, i32 elSize, i32 count) {
  usz ps = getPageSize();
  u64 sz = (elSize*count + ps-1)/ps * ps;
  assert(sz%elSize == 0);
  #if NO_MMAP
  void* mem = calloc(sz+ps, 1);
  #else
  void* mem = mmap(NULL, sz+ps, PROT_READ|PROT_WRITE, MAP_NORESERVE|MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  if (*curr == MAP_FAILED) fatal("Failed to allocate stack");
  #endif
  *curr = *start = mem;
  *end = ((char*)*start)+sz;
  #if !WASM && !NO_MMAP
  mprotect(*end, ps, PROT_NONE); // idk first way i found to force erroring on overflow
  #endif
}
void print_vmStack() {
  #if DEBUG_VM
    printf("vm stack:");
    for (i32 i = 0; i < (bcDepth>>1) + 1; i++) { printf(" %d", vmStack[i]); fflush(stdout); }
    printf("\n"); fflush(stdout);
  #endif
}

B oomMessage;


u32 bL_m[BC_SIZE] = { // bytecode length map
  [FN1C]=1, [FN2C]=1, [FN1O]=1, [FN2O]=1,
  [MD1C]=1, [MD2C]=1, [MD2R]=1,
  [TR2D]=1, [TR3D]=1, [TR3O]=1,
  [SETN]=1, [SETU]=1, [SETM]=1, [SETH]=1, [SETC]=1,
  [POPS]=1, [CHKV]=1, [VFYM]=1, [NOTM]=1, [RETN]=1, [RETD]=1,
  [FAIL]=1, [PRED]=1,
  
  [PUSH]=2, [DFND]=2, [LSTO]=2, [LSTM]=2, [ARMO]=2, [ARMM]=2,
  [DYNO]=2, [DYNM]=2, [FLDO]=2, [FLDG]=2, [FLDM]=2,
  [SYSV]=2, [ALIM]=2,
  
  [VARO]=3, [VARM]=3, [VARU]=3,
  [EXTO]=3, [EXTM]=3, [EXTU]=3,
  [ADDI]=3, [ADDU]=3,
  [FN1Ci]=3, [FN1Oi]=3, [FN2Ci]=3, [DFND0]=3, [DFND1]=3, [DFND2]=3,
  [SETNi]=3, [SETUi]=3, [SETMi]=3, [SETCi]=3,
  [SETNv]=3, [SETUv]=3, [SETMv]=3, [SETCv]=3, [PRED1]=3, [SETH1]=3,
  
  [FN2Oi]=5, [SETH2]=5, [PRED2]=5,
};
i32 sD_m[BC_SIZE] = { // stack diff map
  [PUSH ]= 1, [DYNO ]= 1, [DYNM]= 1, [DFND]= 1, [VARO]= 1, [VARM]= 1, [DFND0]= 1, [DFND1]=1, [DFND2]=1,
  [VARU ]= 1, [EXTO ]= 1, [EXTM]= 1, [EXTU]= 1, [SYSV]= 1, [ADDI]= 1, [ADDU ]= 1, [NOTM ]= 1,
  [FN1Ci]= 0, [FN1Oi]= 0, [CHKV]= 0, [VFYM]= 0, [FLDO]= 0, [FLDG]= 0, [FLDM]= 0, [RETD ]= 0, [ALIM ]=0,
  [FN2Ci]=-1, [FN2Oi]=-1, [FN1C]=-1, [FN1O]=-1, [MD1C]=-1, [TR2D]=-1, [POPS ]=-1, [MD2R ]=-1, [RETN]=-1, [PRED]=-1, [PRED1]=-1, [PRED2]=-1,
  [MD2C ]=-2, [TR3D ]=-2, [FN2C]=-2, [FN2O]=-2, [TR3O]=-2, [SETH]=-2, [SETH1]=-2, [SETH2]=-2,
  
  [SETN]=-1, [SETNi]= 0, [SETNv]=-1,
  [SETU]=-1, [SETUi]= 0, [SETUv]=-1,
  [SETC]=-1, [SETCi]= 0, [SETCv]=-1,
  [SETM]=-2, [SETMi]=-1, [SETMv]=-2,
  
  [FAIL]=0
};
i32 sC_m[BC_SIZE] = { // stack consumed map
  [PUSH]=0, [DYNO]=0, [DYNM]=0, [DFND]=0, [VARO ]=0,[VARM ]=0,[NOTM ]=0, [VARU]=0, [EXTO]=0, [EXTM]=0,
  [EXTU]=0, [SYSV]=0, [ADDI]=0, [ADDU]=0, [DFND0]=0,[DFND1]=0,[DFND2]=0,
  
  [CHKV ]=0,[RETD ]=0,
  [FN1Ci]=1,[FN1Oi]=1, [FLDO]=1, [FLDG]=1, [FLDM]=1, [ALIM]=1, [RETN]=1, [POPS]=1, [PRED]=1, [PRED1]=1, [PRED2]=1, [VFYM]=1,
  [FN2Ci]=2,[FN2Oi]=2, [FN1C]=2, [FN1O]=2, [MD1C]=2, [TR2D]=2, [MD2R]=2, [SETH]=2, [SETH1]=2, [SETH2]=2,
  [MD2C ]=3,[TR3D ]=3, [FN2C]=3, [FN2O]=3, [TR3O]=3,
  
  [SETN]=2, [SETNi]=1, [SETNv]=1,
  [SETU]=2, [SETUi]=1, [SETUv]=1,
  [SETC]=2, [SETCi]=1, [SETCv]=1,
  [SETM]=3, [SETMi]=2, [SETMv]=2,
  
  [FAIL]=0
};
i32 sA_m[BC_SIZE]; // stack added map

B funBl_uc1(B t, B o, B x) {
  return funBl_im(t, c1(o, c1(t, x)));
}

void comp_init(void) {
  TIi(t_comp    ,freeO) =  comp_freeO; TIi(t_comp    ,freeF) =  comp_freeF; TIi(t_comp    ,visit) = comp_visit;  TIi(t_comp    ,print) =  comp_print;
  TIi(t_body    ,freeO) =  body_freeO; TIi(t_body    ,freeF) =  body_freeF; TIi(t_body    ,visit) = body_visit;  TIi(t_body    ,print) =  body_print;
  TIi(t_block   ,freeO) = block_freeO; TIi(t_block   ,freeF) = block_freeF; TIi(t_block   ,visit) = block_visit; TIi(t_block   ,print) = block_print;
  TIi(t_scope   ,freeO) = scope_freeO; TIi(t_scope   ,freeF) = scope_freeF; TIi(t_scope   ,visit) = scope_visit; TIi(t_scope   ,print) = scope_print;
  TIi(t_scopeExt,freeO) = scExt_freeO; TIi(t_scopeExt,freeF) = scExt_freeF; TIi(t_scopeExt,visit) = scExt_visit; TIi(t_scopeExt,print) = scExt_print;
  TIi(t_blBlocks,freeO) = bBlks_freeO; TIi(t_blBlocks,freeF) = bBlks_freeF; TIi(t_blBlocks,visit) = bBlks_visit; TIi(t_blBlocks,print) = bBlks_print;
  TIi(t_fldAlias,freeO) = alias_freeO; TIi(t_fldAlias,freeF) = alias_freeF; TIi(t_fldAlias,visit) = alias_visit; TIi(t_fldAlias,print) = alias_print;
  TIi(t_vfyObj  ,freeO) = wrobj_freeO; TIi(t_vfyObj  ,freeF) = wrobj_freeF; TIi(t_vfyObj  ,visit) = wrobj_visit; TIi(t_vfyObj  ,print) = vfymO_print;
  TIi(t_arrMerge,freeO) = wrobj_freeO; TIi(t_arrMerge,freeF) = wrobj_freeF; TIi(t_arrMerge,visit) = wrobj_visit; TIi(t_arrMerge,print) = marrO_print;
  TIi(t_funBl   ,freeO) = funBl_freeO; TIi(t_funBl   ,freeF) = funBl_freeF; TIi(t_funBl   ,visit) = funBl_visit; TIi(t_funBl   ,print) = funBl_print; TIi(t_funBl,decompose) = block_decompose;
  TIi(t_md1Bl   ,freeO) = md1Bl_freeO; TIi(t_md1Bl   ,freeF) = md1Bl_freeF; TIi(t_md1Bl   ,visit) = md1Bl_visit; TIi(t_md1Bl   ,print) = md1Bl_print; TIi(t_md1Bl,decompose) = block_decompose; TIi(t_md1Bl,m1_d)=md1Bl_d;
  TIi(t_md2Bl   ,freeO) = md2Bl_freeO; TIi(t_md2Bl   ,freeF) = md2Bl_freeF; TIi(t_md2Bl   ,visit) = md2Bl_visit; TIi(t_md2Bl   ,print) = md2Bl_print; TIi(t_md2Bl,decompose) = block_decompose; TIi(t_md2Bl,m2_d)=md2Bl_d;
  
  TIi(t_funBl,fn_uc1) = funBl_uc1;
  TIi(t_funBl,fn_im) = funBl_im; TIi(t_md1Bl,m1_im) = md1Bl_im; TIi(t_md2Bl,m2_im) = md2Bl_im;
  TIi(t_funBl,fn_iw) = funBl_iw; TIi(t_md1Bl,m1_iw) = md1Bl_iw; TIi(t_md2Bl,m2_iw) = md2Bl_iw;
  TIi(t_funBl,fn_ix) = funBl_ix; TIi(t_md1Bl,m1_ix) = md1Bl_ix; TIi(t_md2Bl,m2_ix) = md2Bl_ix;
  
  oomMessage = m_c8vec_0("Out of memory"); gc_add(oomMessage);
  WrappedObj* arm0 = mm_alloc(sizeof(WrappedObj), t_arrMerge);
  arm0->obj = emptyHVec();
  emptyARMM = tag(arm0, OBJ_TAG);
  gc_add(emptyARMM);
  
  
  #ifndef GS_REALLOC
    allocStack((void**)&gStack, (void**)&gStackStart, (void**)&gStackEnd, sizeof(B), GS_SIZE);
  #endif
  allocStack((void**)&envCurr, (void**)&envStart, (void**)&envEnd, sizeof(Env), ENV_SIZE);
  envCurr--;
  
  for (i32 i = 0; i < BC_SIZE; i++) sA_m[i] = sD_m[i] + sC_m[i];
  sA_m[LSTO]=1; sA_m[ARMO]=1;
  sA_m[LSTM]=1; sA_m[ARMM]=1;
}



typedef struct CatchFrame {
  #if CATCH_ERRORS
    jmp_buf jmp;
  #endif
  u64 gsDepth;
  u64 envDepth;
  u64 cfDepth;
} CatchFrame;
CatchFrame* cf; // points to after end
CatchFrame* cfStart;
CatchFrame* cfEnd;

#if CATCH_ERRORS
jmp_buf* prepareCatch() {
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
  assert(cf>cfStart);
  cf--;
}
#endif

NOINLINE B vm_fmtPoint(B src, B prepend, B path, usz cs, usz ce) { // consumes prepend
  SGetU(src)
  usz srcL = IA(src);
  usz srcS = cs;
  while (srcS>0 && o2cG(GetU(src,srcS-1))!='\n') srcS--;
  usz srcE = srcS;
  while (srcE<srcL) { if(o2cG(GetU(src, srcE))=='\n') break; srcE++; }
  if (ce>srcE) ce = srcE;
  
  i64 ln = 1;
  for (usz i = 0; i < srcS; i++) if(o2cG(GetU(src, i))=='\n') ln++;
  B s = prepend;
  if (isArr(path) && (IA(path)>1 || (IA(path)==1 && IGetU(path,0).u!=m_c32('.').u))) AFMT("%R:%l:\n  ", path, ln);
  else AFMT("at ");
  i64 padEnd = (i64)IA(s);
  i64 padStart = padEnd;
  SGetU(s)
  while (padStart>0 && o2cG(GetU(s,padStart-1))!='\n') padStart--;
  
  AJOIN(taga(arr_shVec(TI(src,slice)(incG(src),srcS, srcE-srcS))));
  cs-= srcS;
  ce-= srcS;
  ACHR('\n');
  for (i64 i = padStart; i < padEnd; i++) ACHR(' ');
  for (u64 i = 0; i < cs; i++) ACHR(o2cG(GetU(src, srcS+i))=='\t'? '\t' : ' '); // ugh tabs
  for (u64 i = cs; i < ce; i++) ACHR('^');
  return s;
}

extern bool cbqn_initialized;
NOINLINE void vm_printPos(Comp* comp, i32 bcPos, i64 pos) {
  B src = comp->src;
  if (!q_N(src) && !q_N(comp->indices)) {
    B inds = IGetU(comp->indices, 0); usz cs = o2s(IGetU(inds,bcPos));
    B inde = IGetU(comp->indices, 1); usz ce = o2s(IGetU(inde,bcPos))+1;
    // printf("  bcPos=%d\n", bcPos);       // in case the pretty error generator is broken
    // printf(" inds:%d…%d\n", cs, ce);
    
    
    // want to try really hard to print errors
    if (!cbqn_initialized) goto native_print;
    #if FORCE_NATIVE_ERROR_PRINT
      goto native_print;
    #endif
    if (CATCH) { freeThrown(); goto native_print; }
    
    B msg = vm_fmtPoint(src, emptyCVec(), comp->path, cs, ce);
    fprintsB(stderr, msg);
    dec(msg);
    fputc('\n', stderr);
    popCatch();
    return;
    
native_print:
    freeThrown();
    int start = fprintf(stderr, "at ");
    usz srcL = IA(src);
    SGetU(src)
    usz srcS = cs;   while (srcS>0 && o2cG(GetU(src,srcS-1))!='\n') srcS--;
    usz srcE = srcS; while (srcE<srcL) { u32 chr = o2cG(GetU(src, srcE)); if(chr=='\n')break; fprintCodepoint(stderr, chr); srcE++; }
    if (ce>srcE) ce = srcE;
    cs-= srcS; ce-= srcS;
    fputc('\n', stderr);
    for (i32 i = 0; i < cs+start; i++) fputc(' ', stderr);
    for (i32 i = cs; i < ce; i++) fputc('^', stderr);
    fputc('\n', stderr);
    return;
    
    //print_BCStream((u32*)i32arr_ptr(comp->bc)+bcPos);
  } else {
    #if DEBUG
      if (pos!=-1) fprintf(stderr, N64d": ", pos);
      fprintf(stderr, "source unknown\n");
    #endif
  }
}

NOINLINE void vm_pst(Env* s, Env* e) { // e not included
  assert(s<=e);
  i64 l = e-s;
  i64 i = l-1;
  while (i>=0) {
    Env* c = s+i;
    if (l>30 && i==l-10) {
      fprintf(stderr, "("N64d" entries omitted)\n", l-20);
      i = 10;
    }
    Comp* comp = c->sc->body->bl->comp;
    i32 bcPos = c->pos&1? ((u32)c->pos)>>1 : BCPOS(c->sc->body, TOPTR(u32, c->pos));
    vm_printPos(comp, bcPos, i);
    i--;
  }
}
NOINLINE void vm_pstLive() {
  vm_pst(envStart, envCurr+1);
}


#if __has_include(<sys/time.h>) && __has_include(<signal.h>) && !NO_MMAP && !WASM
#include <sys/time.h>
#include <signal.h>
#define PROFILE_BUFFER (1ULL<<25) // number of `Profiler_ent`s

#define PROFILE_BUFFER_CHECK \
  Profiler_ent* bn = profiler_buf_c+1; \
  if (RARE(bn>=profiler_buf_e)) { profile_buf_full = true; return; }

typedef union Profiler_ent {
  struct {
    Comp* comp;
    usz bcPos;
  };
  u64 ip;
} Profiler_ent;
Profiler_ent* profiler_buf_s;
Profiler_ent* profiler_buf_c;
Profiler_ent* profiler_buf_e;
bool profile_buf_full;


void profiler_bc_handler(int x) {
  PROFILE_BUFFER_CHECK;
  
  if (envCurr<envStart) return;
  Env e = *envCurr;
  Comp* comp = e.sc->body->bl->comp;
  i32 bcPos = e.pos&1? ((u32)e.pos)>>1 : BCPOS(e.sc->body, TOPTR(u32, e.pos));
  
  *profiler_buf_c = (Profiler_ent){.comp = ptr_inc(comp), .bcPos = bcPos};
  profiler_buf_c = bn;
}

#if PROFILE_IP
void profiler_ip_handler(int x, siginfo_t* info, void* context) {
  PROFILE_BUFFER_CHECK;
  
  ucontext_t* ctx = (ucontext_t*)context;
  u64 ptr;
  #if __x86_64__
    ptr = ctx->uc_mcontext.gregs[REG_RIP];
  #elif __aarch64__
    ptr = ctx->uc_mcontext.pc;
  #else
    #error "don't know how to get instruction pointer on current arch"
  #endif
  *profiler_buf_c = (Profiler_ent){.ip = ptr};
  profiler_buf_c = bn;
}
NOINLINE B gsc_exec_inplace(B src, B path, B args);
#endif

static bool setProfHandler(i32 mode) {
  struct sigaction act = {};
  switch (mode) {
    default: printf("Unsupported profiling mode\n"); return false;
    case 0: act.sa_handler = SIG_DFL; break;
    case 1: act.sa_handler = profiler_bc_handler; break;
    #if PROFILE_IP
    case 2: act.sa_sigaction = profiler_ip_handler; act.sa_flags=SA_SIGINFO;
    #endif
  }
  if (sigaction(SIGALRM/*SIGPROF*/, &act, NULL)) {
    printf("Failed to set profiling signal handler\n");
    return false;
  }
  return true;
}
static bool setProfTimer(i64 us) {
  struct itimerval timer;
  timer.it_value.tv_sec=0;
  timer.it_value.tv_usec=us;
  timer.it_interval.tv_sec=0;
  timer.it_interval.tv_usec=us;
  if (setitimer(ITIMER_REAL/*ITIMER_PROF*/, &timer, NULL)) {
    printf("Failed to start sampling timer\n");
    return false;
  }
  return true;
}

void* profiler_makeMap(void);
i32 profiler_index(void** mapRaw, B comp);
void profiler_freeMap(void* mapRaw);

i32 profiler_mode; // 0: freed; 1: bytecode; 2: instruction pointers
bool profiler_active;

bool profiler_alloc(void) {
  profiler_buf_s = profiler_buf_c = mmap(NULL, PROFILE_BUFFER*sizeof(Profiler_ent), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  if (profiler_buf_s == MAP_FAILED) {
    fprintf(stderr, "Failed to allocate profiler buffer\n");
    return false;
  }
  profiler_buf_e = profiler_buf_s+PROFILE_BUFFER;
  profile_buf_full = false;
  return true;
}
void profiler_free(void) {
  profiler_mode = 0;
  munmap(profiler_buf_s, PROFILE_BUFFER*sizeof(Profiler_ent));
}

bool profiler_start(i32 mode, i64 hz) { // 1: bytecode; 2: instruction pointers
  assert(mode==1 || mode==2);
  i64 us = 999999/hz;
  profiler_mode = mode;
  profiler_active = true;
  return setProfHandler(mode) && setProfTimer(us);
}
bool profiler_stop(void) {
  if (profiler_mode==0) return false;
  profiler_active = false;
  if (profile_buf_full) fprintf(stderr, "Profiler buffer ran out in the middle of execution. Only timings of the first "N64u" samples will be shown.\n", (u64)PROFILE_BUFFER);
  return setProfTimer(0) && setProfHandler(0);
}


static bool isPathREPL(B path) {
  return isArr(path) && IA(path)==1 && IGetU(path,0).u==m_c32('.').u;
}
usz profiler_getResults(B* compListRes, B* mapListRes, bool keyPath) {
  if (profiler_mode!=1) fatal("profiler_getResults called on mode!=1");
  Profiler_ent* c = profiler_buf_s;
  
  B compList = emptyHVec();
  B mapList = emptyHVec();
  usz compCount = 0;
  void* map = profiler_makeMap();
  
  while (c!=profiler_buf_c) {
    usz bcPos = c->bcPos;
    Comp* comp = c->comp;
    B path = comp->path;
    i32 idx = profiler_index(&map, q_N(path) || isPathREPL(path)? tag(comp, OBJ_TAG) : path);
    if (idx == compCount) {
      compList = vec_addN(compList, tag(comp, OBJ_TAG));
      i32* rp;
      usz ia = q_N(comp->src)? 1 : IA(comp->src);
      mapList = vec_addN(mapList, m_i32arrv(&rp, ia));
      for (i32 i = 0; i < ia; i++) rp[i] = 0;
      compCount++;
    }
    usz cs;
    if (q_N(comp->src)) cs = 0;
    else {
      B inds = IGetU(comp->indices, 0); cs = o2s(IGetU(inds,bcPos));
      // B inde = IGetU(comp->indices, 1); ce = o2s(IGetU(inde,bcPos));
    }
    i32* cMap = i32arr_ptr(IGetU(mapList, idx));
    // for (usz i = cs; i <= ce; i++) cMap[i]++;
    cMap[cs]++;
    c++;
  }
  profiler_freeMap(map);
  
  *compListRes = compList;
  *mapListRes = mapList;
  return compCount;
}

void profiler_displayResults(void) {
  ux count = (u64)(profiler_buf_c-profiler_buf_s);
  printf("Got %zu samples\n", count);
  if (profiler_mode==1) {
    B compList, mapList;
    usz compCount = profiler_getResults(&compList, &mapList, true);
    
    SGetU(compList) SGetU(mapList)
    for (usz i = 0; i < compCount; i++) {
      Comp* c = c(Comp, GetU(compList, i));
      B mapObj = GetU(mapList, i);
      i32* m = i32arr_ptr(mapObj);
      
      u64 sum = 0;
      usz ia = IA(mapObj);
      for (usz i = 0; i < ia; i++) sum+= m[i];
      
      if (q_N(c->path)) printf("(anonymous)");
      else if (isPathREPL(c->path)) printf("(REPL)");
      else printsB(c->path);
      if (q_N(c->src)) {
        printf(": "N64d" samples\n", sum);
      } else {
        printf(": "N64d" samples:\n", sum);
        B src = c->src;
        SGetU(src)
        usz sia = IA(src);
        usz pi = 0;
        i32 curr = 0;
        for (usz i = 0; i < sia; i++) {
          u32 c = o2cG(GetU(src, i));
          curr+= m[i];
          if (c=='\n' || i==sia-1) {
            Arr* sl = arr_shVec(TI(src,slice)(incG(src), pi, i-pi+(c=='\n'?0:1)));
            if (curr==0) printf("      │");
            else printf("%6d│", curr);
            printsB(taga(sl));
            printf("\n");
            ptr_dec(sl);
            curr = 0;
            pi = i+1;
          }
        }
      }
    }
    dec(compList);
    dec(mapList);
#if PROFILE_IP
  } else if (profiler_mode==2) {
    f64* rp; B r = m_f64arrv(&rp, count);
    PLAINLOOP for (ux i = 0; i < count; i++) rp[i] = profiler_buf_s[i].ip;
    gsc_exec_inplace(utf8Decode0("profilerResult←•args⋄@"), bi_N, r);
    printf("wrote result to profilerResult\n");
#endif
  } else fatal("profiler_displayResults called with unexpected active mode");
}
#else
bool profiler_alloc() {
  printf("Profiler not supported\n");
  return false;
}
bool profiler_start(i32 mode, i64 hz) { return false; }
bool profiler_stop() { return false; }
void profiler_free() { thrM("Profiler not supported"); }
usz profiler_getResults(B* compListRes, B* mapListRes, bool keyPath) { thrM("Profiler not supported"); }
void profiler_displayResults() { thrM("Profiler not supported"); }
#endif

void unwindEnv(Env* envNew) {
  assert(envNew<=envCurr);
  while (envCurr!=envNew) {
    // if ((envCurr->pos&1) == 0) printf("unwinding %ld\n", (u32*)envCurr->pos - (u32*)envCurr->sc->body->bl->bc);
    // else printf("not unwinding %ld", envCurr->pos>>1);
    if ((envCurr->pos&1) == 0) envCurr->pos = (BCPOS(envCurr->sc->body, TOPTR(u32, envCurr->pos))<<1) | 1;
    envCurr--;
  }
}
void unwindCompiler() {
  #if UNWIND_COMPILER
    unwindEnv(envStart+o2i64(COMPS_CREF(envPos)));
  #endif
}

NOINLINE bool isStr(B x) {
  if (isAtm(x) || RNK(x)!=1) return false;
  if (elChr(TI(x,elType))) return true;
  usz ia = IA(x); SGetU(x)
  for (usz i = 0; i < ia; i++) if (!isC32(GetU(x,i))) return false;
  return true;
}
NOINLINE void printErrMsg(B msg) {
  if (isStr(msg)) fprintsB(stderr, msg);
  else fprintI(stderr, msg);
}


void before_exit(void);
NOINLINE NORETURN void throwImpl(bool rethrow) {
  // printf("gStack %p-%p:\n", gStackStart, gStack); B* c = gStack;
  // while (c>gStackStart) { printI(*--c); putchar('\n'); } printf("gStack printed\n");
  NOGC_CHECK("throwing during noAlloc");
  if (!rethrow) envPrevHeight = envCurr-envStart + 1;
#if CATCH_ERRORS
  if (cf>cfStart) { // something wants to catch errors
    cf--;
    
    B* gStackNew = gStackStart + cf->gsDepth;
    assert(gStackNew<=gStack);
    while (gStack!=gStackNew) dec(*--gStack);
    unwindEnv(envStart + cf->envDepth - 1);
    
    
    if (cfStart+cf->cfDepth > cf) fatal("bad catch cfDepth");
    cf = cfStart+cf->cfDepth;
    longjmp(cf->jmp, 1);
  } else { // uncaught error
#endif
    assert(cf==cfStart);
    fprintf(stderr, "Error: "); printErrMsg(thrownMsg); fputc('\n',stderr); fflush(stderr);
    Env* envEnd = envStart+envPrevHeight;
    unwindEnv(envStart-1);
    vm_pst(envCurr+1, envEnd);
    before_exit();
    #if DEBUG
    __builtin_trap();
    #else
    exit(1);
    #endif
#if CATCH_ERRORS
  }
#endif
}

NOINLINE NORETURN void thr(B msg) {
  thrownMsg = msg;
  throwImpl(false);
}
NOINLINE NORETURN void rethrow() {
  throwImpl(true);
}

NOINLINE void freeThrown() {
  dec(thrownMsg);
  thrownMsg = bi_N;
}

NOINLINE NORETURN void thrM(char* s) {
  NOGC_CHECK("throwing during noAlloc");
  thr(utf8Decode0(s));
}
NOINLINE NORETURN void thrOOM() {
  if (oomMessage.u==0) fatal("out-of-memory encountered before out-of-memory error message object was initialized");
  thr(incG(oomMessage));
}
