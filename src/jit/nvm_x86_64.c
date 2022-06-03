#include <sys/mman.h>
#include "../core.h"
#include "../core/gstack.h"
#include "../ns.h"
#include "../utils/file.h"
#include "../utils/talloc.h"
#include "../utils/mut.h"
#include "../utils/wyhash.h"
#include "../vm.h"

#ifndef USE_PERF
  #define USE_PERF 0 // enable writing symbols to /tmp/perf-<pid>.map
#endif
#ifndef WRITE_ASM
  #define WRITE_ASM 0 // writes on every compilation, overriding the previous; view with:
#endif                // objdump -b binary -m i386 -M x86-64,intel --insn-width=10 -D --adjust-vma=$(cat asm_off) asm_bin | tail -n+8 | sed "$(cat asm_sed);s/\\t/ /g;s/.*: //"


// separate memory management system for executable code; isn't garbage-collected
EmptyValue* mmX_buckets[64];
u64 mmX_ctrs[64];
#define  BSZ(X) (1ull<<(X))
#define BSZI(X) ((u8)(64-CLZ((X)-1ull)))
#define  MMI(X) X
#define   BN(X) mmX_##X
#include "../opt/mm_buddyTemplate.h"
#define  MMI(X) X
#define  ALSZ  17

static u64 nvm_mmap_seed = 0;
#ifdef __clang__
#if __clang_major__ < 11 // clang 10 gets stuck in an infinite loop while optimizing this
__attribute__((optnone))
#endif
#endif
static void* mmap_nvm(u64 sz) {
  u64 near = (u64)&bqn_exec;
  u64 MAX_DIST = 1ULL<<30;
  if (near < MAX_DIST) return mmap(NULL, sz, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_NORESERVE|MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT, -1, 0);
  u64 ps = getPageSize();
  
  i32 attempt = 0;
  // printf("binary at %p\n", &bqn_exec);
  while(true) {
    if (attempt++ > 200) err("Failed to allocate memory for JIT 200 times; stopping trying");
    u64 randOff = wyrand(&nvm_mmap_seed) & (MAX_DIST>>1)-1;
    u64 loc = near+randOff & ~(ps-1);
    // printf("request %d: %p\n", attempt, (void*)loc);
    int noreplace = 0;
    #ifdef MAP_FIXED_NOREPLACE
      noreplace|= MAP_FIXED_NOREPLACE;
    #endif
    void* c = mmap((void*)loc, sz, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_NORESERVE|MAP_PRIVATE|MAP_ANONYMOUS|noreplace, -1, 0);
    if (c==NULL) continue;
    
    i64 dist = (i64)near - (i64)c;
    if ((dist<0?-dist:dist) < MAX_DIST) return c;
    munmap(c, sz);
  }
}

#define MMAP(SZ) mmap_nvm(sz);
#include "../opt/mm_buddyTemplate.c"
static void* mmX_allocN(usz sz, u8 type) { assert(sz>=16); return mmX_allocL(64-CLZ(sz-1ull), type); }
#undef BN
#undef BSZ


// all the instructions to be called by the generated code
#define GSP (*--cStack)
#define GS_UPD { gStack=cStack; }
#define P(N) B N=GSP;
#if VM_POS
  #define POS_UPD envCurr->pos = (u64)bc;
#else
  #define POS_UPD
#endif
#define INS NOINLINE __attribute__ ((aligned(64), hot)) // idk man
INS void i_POPS(B x) {
  dec(x);
}
INS void i_INC(Value* v) {
  ptr_inc(v);
}
INS B i_FN1C(B f, B x, u32* bc) { POS_UPD; // TODO figure out a way to instead pass an offset in bc, so that shorter `mov`s can be used to pass it
  B r = c1(f, x);
  dec(f); return r;
}
INS B i_FN1O(B f, B x, u32* bc) { POS_UPD;
  B r = q_N(x)? x : c1(f, x);
  dec(f); return r;
}
INS B i_FN2C(B w, B f, B x, u32* bc) { POS_UPD;
  B r = c2(f, w, x);
  dec(f); return r;
}
INS B i_FN2O(B w, B f, B x, u32* bc) { POS_UPD;
  B r;
  if (q_N(x)) { dec(w); r = x; }
  else r = q_N(w)? c1(f, x) : c2(f, w, x);
  dec(f);
  return r;
}
INS B i_FN1Oi(B x, BB2B fm, u32* bc) { POS_UPD;
  B r = q_N(x)? x : fm(b((u64)0), x);
  return r;
}
INS B i_FN2Oi(B w, B x, BB2B fm, BBB2B fd, u32* bc) { POS_UPD;
  if (q_N(x)) { dec(w); return x; }
  else return q_N(w)? fm(b((u64)0), x) : fd(b((u64)0), w, x);
}
INS B i_ARR_0() { // TODO combine with ADDI
  return emptyHVec();
}
INS B i_ARR_p(B el0, i64 sz, B* cStack) { assert(sz>0);
  HArr_p r = m_harrUv(sz); // can't use harrs as gStack isn't updated
  bool allNum = isNum(el0);
  r.a[sz-1] = el0;
  for (i64 i = 1; i < sz; i++) if (!isNum(r.a[sz-i-1] = GSP)) allNum = false;
  GS_UPD;
  if (allNum) return num_squeeze(r.b);
  return r.b;
}
INS B i_DFND_0(u32* bc, Scope* sc, Block* bl) { POS_UPD; return evalFunBlock(bl, sc); }
INS B i_DFND_1(u32* bc, Scope* sc, Block* bl) { POS_UPD; return m_md1Block(bl, sc); } // TODO these only fail on oom, so no need to update pos
INS B i_DFND_2(u32* bc, Scope* sc, Block* bl) { POS_UPD; return m_md2Block(bl, sc); }
INS B i_MD1C(B f,B m,      u32* bc) { POS_UPD; return m1_d  (m,f  ); }
INS B i_MD2C(B f,B m, B g, u32* bc) { POS_UPD; return m2_d  (m,f,g); }
INS B i_MD2R(B m,     B g         ) {          return m2_h  (m,  g); }
INS B i_TR2D(B g,     B h         ) {          return m_atop(  g,h); }
INS B i_TR3D(B f,B g, B h         ) {          return m_fork(f,g,h); }
INS B i_TR3O(B f,B g, B h         ) {          return q_N(f)? m_atop(g,h) : m_fork(f,g,h); }
INS B i_NOVAR(u32* bc, B* cStack) {
  POS_UPD; GS_UPD; thrM("Reading variable before its defined");
}
INS B i_EXTO(u32 p, Scope* sc, u32* bc, B* cStack) {
  B l = sc->ext->vars[p];
  if(l.u==bi_noVar.u) { POS_UPD; GS_UPD; thrM("Reading variable before its defined"); }
  return inc(l);
}
INS B i_EXTU(u32 p, Scope* sc) {
  B* vars = sc->ext->vars;
  B r = vars[p];
  vars[p] = bi_optOut;
  return r;
}
INS B i_SETN(B s,      B x, Scope** pscs, u32* bc) { POS_UPD; v_set(pscs, s, x, false, true); dec(s); return x; }
INS B i_SETU(B s,      B x, Scope** pscs, u32* bc) { POS_UPD; v_set(pscs, s, x, true,  true); dec(s); return x; }
INS B i_SETM(B s, B f, B x, Scope** pscs, u32* bc) { POS_UPD;
  B w = v_get(pscs, s, true);
  B r = c2(f,w,x); dec(f);
  v_set(pscs, s, r, true, false); dec(s);
  return r;
}
INS B i_SETC(B s, B f, Scope** pscs, u32* bc) { POS_UPD;
  B x = v_get(pscs, s, true);
  B r = c1(f,x); dec(f);
  v_set(pscs, s, r, true, false); dec(s);
  return r;
}
FORCE_INLINE B gotoNextBodyJIT(Scope* sc, Body* body) {
  if (body==NULL) thrF("No header matched argument%S", q_N(sc->vars[2])?"":"s");
  Block* bl = body->bl;
  // because of nvm returning semantics, we cannot quick-skip to the next body. TODO maybe we can by moving tail of evalJIT into i_RETN etc and some magic™ here?
  u64 ga = blockGivenVars(bl);
  for (u64 i = 0; i < ga; i++) inc(sc->vars[i]);
  Scope* nsc = m_scope(body, sc->psc, body->varAm, ga, sc->vars);
  return execBodyInlineI(body, nsc, bl);
}
INS B i_SETH1(B s, B x, Scope** pscs, u32* bc, Body* v1) { POS_UPD;
  bool ok = v_seth(pscs, s, x); dec(x); dec(s);
  if (ok) return bi_okHdr;
  return gotoNextBodyJIT(pscs[0], v1);
}
INS B i_SETH2(B s, B x, Scope** pscs, u32* bc, Body* v1, Body* v2) { POS_UPD;
  bool ok = v_seth(pscs, s, x); dec(x); dec(s);
  if (ok) return bi_okHdr;
  return gotoNextBodyJIT(pscs[0], q_N(pscs[0]->vars[2])? v1 : v2);
}
INS B i_PRED1(B x, Scope* sc, u32* bc, Body* v) { POS_UPD;
  if (o2b(x)) return bi_okHdr;
  return gotoNextBodyJIT(sc, v);
}
INS B i_PRED2(B x, Scope* sc, u32* bc, Body* v1, Body* v2) { POS_UPD;
  if (o2b(x)) return bi_okHdr;
  return gotoNextBodyJIT(sc, q_N(sc->vars[2])? v1 : v2);
}
INS B i_SETNi(     B x, Scope* sc, u32 p         ) {          v_setI(sc, p, inc(x), false, false); return x; }
INS B i_SETUi(     B x, Scope* sc, u32 p, u32* bc) { POS_UPD; v_setI(sc, p, inc(x), true,  false); return x; }
INS B i_SETMi(B f, B x, Scope* sc, u32 p, u32* bc) { POS_UPD; B r = c2(f,v_getI(sc, p, false),x); dec(f); v_setI(sc, p, inc(r), true, false); return r; }
INS B i_SETCi(B f,      Scope* sc, u32 p, u32* bc) { POS_UPD; B r = c1(f,v_getI(sc, p, false)  ); dec(f); v_setI(sc, p, inc(r), true, false); return r; }
INS void i_SETNv(B x, Scope* sc, u32 p         ) {          v_setI(sc, p, x, false, false); }
INS void i_SETUv(B x, Scope* sc, u32 p, u32* bc) { POS_UPD; v_setI(sc, p, x, true,  false); }
INS void i_SETMv(B f, B x, Scope* sc, u32 p, u32* bc) { POS_UPD; B r = c2(f,v_getI(sc, p, false),x); dec(f); v_setI(sc, p, r, true, false); }
INS void i_SETCv(B f,      Scope* sc, u32 p, u32* bc) { POS_UPD; B r = c1(f,v_getI(sc, p, false)  ); dec(f); v_setI(sc, p, r, true, false); }
INS B i_FLDO(B ns, u32 p, Scope* sc) {
  if (!isNsp(ns)) thrM("Trying to read a field from non-namespace");
  B r = inc(ns_getU(ns, p));
  dec(ns);
  return r;
}
INS B i_VFYM(B o) { // TODO this and ALIM allocate and thus can error on OOM
  VfyObj* a = mm_alloc(sizeof(VfyObj), t_vfyObj);
  a->obj = o;
  return tag(a,OBJ_TAG);
}
INS B i_ALIM(B o, u32 l) {
  FldAlias* a = mm_alloc(sizeof(FldAlias), t_fldAlias);
  a->obj = o;
  a->p = l;
  return tag(a,OBJ_TAG);
}
INS B i_CHKV(B x, u32* bc, B* cStack) {
  if(q_N(x)) { POS_UPD; GS_UPD; thrM("Unexpected Nothing (·)"); }
  return x;
}
INS B i_FAIL(u32* bc, Scope* sc, B* cStack) {
  POS_UPD; GS_UPD; thrM(q_N(sc->vars[2])? "This block cannot be called monadically" : "This block cannot be called dyadically");
}
INS B i_RETD(Scope* sc) {
  return m_ns(ptr_inc(sc), ptr_inc(sc->body->nsDesc));
}

#undef INS
#undef P
#undef GSP
#undef GS_UPD
#undef POS_UPD





#include "x86_64.h"

#if USE_PERF
#include <unistd.h>
#include "../utils/file.h"
FILE* perf_map;
u32 perfid = 0;
#endif

static void* nvm_alloc(u64 sz) {
  // void* r = mmap(NULL, sz, PROT_EXEC|PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT, -1, 0);
  // if (r==MAP_FAILED) thrM("JIT: Failed to allocate executable memory");
  // return r;
  I8Arr* src = mmX_allocN(fsizeof(I8Arr,a,u8,sz), t_i8arr);
  src->ia = sz;
  arr_shVec((Arr*)src);
  return src->a;
}
void nvm_free(u8* ptr) {
  if (!USE_PERF) mmX_free((Value*)RFLD(ptr, I8Arr, a));
}



typedef struct SRef { B v; i32 p; } SRef;
#define SREF(V,P) ((SRef){.v=V,  .p=P})
typedef struct OptRes { u32* bc; u32* offset; B refs; } OptRes;
static OptRes opt(u32* bc0) {
  TSALLOC(SRef, stk, 8);
  TSALLOC(u8, actions, 64); // 1 per instruction; 0: nothing; 1: indicates return; 2: immediate SET; 3: immediate FN1_/FN2C; 4: FN2O; 5: replace with PUSH; 6: decrement 1 data; 7: SET_i+POPS merge; 10+N: ignore N data
  TSALLOC(u64, data, 64); // variable length; whatever things are needed for the specific action
  u8 rm_map[] = {10,10,10,11,12,6,6,11,99,99,11,12,13,14,15,16,17,18,19};
  #define RM(N) actions[N] = rm_map[actions[N]]
  u32* bc = bc0; usz pos = 0;
  while (true) {
    u32* sbc = bc;
    bool ret = false;
    u8 cact = 0;
    #define L64 ({ u64 r = bc[0] | ((u64)bc[1])<<32; bc+= 2; r; })
    #define S(N,I) SRef N = stk[TSSIZE(stk)-1-(I)];
    switch (*bc++) { case FN1Ci: case FN1Oi: case FN2Ci: case FN2Oi: err("optimization: didn't expect already immediate FN__");
      case ADDU: case ADDI: cact = 0; TSADD(stk,SREF(b(L64), pos)); break;
      case POPS: { assert(TSSIZE(actions) > 0);
        u64 asz = TSSIZE(actions);
        if (actions[asz-1]!=2) goto defIns;
        actions[asz-1] = 7;
        cact = 10;
        TSSIZE(stk)--;
        break;
      }
      case VARM: { u32 d = *bc++; u32 p = *bc++;
        TSADD(stk,SREF(tag((u64)d<<32 | (u32)p, VAR_TAG), pos));
        break;
      }
      case FN1C: case FN1O: { S(f,0)
        if (!isFun(f.v) || v(f.v)->type!=t_funBI) goto defIns;
        RM(f.p); cact = 3;
        TSADD(data, (u64) c(Fun, f.v)->c1);
        goto defIns;
      }
      case FN2C: { S(f,1)
        if (!isFun(f.v) || v(f.v)->type!=t_funBI) goto defIns;
        cact = 3; RM(f.p);
        TSADD(data, (u64) c(Fun, f.v)->c2);
        goto defIns;
      }
      case FN2O: { S(f,1)
        if (!isFun(f.v) || v(f.v)->type!=t_funBI) goto defIns;
        cact = 4; RM(f.p);
        TSADD(data, (u64) c(Fun, f.v)->c1);
        TSADD(data, (u64) c(Fun, f.v)->c2);
        goto defIns;
      }
      case MD1C: { S(f,0) S(m,1)
        if (f.p==-1 | m.p==-1) goto defIns;
        B d = m1_d(inc(m.v), inc(f.v));
        cact = 5; RM(f.p); RM(m.p);
        TSADD(data, d.u);
        TSSIZE(stk)--;
        stk[TSSIZE(stk)-1] = SREF(d, pos);
        break;
      }
      case MD2C: { S(f,0) S(m,1) S(g,2)
        if (f.p==-1 | m.p==-1 | g.p==-1) goto defIns;
        B d = m2_d(inc(m.v), inc(f.v), inc(g.v));
        cact = 5; RM(f.p); RM(m.p); RM(g.p);
        TSADD(data, d.u);
        TSSIZE(stk)-= 2;
        stk[TSSIZE(stk)-1] = SREF(d, pos);
        break;
      }
      case TR2D: { S(g,0) S(h,1)
        if (g.p==-1 | h.p==-1) goto defIns;
        B d = m_atop(inc(g.v), inc(h.v));
        cact = 5; RM(g.p); RM(h.p);
        TSADD(data, d.u);
        TSSIZE(stk)--;
        stk[TSSIZE(stk)-1] = SREF(d, pos);
        break;
      }
      case TR3D: case TR3O: { S(f,0) S(g,1) S(h,2)
        if (f.p==-1 | g.p==-1 | h.p==-1) goto defIns;
        if (q_N(f.v)) err("JIT optimization: didn't expect constant ·");
        B d = m_fork(inc(f.v), inc(g.v), inc(h.v));
        cact = 5; RM(f.p); RM(g.p); RM(h.p);
        TSADD(data, d.u);
        TSSIZE(stk)-= 2;
        stk[TSSIZE(stk)-1] = SREF(d, pos);
        break;
      }
      case SETN: case SETU: case SETM: case SETC: { S(s,0)
        if (!isVar(s.v)) goto defIns;
        cact = 2; RM(s.p);
        TSADD(data, s.v.u);
        TSSIZE(stk)-= SETM==*sbc? 2 : 1;
        break;
      }
      case ARRO: case ARRM: { i32 len = *bc++;
        bool allNum = len>0;
        for (i32 i = 0; i < len; i++) { S(c,i);
          if(c.p==-1) goto defIns;
          allNum&= isNum(c.v);
        }
        TSSIZE(stk)-= len-1; // huh, doing this beforehand works out nicely
        HArr_p h = m_harrUv(len);
        for (i32 i = 0; i < len; i++) { S(c,-i);
          h.a[i] = inc(c.v);
          RM(c.p);
        }
        B r = allNum? num_squeeze(h.b) : h.b;
        cact = 5;
        TSADD(data, r.u);
        stk[TSSIZE(stk)-1] = SREF(r, pos);
        break;
      }
      case RETN: case RETD:
        ret = true;
        cact = 1;
        goto defIns;
      default: defIns:;
        TSSIZE(stk)-= stackConsumed(sbc);
        i32 added = stackAdded(sbc);
        for (i32 i = 0; i < added; i++) TSADD(stk, SREF(bi_optOut, -1))
    }
    #undef S
    #undef L64
    TSADD(actions, cact);
    if (ret) break;
    bc = nextBC(sbc);
    pos++;
  }
  #undef RM
  TSFREE(stk);
  
  TSALLOC(u32, rbc, TSSIZE(actions));
  TSALLOC(u32, roff, TSSIZE(actions));
  B refs = emptyHVec();
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
      case 2: { assert(v==SETN|v==SETU|v==SETM|v==SETC);
        TSADD(rbc, v==SETN? SETNi : v==SETU? SETUi : v==SETC? SETCi : SETMi);
        u64 d = data[dpos++];
        TSADD(rbc, (u16)(d>>32));
        TSADD(rbc, (u32)d);
        break;
      }
      case 7: { assert(v==SETN|v==SETU|v==SETM|v==SETC);
        TSADD(rbc, v==SETN? SETNv : v==SETU? SETUv : v==SETC? SETCv : SETMv);
        u64 d = data[dpos++];
        TSADD(rbc, (u16)(d>>32));
        TSADD(rbc, (u32)d);
        break;
      }
      case 3: assert(v==FN1C|v==FN1O|v==FN2C);
        TSADD(rbc, v==FN1C? FN1Ci : v==FN1O? FN1Oi : FN2Ci);
        A64(data[dpos++]);
        break;
      case 4: assert(v==FN2O);
        TSADD(rbc, FN2Oi);
        A64(data[dpos++]);
        A64(data[dpos++]);
        break;
      case 5:;
        u64 on = data[dpos++]; B ob = b(on);
        TSADD(rbc, isVal(ob)? ADDI : ADDU);
        A64(on);
        if (isVal(ob)) refs = vec_addN(refs, ob);
        break;
      case 6:
        dec(b(data[dpos++]));
        break;
      case 10:
      case 11:case 12:case 13:case 14:case 15:case 16:case 17:case 18:case 19:
        dpos+= ctype-10;
        break;
      case 1: ret = true; goto def2; // return
      case 0: def2:; // do nothing
        TSADDA(rbc, sbc, ebc-sbc);
    }
    u64 added = TSSIZE(rbc)-psz;
    for (u64 i = 0; i < added; i++) TSADD(roff, sbc-bc0);
    #undef A64
    if (ret) break;
    bc = ebc;
  }
  bc = bc0; pos = 0;
  TSFREE(data);
  TSFREE(actions);
  if (a(refs)->ia==0) { dec(refs); refs=m_f64(0); }
  return (OptRes){.bc = rbc, .offset = roff, .refs = refs};
}
#undef SREF
void freeOpt(OptRes o) {
  TSFREEP(o.bc);
  TSFREEP(o.offset);
}

#define ASM_TEST 0 // make -j4 debug&&./BQN&&objdump -b binary -m i386 -M x86-64,intel --insn-width=12 -D --adjust-vma=$(cat asm_off) asm_bin | tail -n+8 | sed "$(cat asm_sed);s/\\t/ /g;s/.*: //"
#if ASM_TEST
  #undef WRITE_ASM
  #define WRITE_ASM 1
  static void write_asm(u8* p, u64 sz);
  static void asm_test() {
    asm_init();
    ALLOC_ASM(64);
    for (int i = 0; i < 16; i++) MOV4moi(i, 0, 0xaaaaaaaa);
    for (int i = 0; i < 16; i++) MOV4moi(i, 400, 0xaaaaaaaa);
    write_asm(bin, ASM_SIZE);
    exit(0);
  }
#endif

#if WRITE_ASM
  static void write_asm(u8* p, u64 sz) {
    i32* rp; B r = m_i32arrv(&rp, sz);
    for (u64 i = 0; i < sz; i++) rp[i] = p[i];
    path_wBytes(m_str8l("asm_bin"), r); dec(r);
    char off[20]; snprintf(off, 20, "%p", p);
    B o = m_str8l(off);
    path_wChars(m_str8l("asm_off"), o); dec(o);
    B s = emptyCVec();
    #define F(X) AFMT("s/%p$/%p   # i_" #X "/;", i_##X, i_##X);
    F(POPS)F(INC)F(FN1C)F(FN1O)F(FN2C)F(FN2O)F(FN1Oi)F(FN2Oi)F(ARR_0)F(ARR_p)F(DFND_0)F(DFND_1)F(DFND_2)F(MD1C)F(MD2C)F(MD2R)F(TR2D)F(TR3D)F(TR3O)F(NOVAR)F(EXTO)F(EXTU)F(SETN)F(SETU)F(SETM)F(SETC)F(SETH1)F(SETH2)F(PRED1)F(PRED2)F(SETNi)F(SETUi)F(SETMi)F(SETCi)F(SETNv)F(SETUv)F(SETMv)F(SETCv)F(FLDO)F(VFYM)F(ALIM)F(CHKV)F(FAIL)F(RETD)
    #undef F
    path_wChars(m_str8l("asm_sed"), s); dec(s);
  }
#endif

static void onJIT(Body* body, u8* binEx, u64 sz) {
  #if USE_PERF
    if (!perf_map) {
      B s = m_str8l("/tmp/perf-"); AFMT("%l.map", getpid());
      perf_map = file_open(s, "open", "wa");
      print(s); printf(": map\n");
      dec(s);
    }
    u32 bcPos = body->bl->map[0];
    // printf("JIT %d:\n", perfid);
    // vm_printPos(body->comp, bcPos, -1);
    fprintf(perf_map, N64x" "N64x" JIT %d: BC@%u\n", (u64)binEx, sz, perfid++, bcPos);
  #endif
  #if WRITE_ASM
    write_asm(binEx, sz);
    // exit(0);
  #endif
}

typedef B JITFn(B* cStack, Scope* sc);
static inline i32 maxi32(i32 a, i32 b) { return a>b?a:b; }
Nvm_res m_nvm(Body* body) {
  #if ASM_TEST
    asm_test();
  #endif
  asm_init();
  Reg r_CS  = R_P0;
  Reg r_SC  = R_P1;
  Reg r_ENV = R_P2;
  u64 pushAm = 0;
  PUSH(R_BP ); pushAm++; // idk, rbp; todo make gdb happy
  PUSH(r_ENV); pushAm++; // env pointer for quick bytecode pos updating
  PUSH(r_CS ); pushAm++; // starting gStack
  PUSH(r_SC ); pushAm++; // Scope* sc
  u64 lsz = 0; // local variable used up space
  #define ALLOCL(NAME,N) u64 NAME##Off = lsz; lsz+= (N)
  
  ALLOCL(pscs, (body->maxPSC)*8);
  while (((lsz+pushAm*8)&0xf) != 8) lsz++; // lazy way to make sure we're properly aligned
  SUBi(R_SP, lsz);
  
  MOV(r_CS, R_A0);
  MOV(r_SC, R_A1);
  MOV8rp(r_ENV, &envCurr);
  
  #define VAR(OFF,N) (OFF##Off + (N))
  #define VAR8(OFF,N) VAR(OFF,(N)*8)
  if (body->maxPSC) {
    MOV8mro(R_SP, R_A1, VAR(pscs,0));
    for (i32 i = 1; i < body->maxPSC; i++) {
      MOV8rmo(R_A1, R_A1, offsetof(Scope, psc));
      MOV8mro(R_SP, R_A1, VAR8(pscs,i));
    }
  }
  
  #define CCALL(F) CALLi((u64)(F))
  u32* origBC = body->bc;
  OptRes optRes = opt(origBC);
  i32 depth = 0;
  u32* bc = optRes.bc;
  i32 lGPos = 0; // last updated gStack offset
  
  TSALLOC(u64, retLbls, 1);
  while (true) {
    u32* s = bc;
    u32* n = nextBC(bc);
    u32 bcpos = optRes.offset[s-optRes.bc];
    u32 bodyOff = origBC - (u32*)body->bl->bc;
    u32* off = origBC + bcpos;
    bool ret = false;
    #define L64 ({ u64 r = bc[0] | ((u64)bc[1])<<32; bc+= 2; r; })
    // #define LEA0(O,I,OFF) { MOV(O,I); ADDI(O,OFF); }
    #define LEA0(O,I,OFF,Q) ({ i32 o=(OFF); if (Q||o) LEAi(O,I,o); o?O:I; })
    #define SPOSq(N) (maxi32(0, depth+(N)-1) * sizeof(B))
    #define SPOS(R,N,Q) LEA0(R, r_CS, SPOSq(N), Q) // load stack position N in register R; if Q==0, then might not write and instead return another register which will have the wanted value
    #define INV(N,D,F) SPOS(R_A##N, D, 1); CCALL(F)
    #define TOPpR(R) MOV(R,R_RES)
    #define TOPp TOPpR(R_A0)
    #define TOPs if (depth) { u8 t = SPOS(R_A3, 0, 0); MOV8mr(t, R_RES); }
    #define LSC(R,D) { if(D) MOV8rmo(R,R_SP,VAR8(pscs,D)); else MOV(R,r_SC); } // TODO return r_SC directly without a pointless mov
    #define INCV(R) INC4mo(R, offsetof(Value,refc)); // ADD4mi(R_A3, 1); CCALL(i_INC);
    #ifdef __BMI2__ // TODO move to runtime detection maybe
      #define INCB(R,T,U) IMM(T,0xfffffffffffffull);ADD(T,R);IMM(U,0x7fffffffffffeull);CMP(T,U);{J1(cA,lI);MOVi1l(U,0x30);BZHI(U,R,U);INCV(U);LBL1(lI);}
    #else
      #define INCB(R,T,U) IMM(T,0xfffffffffffffull);ADD(T,R);IMM(U,0x7fffffffffffeull);CMP(T,U);{J1(cA,lI);IMM(U,0xffffffffffffull);AND(U,R);INCV(U);LBL1(lI);}
    #endif
    // #define POS_UPD(R1,R2) IMM(R1, off); MOV8mro(r_ENV, R1, offsetof(Env,pos));
    #define POS_UPD(R1,R2) MOV4moi(r_ENV, offsetof(Env,pos), body->bl->map[bcpos + bodyOff]<<1 | 1);
    #define GS_SET(R) MOV8pr(&gStack, R)
    #define GET(R,P,U) { i32 p = SPOSq(-(P)); if (U && lGPos!=p) { Reg t=LEA0(R,r_CS,p,0); GS_SET(t); lGPos=p; if(U!=2) MOV8rm(R,t); } else if (U!=2) { MOV8rmo(R, r_CS, p); } }
    // use GET(R_A1,0,2); as GS_UPD when there's one argument, and GET(R_A3,-1,2); when there are zero arguments (i think?)
    #define NORES(D) if (depth>D) MOV8rm(R_RES, SPOS(R_A3, -D, 0)); // call at end if rax is unset; arg is removed stack item count
    switch (*bc++) {
      case POPS: TOPp;
        CCALL(i_POPS);
        // if (depth>1) MOV8rm(R_RES, SPOS(R_A3, -1, 0));
        NORES(1);
      break;
      case ADDI: TOPs; { u64 x = L64; IMM(R_RES, x); IMM(R_A3, v(b(x))); INCV(R_A3); break; } // (u64 v, S)
      case ADDU: TOPs; IMM(R_RES, L64); break;
      case FN1C: TOPp;                GET(R_A1,1,1); IMM(R_A2,off); CCALL(i_FN1C); break; // (     B f, B x, u32* bc)
      case FN1O: TOPp;                GET(R_A1,1,1); IMM(R_A2,off); CCALL(i_FN1O); break; // (     B f, B x, u32* bc)
      case FN2C: TOPp; GET(R_A1,1,0); GET(R_A2,2,1); IMM(R_A3,off); CCALL(i_FN2C); break; // (B w, B f, B x, u32* bc)
      case FN2O: TOPp; GET(R_A1,1,0); GET(R_A2,2,1); IMM(R_A3,off); CCALL(i_FN2O); break; // (B w, B f, B x, u32* bc)
      case FN1Ci: { u64 fn = L64; POS_UPD(R_A0,R_A3); MOV(R_A1, R_RES); GET(R_A2,0,2); CCALL(fn); } break;
      case FN2Ci: { u64 fn = L64; POS_UPD(R_A0,R_A3); MOV(R_A1, R_RES); GET(R_A2,1,1); CCALL(fn); } break;
      case FN1Oi:TOPp; GET(R_A1,0,2); IMM(R_A1,L64);                 IMM(R_A2,off); CCALL(i_FN1Oi); break; // (     B x, BB2B fm,           u32* bc)
      case FN2Oi:TOPp; GET(R_A1,1,1); IMM(R_A2,L64); IMM(R_A3, L64); IMM(R_A4,off); CCALL(i_FN2Oi); break; // (B w, B x, BB2B fm, BBB2B fd, u32* bc)
      case ARRM: case ARRO:; bool o = *(bc-1) == ARRO;
        u32 sz = *bc++;
        if      (sz==0     ) { TOPs; CCALL(i_ARR_0); } // unused with optimizations
        else if (sz==1 && o) { TOPp;        GET(R_A3,0,2); CCALL(m_vec1); } // (B a)
        else if (sz==2 && o) { TOPpR(R_A1); GET(R_A0,1,1); CCALL(m_vec2); } // (B a, B b)
        else                 { TOPp; IMM(R_A1, sz); lGPos=SPOSq(1-sz); INV(2,0,i_ARR_p); } // (B a, i64 sz, S)
        break;
      case DFND0: case DFND1: case DFND2: TOPs; // (u32* bc, Scope* sc, Block* bl)
        Block* bl = (Block*)L64;
        u64 fn = (u64)(bl->ty==0? i_DFND_0 : bl->ty==1? i_DFND_1 : bl->ty==2? i_DFND_2 : NULL);
        if (fn==0) err("JIT: Bad DFND argument");
        GET(R_A3,-1,2);
        IMM(R_A0,off); MOV(R_A1,r_SC); IMM(R_A2,bl); CCALL(fn);
        break;
      case MD1C: TOPp; GET(R_A1,1,1);                IMM(R_A2,off); CCALL(i_MD1C); break; // (B f,B m,      u32* bc)
      case MD2C: TOPp; GET(R_A1,1,0); GET(R_A2,2,1); IMM(R_A3,off); CCALL(i_MD2C); break; // (B f,B m, B g, u32* bc)
      case MD2R: TOPp; GET(R_A1,1,0);                               CCALL(i_MD2R); break; // (B m,     B g) // TODO these can actually error on OOM so should do something with bc/gStack
      case TR2D: TOPp; GET(R_A1,1,0);                               CCALL(i_TR2D); break; // (B g,     B h)
      case TR3D: TOPp; GET(R_A1,1,0); GET(R_A2,2,0);                CCALL(i_TR3D); break; // (B f,B g, B h)
      case TR3O: TOPp; GET(R_A1,1,0); GET(R_A2,2,0);                CCALL(i_TR3O); break; // (B f,B g, B h)
      case VARM: TOPs; { u64 d=*bc++; u64 p=*bc++; IMM(R_RES, tag((u64)d<<32 | (u32)p, VAR_TAG).u); } break;
      case EXTM: TOPs; { u64 d=*bc++; u64 p=*bc++; IMM(R_RES, tag((u64)d<<32 | (u32)p, EXT_TAG).u); } break;
      case VARO: TOPs; { u64 d=*bc++; u64 p=*bc++; LSC(R_A1,d);
        MOV8rmo(R_RES,R_A1,p*8+offsetof(Scope,vars)); // read variable
        INCB(R_RES,R_A2,R_A3); // increment refcount if one's needed
        if (d) { IMM(R_A2, bi_noVar.u); CMP(R_A2,R_RES); J1(cNE,lN); IMM(R_A0,off); INV(1,1,i_NOVAR); LBL1(lN); } // check for error
      } break;
      case VARU: TOPs; { u64 d=*bc++; u64 p=*bc++;
        LSC(R_A1,d);            MOV8rmo(R_RES,R_A1,p*8+offsetof(Scope,vars)); // read variable
        IMM(R_A2, bi_optOut.u); MOV8mro(R_A1, R_A2,p*8+offsetof(Scope,vars)); // set to bi_optOut
      } break;
      case EXTO: TOPs; { u64 d=*bc++; IMM(R_A0,*bc++); LSC(R_A1,d); IMM(R_A2,off); INV(3,1,i_EXTO); } break; // (u32 p, Scope* sc, u32* bc, S)
      case EXTU: TOPs; { u64 d=*bc++; IMM(R_A0,*bc++); LSC(R_A1,d);                  CCALL(i_EXTU); } break; // (u32 p, Scope* sc)
      case SETH1:TOPp; { u64 v1=L64;             GET(R_A1,1,1); LEAi(R_A2,R_SP,VAR(pscs,0)); IMM(R_A3,off); IMM(R_A4,v1);               CCALL(i_SETH1); IMM(R_A0, bi_okHdr.u); CMP(R_A0,R_RES); J4(cNE,l); TSADD(retLbls, l); break; } // (B s, B x, Scope** pscs, u32* bc, Body* v)
      case SETH2:TOPp; { u64 v1=L64; u64 v2=L64; GET(R_A1,1,1); LEAi(R_A2,R_SP,VAR(pscs,0)); IMM(R_A3,off); IMM(R_A4,v1); IMM(R_A5,v2); CCALL(i_SETH2); IMM(R_A0, bi_okHdr.u); CMP(R_A0,R_RES); J4(cNE,l); TSADD(retLbls, l); break; } // (B s, B x, Scope** pscs, u32* bc, Body* v1, Body* v2)
      case PRED1:TOPp; { u64 v1=L64;             GET(R_A1,0,2); MOV(R_A1,r_SC);              IMM(R_A2,off); IMM(R_A3,v1);               CCALL(i_PRED1); IMM(R_A0, bi_okHdr.u); CMP(R_A0,R_RES); J4(cNE,l); TSADD(retLbls, l); NORES(1); break; } // (B x, Scope** pscs, u32* bc, Body* v)
      case PRED2:TOPp; { u64 v1=L64; u64 v2=L64; GET(R_A1,0,2); MOV(R_A1,r_SC);              IMM(R_A2,off); IMM(R_A3,v1); IMM(R_A4,v2); CCALL(i_PRED2); IMM(R_A0, bi_okHdr.u); CMP(R_A0,R_RES); J4(cNE,l); TSADD(retLbls, l); NORES(1); break; } // (B x, Scope** pscs, u32* bc, Body* v1, Body* v2)
      case SETN: TOPp; GET(R_A1,1,1);                LEAi(R_A2,R_SP,VAR(pscs,0)); IMM(R_A3,off); CCALL(i_SETN); break; // (B s,      B x, Scope** pscs, u32* bc)
      case SETU: TOPp; GET(R_A1,1,1);                LEAi(R_A2,R_SP,VAR(pscs,0)); IMM(R_A3,off); CCALL(i_SETU); break; // (B s,      B x, Scope** pscs, u32* bc)
      case SETM: TOPp; GET(R_A1,1,1); GET(R_A2,2,1); LEAi(R_A3,R_SP,VAR(pscs,0)); IMM(R_A4,off); CCALL(i_SETM); break; // (B s, B f, B x, Scope** pscs, u32* bc)
      case SETC: TOPp; GET(R_A1,1,1);                LEAi(R_A2,R_SP,VAR(pscs,0)); IMM(R_A3,off); CCALL(i_SETC); break; // (B s, B f,      Scope** pscs, u32* bc)
      // TODO SETNi doesn't really need to update gStack
      case SETNi:TOPp; { u64 d=*bc++; u64 p=*bc++; GET(R_A1,0,2); LSC(R_A1,d); IMM(R_A2,p);                CCALL(i_SETNi);           break; } // (     B x, Scope* sc, u32 p         )
      case SETUi:TOPp; { u64 d=*bc++; u64 p=*bc++; GET(R_A1,0,2); LSC(R_A1,d); IMM(R_A2,p); IMM(R_A3,off); CCALL(i_SETUi);           break; } // (     B x, Scope* sc, u32 p, u32* bc)
      case SETMi:TOPp; { u64 d=*bc++; u64 p=*bc++; GET(R_A1,1,1); LSC(R_A2,d); IMM(R_A3,p); IMM(R_A4,off); CCALL(i_SETMi);           break; } // (B f, B x, Scope* sc, u32 p, u32* bc)
      case SETCi:TOPp; { u64 d=*bc++; u64 p=*bc++; GET(R_A1,0,2); LSC(R_A1,d); IMM(R_A2,p); IMM(R_A3,off); CCALL(i_SETCi);           break; } // (B f,      Scope* sc, u32 p, u32* bc)
      case SETNv:TOPp; { u64 d=*bc++; u64 p=*bc++; GET(R_A1,0,2); LSC(R_A1,d); IMM(R_A2,p);                CCALL(i_SETNv); NORES(1); break; } // (     B x, Scope* sc, u32 p, u32* bc)
      case SETUv:TOPp; { u64 d=*bc++; u64 p=*bc++; GET(R_A1,0,2); LSC(R_A1,d); IMM(R_A2,p); IMM(R_A3,off); CCALL(i_SETUv); NORES(1); break; } // (     B x, Scope* sc, u32 p, u32* bc)
      case SETMv:TOPp; { u64 d=*bc++; u64 p=*bc++; GET(R_A1,1,1); LSC(R_A2,d); IMM(R_A3,p); IMM(R_A4,off); CCALL(i_SETMv); NORES(2); break; } // (B f, B x, Scope* sc, u32 p, u32* bc)
      case SETCv:TOPp; { u64 d=*bc++; u64 p=*bc++; GET(R_A1,0,2); LSC(R_A1,d); IMM(R_A2,p); IMM(R_A3,off); CCALL(i_SETCv); NORES(1); break; } // (B f,      Scope* sc, u32 p, u32* bc)
      case FLDO: TOPp; GET(R_A1,0,2); IMM(R_A1,*bc++); MOV(R_A2,r_SC); CCALL(i_FLDO); break; // (B, u32 p, Scope* sc)
      case ALIM: TOPp; IMM(R_A1,*bc++); CCALL(i_ALIM); break; // (B, u32 l)
      case VFYM: TOPp;                  CCALL(i_VFYM); break; // (B)
      case CHKV: TOPp; IMM(R_A1,off); INV(2,0,i_CHKV); break; // (B, u32* bc, S)
      case RETD: if (lGPos!=0) GS_SET(r_CS); MOV(R_A0,r_SC); CCALL(i_RETD); ret=true; break; // (Scope* sc)
      case RETN: if (lGPos!=0) GS_SET(r_CS);                                ret=true; break;
      case FAIL: TOPs; IMM(R_A0,off); MOV(R_A1,r_SC);      INV(2,0,i_FAIL); ret=true; break;
      default: print_fmt("JIT: Unsupported bytecode %i/%S", *s, bc_repr(*s)); err("");
    }
    #undef GET
    #undef GS_SET
    #undef POS_UPD
    #undef INCB
    #undef INCV
    #undef LSC
    #undef TOPs
    #undef TOPp
    #undef INV
    #undef SPOS
    #undef L64
    if (n!=bc) err("JIT: Wrong parsing of bytecode");
    depth+= stackDiff(s);
    if (ret) break;
  }
  freeOpt(optRes);
  u64 retLblAm = TSSIZE(retLbls);
  for (u64 i = 0; i < retLblAm; i++) {
    u64 l = retLbls[i]; LBL4(l);
  }
  TSFREE(retLbls);
  ADDi(R_SP, lsz);
  POP(r_SC);
  POP(r_CS);
  POP(r_ENV);
  POP(R_BP);
  RET();
  #undef CCALL
  #undef VAR8
  #undef VAR
  #undef ALLOCL
  u64 sz = ASM_SIZE;
  u8* binEx = nvm_alloc(sz);
  asm_write(binEx, sz);
  asm_free();
  onJIT(body, binEx, sz);
  return (Nvm_res){.p = binEx, .refs = optRes.refs};
}
B evalJIT(Body* b, Scope* sc, u8* ptr) { // doesn't consume
  pushEnv(sc, b->bc);
  gsReserve(b->maxStack);
  // B* sp = gStack;
  B r = ((JITFn*)ptr)(gStack, sc);
  // if (sp!=gStack) thrM("uh oh");
  scope_dec(sc);
  popEnv();
  return r;
}
