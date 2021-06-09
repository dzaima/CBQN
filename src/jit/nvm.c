#include "../core.h"
#include "../core/gstack.h"
#include "../ns.h"
#include "../utils/file.h"

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
  #define GA0  B* cStack
  #define GA1 ,B* cStack
  #define GSA(X) { B tr=X; *(cStack++) = tr; }
  #define GSP (*--cStack)
  #define GS_UPD { gStack=cStack; }
#else
  #define GA0
  #define GA1
  #define GSA(X) { B tr=X; *(gStack++) = tr; }
  #define GSP (*--gStack)
  #define GS_UPD
#endif
#define P(N) B N=GSP;
#if VM_POS
  #define POS_UPD (envCurr-1)->bcL = bc-1;
#else
  #define POS_UPD
#endif
#define INS NOINLINE
INS void i_POPS(GA0) {
  dec(GSP);
}
// INS void i_PUSH(GA0) {
//   thrM("NYI PUSH in nvm");
// }
INS void i_ADDI(u64 v GA1) {
  B o = b(v);
  ptr_inc(v(o));
  GSA(o);
}
INS void i_ADDU(u64 v GA1) {
  GSA(b(v));
}
INS void i_FN1C(u32* bc GA1) { P(f)P(x) // TODO figure out a way to instead pass an offset in bc, so that shorter `mov`s can be used to pass it
  GS_UPD;POS_UPD;
  GSA(c1(f, x)); dec(f);
}
INS void i_FN1O(u32* bc GA1) { P(f)P(x)
  GS_UPD;POS_UPD;
  GSA(isNothing(x)? x : c1(f, x)); dec(f);
}
INS void i_FN2C(u32* bc GA1) { P(w)P(f)P(x)
  GS_UPD;POS_UPD;
  GSA(c2(f, w, x)); dec(f);
}
INS void i_FN2O(u32* bc GA1) { P(w)P(f)P(x)
  GS_UPD;POS_UPD;
  if (isNothing(x)) { dec(w); GSA(x); }
  else GSA(isNothing(w)? c1(f, x) : c2(f, w, x));
  dec(f);
}
INS void i_ARR_0(GA0) {
  GSA(inc(bi_emptyHVec));
}
INS void i_ARR_p(i64 sz GA1) {
  HArr_p r = m_harrUv(sz);
  bool allNum = true;
  assert(sz>0);
  for (i64 i = 0; i < sz; i++) if (!isNum(r.a[sz-i-1] = GSP)) allNum = false;
  if (allNum) {
    GS_UPD;
    GSA(withFill(r.b, m_f64(0)));
  } else GSA(r.b);
}
INS void i_DFND_0(u32* bc, Scope** pscs, Block* bl GA1) { GS_UPD;POS_UPD; GSA(m_funBlock(bl, *pscs)); }
INS void i_DFND_1(u32* bc, Scope** pscs, Block* bl GA1) { GS_UPD;POS_UPD; GSA(m_md1Block(bl, *pscs)); }
INS void i_DFND_2(u32* bc, Scope** pscs, Block* bl GA1) { GS_UPD;POS_UPD; GSA(m_md2Block(bl, *pscs)); }
INS void i_OP1D(u32* bc GA1) { P(f)P(m)     GS_UPD;POS_UPD; GSA(m1_d  (m,f  )); }
INS void i_OP2D(u32* bc GA1) { P(f)P(m)P(g) GS_UPD;POS_UPD; GSA(m2_d  (m,f,g)); }
INS void i_OP2H(        GA0) {     P(m)P(g)                 GSA(m2_h  (m,  g)); }
INS void i_TR2D(        GA0) {     P(g)P(h)                 GSA(m_atop(  g,h)); }
INS void i_TR3D(        GA0) { P(f)P(g)P(h)                 GSA(m_fork(f,g,h)); }
INS void i_TR3O(        GA0) { P(f)P(g)P(h)
  if (isNothing(f)) { GSA(m_atop(g,h)); dec(f); }
  else GSA(m_fork(f,g,h));
}
INS void i_LOCM(u32 d, u32 p GA1) {
  GSA(tag((u64)d<<32 | (u32)p, VAR_TAG));
}
INS void i_LOCO(u32 d, u32 p, Scope** pscs, u32* bc GA1) {
  B l = pscs[d]->vars[p];
  if(l.u==bi_noVar.u) { POS_UPD; thrM("Reading variable before its defined"); }
  GSA(inc(l));
}
INS void i_LOCU(u32 d, u32 p, Scope** pscs GA1) {
  B* vars = pscs[d]->vars;
  GSA(vars[p]);
  vars[p] = bi_optOut;
}
INS void i_EXTM(u32 d, u32 p GA1) {
  GSA(tag((u64)d<<32 | (u32)p, EXT_TAG));
}
INS void i_EXTO(u32 d, u32 p, Scope** pscs, u32* bc GA1) {
  B l = pscs[d]->ext->vars[p];
  if(l.u==bi_noVar.u) { POS_UPD; thrM("Reading variable before its defined"); }
  GSA(inc(l));
}
INS void i_EXTU(u32 d, u32 p, Scope** pscs GA1) {
  B* vars = pscs[d]->ext->vars;
  GSA(vars[p]);
  vars[p] = bi_optOut;
}
INS void i_SETN(Scope** pscs, u32* bc GA1) { P(s)    P(x) GS_UPD; POS_UPD; v_set(pscs, s, x, false); dec(s); GSA(x); }
INS void i_SETU(Scope** pscs, u32* bc GA1) { P(s)    P(x) GS_UPD; POS_UPD; v_set(pscs, s, x, true ); dec(s); GSA(x); }
INS void i_SETM(Scope** pscs, u32* bc GA1) { P(s)P(f)P(x) GS_UPD; POS_UPD;
  B w = v_get(pscs, s);
  B r = c2(f,w,x); dec(f);
  v_set(pscs, s, r, true); dec(s);
  GSA(r);
}
INS void i_FLDO(u32 p, Scope** pscs GA1) { P(ns) GS_UPD;
  if (!isNsp(ns)) thrM("Trying to read a field from non-namespace");
  GSA(inc(ns_getU(ns, pscs[0]->body->nsDesc->nameList, p)));
  dec(ns);
}
INS void i_NSPM(u32 l GA1) { P(o)
  B a = mm_alloc(sizeof(FldAlias), t_fldAlias, ftag(OBJ_TAG));
  c(FldAlias,a)->obj = o;
  c(FldAlias,a)->p = l;
  GSA(a);
}
INS B i_RETD(Scope** pscs GA1) {
  Scope* sc = pscs[0];
  Body* b = sc->body;
  ptr_inc(sc);
  ptr_inc(b->nsDesc);
  GS_UPD;
  return m_ns(sc, b->nsDesc);
}
INS B i_RETN(GA0) { P(v)
  GS_UPD;
  return v;
}

#undef INS
#undef P
#undef GSP
#undef GSA
#undef GS_UPD
#undef POS_UPD
#undef GA0
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


static u32 readBytes4(u8* d) {
  return d[0] | d[1]<<8 | d[2]<<16 | d[3]<<24;
}

typedef B JITFn(B* cStack, Scope** pscs);
u8* m_nvm(Body* body) {
  TSALLOC(u8, bin, 64);
  TSALLOC(u32, rel, 64);
  #define r_TMP 12
  #define r_PSCS 13
  #define r_CS 14
  ASM(PUSH, 12, -);
  ASM(PUSH, 13, -);
  ASM(PUSH, 14, -);
  ASM(MOV, r_CS  , REG_ARG0);
  ASM(MOV, r_PSCS, REG_ARG1);
  // #define CCALL(F) { IMM(r_TMP, F); ASM(CALL, r_TMP, -); }
  #define CCALL(F) { if((u64)F < 1ULL<<31) { TSADD(rel, TSSIZE(bin)); ASM(CALLI, (u32)F, -); } else { IMM(r_TMP, F); ASM(CALL, r_TMP, -); } }
  u32* bc = body->bc;
  Block** blocks = body->blocks->a;
  while (true) {
    u32* s = bc;
    u32* n = nextBC(bc);
    bool ret = false;
    #define L64 ({ u64 r = bc[0] | ((u64)bc[1])<<32; bc+= 2; r; })
    #if CSTACK
      #define INV(N,D,F) ASM(MOV,REG_ARG##N, r_CS); ADDI(r_CS,(D)*sizeof(B)); CCALL(F)
    #else
      #define INV(N,D,F) CCALL(F) // N - stack argument number; D - expected stack delta; F - called function
    #endif
    switch (*bc++) {
      case POPS: INV(0,-1,i_POPS); break; // (S)
      case ADDI: IMM(REG_ARG0, L64); INV(1,1,i_ADDI); break; // (u64 v, S)
      case ADDU:
        #if CSTACK
          IMM(r_TMP, L64); ASM(MOV_MR0, r_CS, r_TMP); ADDI(r_CS,sizeof(B));
        #else
          IMM(REG_ARG0, L64); INV(1,1,i_ADDU);
        #endif
      break; // (u64 v, S)
      case FN1C: IMM(REG_ARG0, s); INV(1,-1,i_FN1C); break; // (u32* bc, S)
      case FN2C: IMM(REG_ARG0, s); INV(1,-2,i_FN2C); break; // (u32* bc, S)
      case FN1O: IMM(REG_ARG0, s); INV(1,-1,i_FN1O); break; // (u32* bc, S)
      case FN2O: IMM(REG_ARG0, s); INV(1,-2,i_FN2O); break; // (u32* bc, S)
      case ARRM: case ARRO: // (i64 sz, S)
        u32 sz = *bc++;
        if (sz) { IMM(REG_ARG0, sz); INV(1,1-(i32)sz,i_ARR_p); }
        else    {                    INV(0,        1,i_ARR_0); }
        break;
      case DFND: // (u32* bc, Scope** pscs, Block* bl, S)
        Block* bl = blocks[*bc++];
        u64 fn = (u64)(bl->ty==0? i_DFND_0 : bl->ty==1? i_DFND_1 : bl->ty==2? i_DFND_2 : NULL);
        if (fn==0) thrM("JIT: Bad DFND argument");
        IMM(REG_ARG0,s); ASM(MOV,REG_ARG1,r_PSCS); IMM(REG_ARG2,bl); INV(3,1,fn);
        break;
      case OP1D: IMM(REG_ARG0,s); INV(1,-1,i_OP1D) break; // (u32* bc, S)
      case OP2D: IMM(REG_ARG0,s); INV(1,-2,i_OP2D) break; // (u32* bc, S)
      case OP2H: INV(0,-1,i_OP2H) break; // (S)
      case TR2D: INV(0,-1,i_TR2D) break; // (S)
      case TR3D: INV(0,-2,i_TR3D) break; // (S)
      case TR3O: INV(0,-2,i_TR3O) break; // (S)
      case LOCM: IMM(REG_ARG0,*bc++); IMM(REG_ARG1,*bc++);                                            INV(2,1,i_LOCM); break; // (u32 d, u32 p, S)
      case LOCO: IMM(REG_ARG0,*bc++); IMM(REG_ARG1,*bc++); ASM(MOV,REG_ARG2,r_PSCS); IMM(REG_ARG3,s); INV(4,1,i_LOCO); break; // (u32 d, u32 p, Scope** pscs, u32* bc, S)
      case LOCU: IMM(REG_ARG0,*bc++); IMM(REG_ARG1,*bc++); ASM(MOV,REG_ARG2,r_PSCS);                  INV(3,1,i_LOCU); break; // (u32 d, u32 p, Scope** pscs, S)
      case EXTM: IMM(REG_ARG0,*bc++); IMM(REG_ARG1,*bc++);                                            INV(2,1,i_EXTM); break; // (u32 d, u32 p, S)
      case EXTO: IMM(REG_ARG0,*bc++); IMM(REG_ARG1,*bc++); ASM(MOV,REG_ARG2,r_PSCS); IMM(REG_ARG3,s); INV(4,1,i_EXTO); break; // (u32 d, u32 p, Scope** pscs, u32* bc, S)
      case EXTU: IMM(REG_ARG0,*bc++); IMM(REG_ARG1,*bc++); ASM(MOV,REG_ARG2,r_PSCS);                  INV(3,1,i_EXTU); break; // (u32 d, u32 p, Scope** pscs, S)
      case SETN: ASM(MOV,REG_ARG0,r_PSCS); IMM(REG_ARG1,s); INV(2,-1,i_SETN); break; // (Scope** pscs, u32* bc, S)
      case SETU: ASM(MOV,REG_ARG0,r_PSCS); IMM(REG_ARG1,s); INV(2,-1,i_SETU); break; // (Scope** pscs, u32* bc, S)
      case SETM: ASM(MOV,REG_ARG0,r_PSCS); IMM(REG_ARG1,s); INV(2,-2,i_SETM); break; // (Scope** pscs, u32* bc, S)
      case FLDO: IMM(REG_ARG0,*bc++); ASM(MOV,REG_ARG1,r_PSCS); INV(2,0,i_FLDO); break; // (u32 p, Scope** pscs, S)
      case NSPM: IMM(REG_ARG0,*bc++); INV(1,0,i_NSPM); break; // (u32 l, S)
      case RETD: ASM(MOV,REG_ARG0,r_PSCS); INV(1,0,i_RETD); ret=true; break; // (Scope** pscs, S); stack diff 0 is wrong, but updating it is useless
      case RETN:                           INV(0,0,i_RETN); ret=true; break; // (S)
      default: thrF("JIT: Unsupported bytecode %i", *s);
    }
    #undef L64
    if (n!=bc) thrM("JIT: Wrong parsing of bytecode");
    if (ret) break;
  }
  ASM(POP, 14, -);
  ASM(POP, 13, -);
  ASM(POP, 12, -);
  ASM_RAW(bin, RET);
  
  u64 sz = TSSIZE(bin);
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
    u32 n = o-(u32)ins-5;
    memcpy(ins+1, (u8[]){BYTES4(n)}, 4);
  }
  // write_asm(binEx, sz);
  TSFREE(bin);
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
  
  B r = ((JITFn*)ptr)(gStack, pscs);
  
  popEnv();
  return r;
}
