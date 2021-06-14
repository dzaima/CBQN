#include "../core.h"
#include "../core/gstack.h"
#include "../ns.h"
#include "../utils/file.h"
#include "../utils/talloc.h"

#ifndef USE_PERF
  #define USE_PERF 0 // enable writing symbols to /tmp/perf-<pid>.map
#endif
#ifndef CSTACK
  #define CSTACK 1
#endif
#ifdef GS_REALLOC
  #undef CSTACK
  #define CSTACK 0
#endif

// separate memory management system for executable code; isn't garbage-collected
#define  BSZ(X) (1ull<<(X))
#define BSZI(X) ((u8)(64-__builtin_clzl((X)-1ull)))
#define  MMI(X) X
#define   BN(X) mmX_##X
#define buckets mmX_buckets
#include "../opt/mm_buddyTemplate.h"
#define  MMI(X) X
#define  ALSZ  17
#define PROT PROT_READ|PROT_WRITE|PROT_EXEC
#define FLAGS MAP_NORESERVE|MAP_PRIVATE|MAP_ANON|MAP_32BIT
#include "../opt/mm_buddyTemplate.c"
static void* mmX_allocN(usz sz, u8 type) { assert(sz>=16); return mmX_allocL(BSZI(sz), type); }
#undef mmX_buckets
#undef BN
#undef BSZI
#undef BSZ


// all the instructions to be called by the generated code
#if CSTACK
  #define GA1 ,B* cStack
  #define GSP (*--cStack)
  #define GS_UPD { gStack=cStack; }
#else
  #define GA1
  #define GSP (*--gStack)
  #define GS_UPD
#endif
#define P(N) B N=GSP;
#if VM_POS
  #define POS_UPD (envCurr-1)->bcL = bc;
#else
  #define POS_UPD
#endif
#define INS NOINLINE
INS void i_POPS(B x) {
  dec(x);
}
INS B i_ADDI(u64 v) {
  B o = b(v);
  ptr_inc(v(o));
  return o;
}
INS B i_ADDU(u64 v) {
  return b(v);
}
INS B i_FN1C(B f, u32* bc GA1) { P(x) GS_UPD;POS_UPD; // TODO figure out a way to instead pass an offset in bc, so that shorter `mov`s can be used to pass it
  B r = c1(f, x);
  dec(f); return r;
}
INS B i_FN1O(B f, u32* bc GA1) { P(x) GS_UPD;POS_UPD;
  B r = isNothing(x)? x : c1(f, x);
  dec(f); return r;
}
INS B i_FN2C(B w, u32* bc GA1) { P(f)P(x) GS_UPD;POS_UPD;
  B r = c2(f, w, x);
  dec(f); return r;
}
INS B i_FN2O(B w, u32* bc GA1) { P(f)P(x) GS_UPD;POS_UPD;
  B r;
  if (isNothing(x)) { dec(w); r = x; }
  else r = isNothing(w)? c1(f, x) : c2(f, w, x);
  dec(f);
  return r;
}
INS B i_FN1Ci(B x, BB2B fi, u32* bc GA1) { GS_UPD;POS_UPD;
  return fi(b((u64)0), x);
}
INS B i_FN2Ci(B w, BBB2B fi, u32* bc GA1) { P(x) GS_UPD;POS_UPD;
  return fi(b((u64)0), w, x);
}
INS B i_FN1Oi(B x, BB2B fm, u32* bc GA1) { GS_UPD;POS_UPD;
  B r = isNothing(x)? x : fm(b((u64)0), x);
  return r;
}
INS B i_FN2Oi(B w, BB2B fm, BBB2B fd, u32* bc GA1) { P(x) GS_UPD;POS_UPD;
  if (isNothing(x)) { dec(w); return x; }
  else return isNothing(w)? fm(b((u64)0), x) : fd(b((u64)0), w, x);
}
INS B i_ARR_0() { // TODO combine with ADDI
  return inc(bi_emptyHVec);
}
INS B i_ARR_p(B el0, i64 sz GA1) { assert(sz>0);
  HArr_p r = m_harrUv(sz);
  bool allNum = isNum(el0);
  r.a[sz-1] = el0;
  for (i64 i = 1; i < sz; i++) if (!isNum(r.a[sz-i-1] = GSP)) allNum = false;
  if (allNum) {
    GS_UPD;
    return withFill(r.b, m_f64(0));
  } else return r.b;
}
INS B i_DFND_0(u32* bc, Scope** pscs, Block* bl GA1) { GS_UPD;POS_UPD; return m_funBlock(bl, *pscs); }
INS B i_DFND_1(u32* bc, Scope** pscs, Block* bl GA1) { GS_UPD;POS_UPD; return m_md1Block(bl, *pscs); }
INS B i_DFND_2(u32* bc, Scope** pscs, Block* bl GA1) { GS_UPD;POS_UPD; return m_md2Block(bl, *pscs); }
INS B i_OP1D(B f, u32* bc GA1) { P(m)     GS_UPD;POS_UPD; return m1_d  (m,f  ); }
INS B i_OP2D(B f, u32* bc GA1) { P(m)P(g) GS_UPD;POS_UPD; return m2_d  (m,f,g); }
INS B i_OP2H(B m          GA1) {     P(g)                 return m2_h  (m,  g); }
INS B i_TR2D(B g          GA1) {     P(h)                 return m_atop(  g,h); }
INS B i_TR3D(B f          GA1) { P(g)P(h)                 return m_fork(f,g,h); }
INS B i_TR3O(B f          GA1) { P(g)P(h) B r;
  if (isNothing(f)) { r=m_atop(g,h); dec(f); }
  else              { r=m_fork(f,g,h); }
  return r;
}
INS B i_LOCO(u32 d, u32 p, Scope** pscs, u32* bc GA1) {
  B l = pscs[d]->vars[p];
  if(l.u==bi_noVar.u) { POS_UPD; thrM("Reading variable before its defined"); } // TODO probably should GS_UPD (also EXTO)
  return inc(l);
}
INS B i_LOCU(u32 d, u32 p, Scope** pscs) {
  B* vars = pscs[d]->vars;
  B r = vars[p];
  vars[p] = bi_optOut;
  return r;
}
INS B i_EXTO(u32 d, u32 p, Scope** pscs, u32* bc GA1) {
  B l = pscs[d]->ext->vars[p];
  if(l.u==bi_noVar.u) { POS_UPD; thrM("Reading variable before its defined"); }
  return inc(l);
}
INS B i_EXTU(u32 d, u32 p, Scope** pscs) {
  B* vars = pscs[d]->ext->vars;
  B r = vars[p];
  vars[p] = bi_optOut;
  return r;
}
INS B i_SETN(B s, Scope** pscs, u32* bc GA1) {     P(x) GS_UPD; POS_UPD; v_set(pscs, s, x, false); dec(s); return x; }
INS B i_SETU(B s, Scope** pscs, u32* bc GA1) {     P(x) GS_UPD; POS_UPD; v_set(pscs, s, x, true ); dec(s); return x; }
INS B i_SETM(B s, Scope** pscs, u32* bc GA1) { P(f)P(x) GS_UPD; POS_UPD;
  B w = v_get(pscs, s);
  B r = c2(f,w,x); dec(f);
  v_set(pscs, s, r, true); dec(s);
  return r;
}
INS B i_FLDO(B ns, u32 p, Scope** pscs GA1) { GS_UPD;
  if (!isNsp(ns)) thrM("Trying to read a field from non-namespace");
  B r = inc(ns_getU(ns, pscs[0]->body->nsDesc->nameList, p));
  dec(ns);
  return r;
}
INS B i_NSPM(B o, u32 l) {
  B a = mm_alloc(sizeof(FldAlias), t_fldAlias, ftag(OBJ_TAG));
  c(FldAlias,a)->obj = o;
  c(FldAlias,a)->p = l;
  return a;
}
INS B i_RETD(Scope** pscs GA1) { GS_UPD;
  Scope* sc = pscs[0];
  Body* b = sc->body;
  ptr_inc(sc);
  ptr_inc(b->nsDesc);
  return m_ns(sc, b->nsDesc);
}

#undef INS
#undef P
#undef GSP
#undef GS_UPD
#undef POS_UPD
#undef GA1





#include "x86_64.h"

#if USE_PERF
#include <unistd.h>
#include "../utils/file.h"
FILE* perf_map;
u32 perfid = 0;
#endif

static void* nvm_alloc(u64 sz) {
  // void* r = mmap(NULL, sz, PROT_EXEC|PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON|MAP_32BIT, -1, 0);
  // if (r==MAP_FAILED) thrM("JIT: Failed to allocate executable memory");
  // return r;
  TmpFile* src = mmX_allocN(fsizeof(TmpFile,a,u8,sz), t_i8arr);
  arr_shVec(tag(src,ARR_TAG), sz);
  return src->a;
}
void nvm_free(u8* ptr) {
  if (!USE_PERF) mmX_free((Value*)RFLD(ptr, TmpFile, a));
}
static void write_asm(u8* p, u64 sz) { // for debugging; view with objdump -b binary -m i386 -M x86-64,intel -D --adjust-vma=$(cat tmp_off) tmp_bin | tail -n+7
  i32* rp; B r = m_i32arrv(&rp, sz);
  for (u64 i = 0; i < sz; i++) rp[i] = p[i];
  file_wBytes(m_str32(U"tmp_bin"), r);
  char off[20]; snprintf(off, 20, "%p", p);
  B o = m_str8l(off);
  file_wChars(m_str32(U"tmp_off"), o);
  dec(r); dec(o);
}


typedef struct SRef { B v; i32 p; } SRef;
#define SREF(V,P) ((SRef){.v=V,  .p=P})
typedef struct OptRes { u32* bc; u32* offset; } OptRes;
static OptRes opt(u32* bc0) {
  TSALLOC(SRef, stk, 8);
  TSALLOC(u8, actions, 64); // 1 per instruction; 0: nothing; 1: indicates return; 2: remove; 3: FN1_/FN2C; 4: FN2O
  TSALLOC(u64, data, 64); // variable length; whatever things are needed for the specific action
  u32* bc = bc0; usz pos = 0;
  while (true) {
    u32* sbc = bc;
    #define L64 ({ u64 r = bc[0] | ((u64)bc[1])<<32; bc+= 2; r; })
    bool ret = false;
    u8 cact = 0;
    switch (*bc++) { case FN1Ci: case FN1Oi: case FN2Ci: case FN2Oi: thrM("JIT optimization: didn't already expect immediate FN__");
      case ADDU: case ADDI: cact = 0; TSADD(stk,SREF(b(L64), pos)); break;
      case FN1C: case FN1O: { SRef f = stk[TSSIZE(stk)-1];
        if (!isFun(f.v) || v(f.v)->type!=t_funBI) goto defIns;
        actions[f.p] = 2; cact = 3;
        TSADD(data, (u64) c(Fun, f.v)->c1);
        goto defStk;
        break;
      }
      case FN2C: { SRef f = stk[TSSIZE(stk)-2];
        if (!isFun(f.v) || v(f.v)->type!=t_funBI) goto defIns;
        actions[f.p] = 2; cact = 3;
        TSADD(data, (u64) c(Fun, f.v)->c2);
        goto defStk;
        break;
      }
      case FN2O: { SRef f = stk[TSSIZE(stk)-2];
        if (!isFun(f.v) || v(f.v)->type!=t_funBI) goto defIns;
        actions[f.p] = 2; cact = 4;
        TSADD(data, (u64) c(Fun, f.v)->c1);
        TSADD(data, (u64) c(Fun, f.v)->c2);
        goto defStk;
        break;
      }
      case RETN: case RETD:
        ret = true;
        cact = 1;
        goto defIns;
      default: defIns:;
        defStk:
        TSSIZE(stk)-= stackConsumed(sbc);
        i32 added = stackAdded(sbc);
        for (i32 i = 0; i < added; i++) TSADD(stk, SREF(bi_optOut, -1))
    }
    TSADD(actions, cact);
    #undef L64
    if (ret) break;
    bc = nextBC(sbc);
    pos++;
  }
  TSFREE(stk);
  
  TSALLOC(u32, rbc, TSSIZE(actions));
  TSALLOC(u32, roff, TSSIZE(actions));
  bc = bc0;
  u64 tpos = 0, dpos = 0;
  while (true) {
    u32* sbc = bc;
    u32* ebc = nextBC(sbc);
    #define L64 ({ u64 r = bc[0] | ((u64)bc[1])<<32; bc+= 2; r; })
    u32 ctype = actions[tpos++];
    bool ret = false;
    u32 v = *bc++;
    u64 psz = TSSIZE(rbc);
    #define A64(X) { u64 a64=(X); TSADD(rbc, (u32)a64); TSADD(rbc, a64>>32); }
    switch (ctype) { default: UD;
      case 3: assert(v==FN1C|v==FN1O|v==FN2C);
        TSADD(rbc, v==FN1C? FN1Ci : v==FN1O? FN1Oi : FN2Ci);
        A64(data[dpos++]);
        break;
      case 4: assert(v==FN2O);
        TSADD(rbc, FN2Oi);
        A64(data[dpos++]);
        A64(data[dpos++]);
        break;
      
      case 2: break; // remove
      
      case 1: ret = true; goto def2; // return
      
      case 0: def2:; // do nothing
        TSADDA(rbc, sbc, ebc-sbc);
    }
    u64 added = TSSIZE(rbc)-psz;
    for (i32 i = 0; i < added; i++) TSADD(roff, sbc-bc0);
    #undef A64
    if (ret) break;
    bc = ebc;
  }
  bc = bc0; pos = 0;
  
  TSFREE(data);
  TSFREE(actions);
  return (OptRes){.bc = rbc, .offset = roff};
}
#undef SREF
void freeOpt(OptRes o) {
  TSFREEP(o.bc);
  TSFREEP(o.offset);
}

static u32 readBytes4(u8* d) {
  return d[0] | d[1]<<8 | d[2]<<16 | d[3]<<24;
}

typedef B JITFn(B* cStack, Scope** pscs);
static inline i32 maxi32(i32 a, i32 b) { return a>b?a:b; }
u8* m_nvm(Body* body) {
  ALLOC_ASM(64);
  TSALLOC(u32, rel, 64);
  #define r_TMP 3
  #define r_PSCS 14
  #define r_CS 15
  ASM(PUSH, r_TMP, -);
  ASM(PUSH, r_PSCS, -);
  ASM(PUSH, r_CS, -);
  ASM(MOV, r_CS  , REG_ARG0);
  ASM(MOV, r_PSCS, REG_ARG1);
  if ((u64)i_SETN != (u32)(u64)i_SETN) thrM("JIT: Refusing to run with CBQN code outside of the 32-bit address range");
  // #define CCALL(F) { IMM(r_TMP, F); ASM(CALL, r_TMP, -); }
  #define CCALL(F) { TSADD(rel, ASM_SIZE); ASM(CALLI, (u32)(u64)F, -); }
  u32* origBC = body->bc;
  OptRes optRes = opt(origBC);
  Block** blocks = body->blocks->a;
  i32 depth = 0;
  u32* bc = optRes.bc;
  while (true) {
    u32* s = bc;
    u32* n = nextBC(bc);
    u32* off = origBC + optRes.offset[s-optRes.bc];
    bool ret = false;
    #define L64 ({ u64 r = bc[0] | ((u64)bc[1])<<32; bc+= 2; r; })
    // #define LEA0(O,I,OFF) { ASM(MOV,O,I); ADDI(O,OFF); }
    #define LEA0(O,I,OFF,Q) ({ i32 o=(OFF); u8 r; if(!o) { r=I; if(Q)ASM(MOV,O,I); } else { r=O; if(o==(i8)o) { ASM3(LEAo1,O,I,o); } else { ASM(MOV,O,I); ADDI(O,o); } } r; })
    #define SPOS(R,N,Q) LEA0(R, r_CS, maxi32(0, depth+(N)-1)*sizeof(B),Q)
    #if CSTACK
      // #define INV(N,D,F) ASM(MOV,REG_ARG##N,r_CS); ADDI(r_CS,(D)*sizeof(B)); CCALL(F)
      #define INV(N,D,F) SPOS(REG_ARG##N, D, 1); CCALL(F)
    #else
      #define INV(N,D,F) CCALL(F) // N - stack argument number; D - expected stack delta; F - called function; TODO instrs which don't need stack (POPS and things with stack delta 1)
    #endif
    #define TOPp ASM(MOV,REG_ARG0,REG_RES)
    #define TOPs if (depth) { u8 t = SPOS(r_TMP, 0, 0); ASM(MOV_MR0, t, REG_RES); }
    switch (*bc++) {
      case POPS: TOPp;
        CCALL(i_POPS);
        if (depth>1) { u8 t = SPOS(r_TMP, -1, 0); ASM(MOV_RM0, REG_RES, t); }
      break;
      case ADDI: TOPs; IMM(REG_ARG0, L64); CCALL(i_ADDI); break; // (u64 v, S)
      case ADDU: TOPs; // (u64 v, S)
        #if CSTACK
          IMM(REG_RES, L64);
        #else
          IMM(REG_ARG0, L64); CCALL(i_ADDU);
        #endif
      break;
      case FN1C: TOPp; IMM(REG_ARG1,off); INV(2,0,i_FN1C); break; // (B, u32* bc, S)
      case FN2C: TOPp; IMM(REG_ARG1,off); INV(2,0,i_FN2C); break; // (B, u32* bc, S)
      case FN1O: TOPp; IMM(REG_ARG1,off); INV(2,0,i_FN1O); break; // (B, u32* bc, S)
      case FN2O: TOPp; IMM(REG_ARG1,off); INV(2,0,i_FN2O); break; // (B, u32* bc, S)
      case FN1Ci:TOPp; IMM(REG_ARG1,L64);                     IMM(REG_ARG2,off); INV(3,0,i_FN1Ci); break; // (B, BB2B  fm, u32* bc, S)
      case FN2Ci:TOPp; IMM(REG_ARG1,L64);                     IMM(REG_ARG2,off); INV(3,0,i_FN2Ci); break; // (B, BBB2B fd, u32* bc, S)
      case FN1Oi:TOPp; IMM(REG_ARG1,L64);                     IMM(REG_ARG2,off); INV(3,0,i_FN1Oi); break; // (B, BB2B  fm,           u32* bc, S)
      case FN2Oi:TOPp; IMM(REG_ARG1,L64); IMM(REG_ARG2, L64); IMM(REG_ARG3,off); INV(4,0,i_FN2Oi); break; // (B, BB2B  fm, BBB2B fd, u32* bc, S)
      case ARRM: case ARRO:;
        u32 sz = *bc++;
        if (sz) { TOPp; IMM(REG_ARG1, sz); INV(2,0,i_ARR_p); } // (B, i64 sz, S)
        else    { TOPs;                      CCALL(i_ARR_0); } // (S)
        break;
      case DFND: TOPs; // (u32* bc, Scope** pscs, Block* bl, S)
        Block* bl = blocks[*bc++];
        u64 fn = (u64)(bl->ty==0? i_DFND_0 : bl->ty==1? i_DFND_1 : bl->ty==2? i_DFND_2 : NULL);
        if (fn==0) thrM("JIT: Bad DFND argument");
        IMM(REG_ARG0,off); ASM(MOV,REG_ARG1,r_PSCS); IMM(REG_ARG2,bl); INV(3,1,fn);
        break;
      case OP1D: TOPp; IMM(REG_ARG1,off); INV(2,0,i_OP1D); break; // (B, u32* bc, S)
      case OP2D: TOPp; IMM(REG_ARG1,off); INV(2,0,i_OP2D); break; // (B, u32* bc, S)
      case OP2H: TOPp; INV(1,0,i_OP2H); break; // (B, S)
      case TR2D: TOPp; INV(1,0,i_TR2D); break; // (B, S)
      case TR3D: TOPp; INV(1,0,i_TR3D); break; // (B, S)
      case TR3O: TOPp; INV(1,0,i_TR3O); break; // (B, S)
      case LOCM: TOPs; { u64 d=*bc++; u64 p=*bc++; IMM(REG_RES, tag((u64)d<<32 | (u32)p, VAR_TAG).u); } break;
      case EXTM: TOPs; { u64 d=*bc++; u64 p=*bc++; IMM(REG_RES, tag((u64)d<<32 | (u32)p, EXT_TAG).u); } break;
      case LOCO: TOPs; IMM(REG_ARG0,*bc++); IMM(REG_ARG1,*bc++); ASM(MOV,REG_ARG2,r_PSCS); IMM(REG_ARG3,off); INV(4,1,i_LOCO); break; // (u32 d, u32 p, Scope** pscs, u32* bc, S)
      case EXTO: TOPs; IMM(REG_ARG0,*bc++); IMM(REG_ARG1,*bc++); ASM(MOV,REG_ARG2,r_PSCS); IMM(REG_ARG3,off); INV(4,1,i_EXTO); break; // (u32 d, u32 p, Scope** pscs, u32* bc, S)
      case LOCU: TOPs; IMM(REG_ARG0,*bc++); IMM(REG_ARG1,*bc++); ASM(MOV,REG_ARG2,r_PSCS);                    CCALL(i_LOCU); break; // (u32 d, u32 p, Scope** pscs, S)
      case EXTU: TOPs; IMM(REG_ARG0,*bc++); IMM(REG_ARG1,*bc++); ASM(MOV,REG_ARG2,r_PSCS);                    CCALL(i_EXTU); break; // (u32 d, u32 p, Scope** pscs, S)
      case SETN: TOPp; ASM(MOV,REG_ARG1,r_PSCS); IMM(REG_ARG2,off); INV(3,0,i_SETN); break; // (B, Scope** pscs, u32* bc, S)
      case SETU: TOPp; ASM(MOV,REG_ARG1,r_PSCS); IMM(REG_ARG2,off); INV(3,0,i_SETU); break; // (B, Scope** pscs, u32* bc, S)
      case SETM: TOPp; ASM(MOV,REG_ARG1,r_PSCS); IMM(REG_ARG2,off); INV(3,0,i_SETM); break; // (B, Scope** pscs, u32* bc, S)
      case FLDO: TOPp; IMM(REG_ARG1,*bc++); ASM(MOV,REG_ARG2,r_PSCS); INV(3,0,i_FLDO); break; // (B, u32 p, Scope** pscs, S)
      case NSPM: TOPp; IMM(REG_ARG1,*bc++); CCALL(i_NSPM); break; // (B, u32 l, S)
      case RETD: ASM(MOV,REG_ARG0,r_PSCS); INV(1,1,i_RETD); ret=true; break; // (Scope** pscs, S); stack diff 0 is wrong, but updating it is useless
      case RETN: IMM(r_TMP, &gStack); ASM(MOV_MR0, r_TMP, r_CS); ret=true; break;
      default: thrF("JIT: Unsupported bytecode %i", *s);
    }
    #undef TOPs
    #undef TOPp
    #undef INV
    #undef SPOS
    #undef L64
    if (n!=bc) thrM("JIT: Wrong parsing of bytecode");
    depth+= stackDiff(s);
    if (ret) break;
  }
  freeOpt(optRes);
  ASM(POP, r_CS, -);
  ASM(POP, r_PSCS, -);
  ASM(POP, r_TMP, -);
  ASM_RAW(bin, RET);
  #undef CCALL
  GET_ASM();
  u64 sz = ASM_SIZE;
  u8* binEx = nvm_alloc(sz);
  #if USE_PERF
    if (!perf_map) {
      B s = m_str32(U"/tmp/perf-"); AFMT("%l.map", getpid());
      perf_map = file_open(s, "open", "wa");
      print(s); printf(": map\n");
      dec(s);
    }
    u32 bcPos = body->map[0];
    // printf("JIT %d:\n", perfid);
    // vm_printPos(body->comp, bcPos, -1);
    fprintf(perf_map, "%lx %lx JIT %d: BC@%u\n", (u64)binEx, sz, perfid++, bcPos);
  #endif
  memcpy(binEx, bin, sz);
  u64 relAm = TSSIZE(rel);
  // printf("allocated at %p; i_ADDU: %p\n", binEx, i_ADDU);
  for (u64 i = 0; i < relAm; i++) {
    u8* ins = binEx+rel[i];
    u32 o = readBytes4(ins+1);
    u32 n = o-(u32)(u64)ins-5;
    memcpy(ins+1, (u8[]){BYTES4(n)}, 4);
  }
  // write_asm(binEx, sz);
  FREE_ASM();
  TSFREE(rel);
  return binEx;
}
B evalJIT(Body* b, Scope* sc, u8* ptr) { // doesn't consume
  u32* bc = b->bc;
  pushEnv(sc, bc);
  gsReserve(b->maxStack);
  Scope* pscs[b->maxPSC+1];
  pscs[0] = sc;
  for (i32 i = 0; i < b->maxPSC; i++) pscs[i+1] = pscs[i]->psc;
  // write_asm(ptr, RFLD(ptr, TmpFile, a)->ia);
  
  B* sp = gStack;
  B r = ((JITFn*)ptr)(gStack, pscs);
  if (sp!=gStack) thrM("uh oh");
  
  popEnv();
  return r;
}
