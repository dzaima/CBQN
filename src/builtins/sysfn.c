#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <sys/wait.h>

#include "../core.h"
#include "../utils/hash.h"
#include "../utils/file.h"
#include "../utils/wyhash.h"
#include "../utils/mut.h"
#include "../builtins.h"
#include "../ns.h"
#include "../nfns.h"


static bool eqStr(B w, u32* x) {
  if (isAtm(w) || rnk(w)!=1) return false;
  SGetU(w)
  u64 i = 0;
  while (x[i]) {
    B c = GetU(w, i);
    if (!isC32(c) || x[i]!=(u32)c.u) return false;
    i++;
  }
  return i==a(w)->ia;
}


B type_c1(B t, B x) {
  i32 r = -1;
       if (isArr(x)) r = 0;
  else if (isF64(x)) r = 1;
  else if (isC32(x)) r = 2;
  else if (isFun(x)) r = 3;
  else if (isMd1(x)) r = 4;
  else if (isMd2(x)) r = 5;
  else if (isNsp(x)) r = 6;
  if (RARE(r==-1)) { print(x); err(": getting type"); }
  decR(x);
  return m_i32(r);
}

B decp_c1(B t, B x) {
  if (!isVal(x)) return m_hVec2(m_i32(-1), x);
  if (isPrim(x)) return m_hVec2(m_i32(0), x);
  return TI(x,decompose)(x);
}

B primInd_c1(B t, B x) {
  if (!isVal(x)) return m_i32(rtLen);
  if (isPrim(x)) { B r = m_i32(v(x)->flags-1); dec(x); return r; }
  dec(x);
  return m_i32(rtLen);
}

B nsFmt(B x);
#ifdef RT_WRAP
B rtWrap_unwrap(B x);
#endif
B glyph_c1(B t, B x) {
  if (!isVal(x)) return m_str32(U"(•Glyph: not given a function)");
  #ifdef RT_WRAP
    x = rtWrap_unwrap(x);
  #endif
  if (isPrim(x)) {
    B r = m_c32(U"+-×÷⋆√⌊⌈|¬∧∨<>≠=≤≥≡≢⊣⊢⥊∾≍⋈↑↓↕«»⌽⍉/⍋⍒⊏⊑⊐⊒∊⍷⊔!˙˜˘¨⌜⁼´˝`∘○⊸⟜⌾⊘◶⎉⚇⍟⎊"[v(x)->flags-1]);
    dec(x);
    return r;
  }
  u8 ty = v(x)->type;
  if (ty==t_funBI) { B r = fromUTF8l(pfn_repr(c(Fun,x)->extra)); dec(x); return r; }
  if (ty==t_md1BI) { B r = fromUTF8l(pm1_repr(c(Md1,x)->extra)); dec(x); return r; }
  if (ty==t_md2BI) { B r = fromUTF8l(pm2_repr(c(Md2,x)->extra)); dec(x); return r; }
  if (ty==t_nfn) { B r = nfn_name(x); dec(x); return r; }
  if (ty==t_fun_block) { dec(x); return m_str8l("(function block)"); }
  if (ty==t_md1_block) { dec(x); return m_str8l("(1-modifier block)"); }
  if (ty==t_md2_block) { dec(x); return m_str8l("(2-modifier block)"); }
  if (ty==t_ns) return nsFmt(x);
  return m_str32(U"(•Glyph: given object with unexpected type)");
}

B repr_c1(B t, B x) {
  if (isF64(x)) {
    NUM_FMT_BUF(buf, x.f);
    return fromUTF8(buf, strlen(buf));
  } else {
    #if FORMATTER
      return bqn_repr(x);
    #else
      thrM("•Repr: Cannot represent non-numbers without FORMATTER defined");
    #endif
  }
}

B fmt_c1(B t, B x) {
  #if FORMATTER
    return bqn_fmt(x);
  #else
    thrM("•Fmt isn't supported without FORMATTER defined");
  #endif
}

B fill_c1(B t, B x) {
  B r = getFillE(x);
  dec(x);
  return r;
}
B fill_c2(B t, B w, B x) { // TODO not set fill for typed arrays
  if (isArr(x)) {
    B fill = asFill(w);
    if (noFill(fill)) return x;
    return withFill(x, fill);
  }
  dec(w);
  return x;
}

B grLen_both(i64 ria, B x) {
  usz ia = a(x)->ia;
  SGetU(x)
  for (usz i = 0; i < ia; i++) {
    i64 c = o2i64u(GetU(x, i));
    if (c>ria) ria = c;
  }
  if (ria > (i64)(USZ_MAX-1)) thrOOM();
  ria++;
  B r;
  {
    u64* rp; r = m_bitarrv(&rp, ria);
    for (usz i = 0; i < BIT_N(ria); i++) rp[i] = 0;
    for (usz i = 0; i < ia; i++) {
      i64 n = o2i64u(GetU(x, i)); assert(n>=-1);
      if (n>=0) {
        if (bitp_get(rp,n)) { dec(r); goto r_i32; }
        bitp_set(rp,n,1);
      }
    }
    goto r_r;
  }
  r_i32:;
  i32* rp; r = m_i32arrv(&rp, ria);
  for (usz i = 0; i < ria; i++) rp[i] = 0;
  for (usz i = 0; i < ia; i++) {
    i64 n = o2i64u(GetU(x, i)); assert(n>=-1);
    if (n>=0) rp[n]++;
  }
  r_r: dec(x); return r;
}
B grLen_c1(B t,      B x) { return grLen_both(         -1, x); } // assumes valid arguments
B grLen_c2(B t, B w, B x) { return grLen_both(o2i64u(w)-1, x); } // assumes valid arguments

B grOrd_c2(B t, B w, B x) { // assumes valid arguments
  usz wia = a(w)->ia;
  usz xia = a(x)->ia;
  if (wia==0) { dec(w); dec(x); return emptyIVec(); }
  if (xia==0) { dec(w); return x; }
  SGetU(w)
  SGetU(x)
  TALLOC(usz, tmp, wia);
  tmp[0] = 0;
  for (usz i = 1; i < wia; i++) tmp[i] = tmp[i-1]+o2su(GetU(w,i-1));
  usz ria = tmp[wia-1]+o2su(GetU(w,wia-1));
  i32* rp; B r = m_i32arrv(&rp, ria);
  if (xia>=I32_MAX) thrM("⊔: Too large");
  for (usz i = 0; i < xia; i++) {
    i64 c = o2i64(GetU(x,i));
    if (c>=0) rp[tmp[c]++] = i;
  }
  dec(w); dec(x); TFREE(tmp);
  return r;
}

B asrt_c1(B t, B x) {
  if (LIKELY(isF64(x) && o2fu(x)==1)) return x;
  if (isF64(x)) thrM("Assertion error");
  thr(x);
}
B asrt_c2(B t, B w, B x) {
  if (LIKELY(isF64(x) && o2fu(x)==1)) { dec(w); return x; }
  dec(x);
  thr(w);
}
B casrt_c2(B t, B w, B x) {
  if (LIKELY(isF64(x) && o2fu(x)==1)) { dec(w); return x; }
  unwindCompiler();
  dec(x);
  if (isArr(w) && a(w)->ia==2) {
    B w0 = IGetU(w,0);
    if (isNum(w0)) {
      B s = IGet(w,1);
      AFMT("\n");
      usz pos = o2s(w0);
      s = vm_fmtPoint(comp_currSrc, s, comp_currPath, pos, pos+1);
      dec(w);
      thr(s);
    }
    if (isArr(w0) && rnk(w0)==1 && a(w0)->ia>=1) {
      B s = IGet(w,1); AFMT("\n");
      usz pos = o2s(IGetU(w0,0));
      s = vm_fmtPoint(comp_currSrc, s, comp_currPath, pos, pos+1);
      dec(w);
      thr(s);
    }
    if (isArr(w0) && rnk(w0)==2 && a(w0)->ia>=2) {
      B s = IGet(w,1); AFMT("\n");
      SGetU(w0)
      s = vm_fmtPoint(comp_currSrc, s, comp_currPath, o2s(GetU(w0,0)), o2s(GetU(w0,1))+1);
      dec(w);
      thr(s);
    }
  }
  thr(w);
}
B casrt_c1(B t, B x) {
  if (LIKELY(isF64(x) && o2fu(x)==1)) return x;
  casrt_c2(t, inc(x), x); UD;
}

B sys_c1(B t, B x);
B  out_c1(B t, B x) {
  if (isArr(x) && rnk(x)>1) thrF("•Out: Argument cannot have rank %i", rnk(x));
  printRaw(x); putchar('\n');
  return x;
}
B show_c1(B t, B x) {
  #if FORMATTER
    B fmt = bqn_fmt(inc(x));
    printRaw(fmt); dec(fmt);
  #else
    print(x);
  #endif
  putchar('\n');
  return x;
}

static B vfyStr(B x, char* name, char* arg) {
  if (isAtm(x) || rnk(x)!=1) thrF("%U: %U must be a character vector", name, arg);
  if (a(x)->type!=t_c32arr && a(x)->type!=t_c32slice) {
    usz ia = a(x)->ia;
    SGetU(x)
    for (usz i = 0; i < ia; i++) if (!isC32(GetU(x,i))) thrF("%U: %U must be a character vector", name, arg);
  }
  return x;
}

B cdPath;
static B args_path(B* fullpath, B w, char* name) { // consumes w, returns args, writes to fullpath
  if (!isArr(w) || rnk(w)!=1 || a(w)->ia>3) thrF("%U: 𝕨 must be a vector with at most 3 items, but had shape %H", name, w);
  usz ia = a(w)->ia;
  SGet(w)
  B path = ia>0? vfyStr(Get(w,0),name,"path"    ) : inc(cdPath);
  B file = ia>1? vfyStr(Get(w,1),name,"filename") : emptyCVec();
  B args = ia>2?        Get(w,2)                  : emptySVec();
  *fullpath = vec_join(vec_add(path, m_c32('/')), file);
  dec(w);
  return args;
}

B bqn_c1(B t, B x) {
  vfyStr(x, "•BQN", "𝕩");
  return bqn_exec(x, bi_N, bi_N);
}

B bqn_c2(B t, B w, B x) {
  vfyStr(x, "•BQN", "𝕩");
  B fullpath;
  B args = args_path(&fullpath, w, "•BQN");
  return bqn_exec(x, fullpath, args);
}

B cmp_c2(B t, B w, B x) {
  B r = m_i32(compare(w, x));
  dec(w); dec(x);
  return r;
}

B hash_c2(B t, B w, B x) {
  u64 secret[4]; make_secret(o2i64(w), secret);
  u64 rv = bqn_hash(x, secret);
  dec(x);
  i32* rp; B r = m_i32arrv(&rp, 4);
  rp[0] = (u16)(rv>>48); rp[1] = (u16)(rv>>32);
  rp[2] = (u16)(rv>>16); rp[3] = (u16)(rv    );
  return r;
}
B hash_c1(B t, B x) {
  u64 rv = bqn_hash(x, wy_secret);
  dec(x);
  i32* rp; B r = m_i32arrv(&rp, 4);
  rp[0] = (u16)(rv>>48); rp[1] = (u16)(rv>>32);
  rp[2] = (u16)(rv>>16); rp[3] = (u16)(rv    );
  return r;
}



static B rand_ns;
static i32 rand_a, rand_b;
static B rand_rangeName;   static NFnDesc* rand_rangeDesc;
static B rand_dealName;    static NFnDesc* rand_dealDesc;
static B rand_subsetName;  static NFnDesc* rand_subsetDesc;
#define RAND_START Scope* sc = c(NS,nfn_objU(t))->sc; \
                   u64 seed = sc->vars[rand_a].u | sc->vars[rand_b].u<<32;
#define RAND_END sc->vars[rand_a].u = seed>>32; \
                 sc->vars[rand_a].u = seed&0xFFFFFFFF;
B rand_range_c1(B t, B x) {
  i64 xv = o2i64(x);
  if (xv<0) thrM("(rand).Range: 𝕩 cannot be negative");
  RAND_START;
  u64 rnd = wyrand(&seed);
  RAND_END;
  return xv? m_f64(wy2u0k(rnd, xv)) : m_f64(wy2u01(rnd));
}
B rand_range_c2(B t, B w, B x) {
  usz am = 1;
  i64 max = o2i64(x);
  if (isArr(w)) {
    if (rnk(w) > 1) thrM("(rand).Range: 𝕨 must be a valid shape");
    SGetU(w);
    usz wia = a(w)->ia;
    for (u64 i = 0; i < wia; i++) mulOn(am, o2s(GetU(w, i)));
  } else {
    am = o2s(w);
  }
  
  RAND_START;
  Arr* r;
  if (max<1) {
    if (max!=0) thrM("(rand).Range: 𝕩 cannot be negative");
    f64* rp; r = m_f64arrp(&rp, am);
    for (usz i = 0; i < am; i++) rp[i] = wy2u01(wyrand(&seed));
  } else if (max > I32_MAX) {
    if (max >= 1LL<<53) thrM("(rand).Range: 𝕩 must be less than 2⋆53");
    f64* rp; r = m_f64arrp(&rp, am);
    for (usz i = 0; i < am; i++) rp[i] = wy2u0k(wyrand(&seed), max);
  } else {
    i32* rp; r = m_i32arrp(&rp, am);
    for (usz i = 0; i < am; i++) rp[i] = wy2u0k(wyrand(&seed), max);
  }

  RAND_END;
  if (isArr(w)) {
    usz wia = a(w)->ia;
    switch (wia) {
      case 0: { arr_shAlloc(r, 0); break; }
      case 1: { arr_shVec(r); break; }
      default: {
        usz* sh = arr_shAlloc(r, wia);
        SGetU(w);
        for (usz i = 0; i < wia; i++) sh[i] = o2su(GetU(w, i));
      }
    }
  } else {
    arr_shVec(r);
  }
  dec(w);
  return taga(r);
}

B rand_deal_c1(B t, B x) {
  i32 xi = o2i(x);
  if (RARE(xi<0)) thrM("(rand).Deal: Argument cannot be negative");
  if (xi==0) return emptyIVec();
  RAND_START;
  i32* rp; B r = m_i32arrv(&rp, xi);
  for (i64 i = 0; i < xi; i++) rp[i] = i;
  for (i64 i = 0; i < xi; i++) {
    i32 j = wy2u0k(wyrand(&seed), xi-i) + i;
    i32 c = rp[j];
    rp[j] = rp[i];
    rp[i] = c;
  }
  RAND_END;
  return r;
}

B rand_deal_c2(B t, B w, B x) {
  i32 wi = o2i(w);
  i32 xi = o2i(x);
  if (RARE(wi<0)) thrM("(rand).Deal: 𝕨 cannot be negative");
  if (RARE(xi<0)) thrM("(rand).Deal: 𝕩 cannot be negative");
  if (RARE(wi>xi)) thrM("(rand).Deal: 𝕨 cannot exceed 𝕩");
  if (wi==0) return emptyIVec();
  B r;
  RAND_START;
  if (wi > xi/64) {
    // Dense shuffle
    TALLOC(i32,s,xi);
    for (i64 i = 0; i < xi; i++) s[i] = i;
    for (i64 i = 0; i < wi; i++) {
      i32 j = wy2u0k(wyrand(&seed), xi-i) + i;
      i32 c = s[j];
      s[j] = s[i];
      s[i] = c;
    }
    i32* rp; r = m_i32arrv(&rp, wi);
    memcpy(rp, s, wi*4);
    TFREE(s);
  } else {
    // Hash-based shuffle
    i32* rp; r = m_i32arrv(&rp, wi);
    i64 sz = 1;
    while (sz < wi*2) sz*= 2;
    TALLOC(i32, hash, 2*sz); i32* val = hash+1;
    for (i64 i = 0; i < 2*sz; i++) hash[i] = 0;
    for (i64 i = 0; i < wi; i++) rp[i] = i;
    u64 mask = 2*(sz-1);
    for (i64 i = 0; i < wi; i++) {
      u64 j = wy2u0k(wyrand(&seed), xi-i) + i;
      if (j<(u64)wi) {
        i32 c = rp[j];
        rp[j] = rp[i];
        rp[i] = c;
      } else {
        u64 p = 2*j;
        i32 prev = j;
        while (true) {
          p&= mask;
          i32 h = hash[p];
          if (h==0) { hash[p] = j; break; }
          if ((u64)h==j) { prev = val[p]; break; }
          p+= 2;
        }
        val[p] = rp[i];
        rp[i] = prev;
      }
    }
    TFREE(hash);
  }
  RAND_END;
  return r;
}

B rand_subset_c2(B t, B w, B x) {
  i32 wi = o2i(w);
  i32 xi = o2i(x);
  if (RARE(wi<0)) thrM("(rand).Subset: 𝕨 cannot be negative");
  if (RARE(xi<0)) thrM("(rand).Subset: 𝕩 cannot be negative");
  if (RARE(wi>xi)) thrM("(rand).Subset: 𝕨 cannot exceed 𝕩");
  if (wi==0) return emptyIVec();
  B r;
  if (wi==xi) { // Only one complete subset; will hang without this
    i32* rp; r = m_i32arrv(&rp, wi);
    for (i64 i = 0; i < wi; i++) rp[i] = i;
    return r;
  }
  RAND_START;
  if (wi > xi/8) {
    // Bit set (as bytes)
    TALLOC(u8, set, xi);
    bool invert = wi > xi/2;
    i32 wn = invert ? xi-wi : wi;
    for (i64 i = 0; i < xi; i++) set[i] = 0;
    for (i32 i = xi-wn; i < xi; i++) {
      i32 j = wy2u0k(wyrand(&seed), i+1);
      if (set[j]) j=i;
      set[j] = 1;
    }
    i32* rp; r = m_i32arrv(&rp, wi);
    if (!invert) { for (i64 i = 0; i < xi; i++) if ( set[i]) *rp++=i; }
    else         { for (i64 i = 0; i < xi; i++) if (!set[i]) *rp++=i; }
    TFREE(set);
  } else {
    // Sorted "hash" set
    u64 sh = 0;
    for (u64 xt=xi/4; xt>=(u64)wi; xt>>=1) sh++;
    u64 sz = ((xi-1)>>sh)+1 + wi;
    TALLOC(i32, hash, sz);
    for (u64 i = 0; i < sz; i++) hash[i] = xi;
    for (i32 i = xi-wi; i < xi; i++) {
      i32 j = wy2u0k(wyrand(&seed), i+1);
      u64 p = (u64)j >> sh;
      while (true) {
        i32 h = hash[p];
        if (LIKELY(j<h)) {
          hash[p]=j;
          while (RARE(h!=xi)) { p++; i32 ht=hash[p]; hash[p]=h; h=ht; }
          break;
        }
        if (h==j) { if (j==i) break; j=i; p=(u64)j>>sh; continue; }
        p++;
      }
    }
    i32* rp; r = m_i32arrv(&rp, wi);
    for (u64 i = 0; i < sz; i++) if (hash[i]!=xi) *rp++=hash[i];
    TFREE(hash);
  }
  RAND_END;
  return r;
}

static NOINLINE void rand_init() {
  rand_ns = bqn_exec(m_str32(U"{a←𝕨⋄b←𝕩⋄range⇐0⋄deal⇐0⋄subset⇐0}"), emptyCVec(), emptySVec()); gc_add(rand_ns);
  rand_rangeName  = m_str8l("range");  gc_add(rand_rangeName);  rand_rangeDesc  = registerNFn(m_str8l("(rand).Range"), rand_range_c1, rand_range_c2);
  rand_dealName   = m_str8l("deal");   gc_add(rand_dealName);   rand_dealDesc   = registerNFn(m_str8l("(rand).Deal"),   rand_deal_c1, rand_deal_c2);
  rand_subsetName = m_str8l("subset"); gc_add(rand_subsetName); rand_subsetDesc = registerNFn(m_str8l("(rand).Subset"),       c1_bad, rand_subset_c2);
  B tmp = c2(rand_ns, m_f64(0), m_f64(0));
  rand_a = ns_pos(tmp, m_str8l("a"));
  rand_b = ns_pos(tmp, m_str8l("b"));
  dec(tmp);
}
B makeRand_c1(B t, B x) {
  if (!isNum(x)) thrM("•MakeRand: 𝕩 must be a number");
  if (rand_ns.u==0) rand_init();
  B r = c2(rand_ns, b(x.u>>32), b(x.u&0xFFFFFFFF));
  ns_set(r, rand_rangeName,   m_nfn(rand_rangeDesc,   incG(r)));
  ns_set(r, rand_dealName,    m_nfn(rand_dealDesc,    incG(r)));
  ns_set(r, rand_subsetName,  m_nfn(rand_subsetDesc,  incG(r)));
  return r;
}
static B randNS;
B getRandNS() {
  if (randNS.u == 0) {
    #if RANDSEED==0
      randNS = c1(bi_makeRand, m_f64(nsTime()));
    #else
      randNS = c1(bi_makeRand, m_f64(RANDSEED));
    #endif
    gc_add(randNS);
  }
  return incG(randNS);
}
static NFnDesc* reBQNDesc;
B reBQN_c1(B t, B x) {
  if (!isNsp(x)) thrM("•ReBQN: Argument must be a namespace");
  B replStr = m_str8l("repl");
  B repl = ns_getNU(x, replStr, false); dec(replStr);
  i32 replVal = q_N(repl) || eqStr(repl,U"none")? 0 : eqStr(repl,U"strict")? 1 : eqStr(repl,U"loose")? 2 : 3;
  if (replVal==3) thrM("•ReBQN: Invalid repl value");
  dec(x);
  Block* initBlock = bqn_comp(m_str8l("\"(REPL initializer)\""), inc(cdPath), m_f64(0));
  B scVal;
  if (replVal==0) {
    scVal = bi_N;
  } else {
    Scope* sc = m_scope(initBlock->bodies[0], NULL, 0, 0, NULL);
    scVal = tag(sc,OBJ_TAG);
  }
  ptr_dec(initBlock);
  return m_nfn(reBQNDesc, m_hVec2(m_f64(replVal), scVal));
}
B repl_c2(B t, B w, B x) {
  vfyStr(x, "REPL", "𝕩");
  B o = nfn_objU(t);
  B* op = harr_ptr(o);
  i32 replMode = o2iu(op[0]);
  Scope* sc = c(Scope, op[1]);
  
  B fullpath;
  B args = args_path(&fullpath, w, "REPL");
  
  B res;
  if (replMode>0) {
    Block* block = bqn_compSc(x, fullpath, args, sc, replMode==2);
    ptr_dec(sc->body);
    sc->body = ptr_inc(block->bodies[0]);
    res = execBlockInline(block, sc);
    ptr_dec(block);
  } else {
    res = bqn_exec(x, fullpath, args);
  }
  
  return res;
}
B repl_c1(B t, B x) {
  return repl_c2(t, emptyHVec(), x);
}

static NFnDesc* fileAtDesc;
B fileAt_c1(B d, B x) {
  return path_rel(nfn_objU(d), x);
}
B fileAt_c2(B d, B w, B x) {
  vfyStr(w,"(file).At","𝕨");
  B r = path_rel(w, x);
  dec(w);
  return r;
}
static NFnDesc* fCharsDesc;
B fchars_c1(B d, B x) {
  return file_chars(path_rel(nfn_objU(d), x));
}
B fchars_c2(B d, B w, B x) {
  if (!isArr(x)) thrM("•FChars: Non-array 𝕩");
  B p = path_rel(nfn_objU(d), w);
  file_wChars(inc(p), x);
  dec(x);
  return p;
}
static NFnDesc* fBytesDesc;
B fbytes_c1(B d, B x) {
  I8Arr* tf = file_bytes(path_rel(nfn_objU(d), x));
  usz ia = tf->ia; u8* p = (u8*)tf->a;
  u8* rp; B r = m_c8arrv(&rp, ia);
  for (i64 i = 0; i < ia; i++) rp[i] = p[i];
  ptr_dec(tf);
  return r;
}
B fbytes_c2(B d, B w, B x) {
  if (!isArr(x)) thrM("•FBytes: Non-array 𝕩");
  B p = path_rel(nfn_objU(d), w);
  file_wBytes(inc(p), x);
  dec(x);
  return p;
}
static NFnDesc* fLinesDesc;
B flines_c1(B d, B x) {
  return file_lines(path_rel(nfn_objU(d), x));
}
B flines_c2(B d, B w, B x) {
  if (!isArr(x)) thrM("•FLines: Non-array 𝕩");
  B nl, s = emptyCVec();
  usz ia = a(x)->ia;
  SGet(x)
  for (u64 i = 0; i < ia; i++) {
    nl = Get(x, i);
    if (!isArr(nl)) thrM("•FLines: Non-array element of 𝕩");
    s = vec_join(s, nl);
    //if (windows) s = vec_add(s, m_c32('\r')); TODO figure out whether or not this is a thing that should be done
    s = vec_add(s, m_c32('\n'));
  }
  dec(x);
  B p = path_rel(nfn_objU(d), w);
  file_wChars(inc(p), s);
  dec(s);
  return p;
}
static NFnDesc* importDesc;
B import_c1(B d,      B x) { return bqn_execFile(path_rel(nfn_objU(d), x), emptySVec()); }
B import_c2(B d, B w, B x) { return bqn_execFile(path_rel(nfn_objU(d), x), w); }
static NFnDesc* listDesc;
B list_c1(B d, B x) {
  return file_list(path_rel(nfn_objU(d), x));
}

B unixTime_c1(B t, B x) {
  dec(x);
  return m_f64(time(NULL));
}
B monoTime_c1(B t, B x) {
  dec(x);
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return m_f64(ts.tv_sec + ts.tv_nsec*1e-9);
}
B delay_c1(B t, B x) {
  f64 sf = o2f(x);
  if (sf<0 || sf>1ULL<<63) thrF("•Delay: Bad argument: %f", sf);
  struct timespec ts,ts0;
  u64 s = (u64)sf;
  ts.tv_sec = (u64)sf;
  ts.tv_nsec = (u64)((sf-s)*1e9);
  clock_gettime(CLOCK_MONOTONIC, &ts0);
  nanosleep(&ts, &ts);
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return m_f64(ts.tv_sec-ts0.tv_sec+(ts.tv_nsec-ts0.tv_nsec)*1e-9);
}
B exit_c1(B t, B x) {
  #ifdef HEAP_VERIFY
    printf("(heapverify doesn't run on •Exit)\n");
  #endif
  bqn_exit(q_i32(x)? o2i(x) : 0);
}
B getLine_c1(B t, B x) {
  dec(x);
  char* ln = NULL;
  size_t gl = 0;
  i64 read = getline(&ln, &gl, stdin);
  if (read<=0 || ln[0]==0) {
    if (ln) free(ln);
    return m_c32(0);
  }
  B r = fromUTF8(ln, strlen(ln)-1);
  free(ln);
  return r;
}

B fromUtf8_c1(B t, B x) {
  if (!isArr(x)) thrM("•FromUTF8: Argument must be a character or number array");
  usz ia = a(x)->ia;
  TALLOC(char, chrs, ia);
  SGetU(x)
  for (u64 i = 0; i < ia; i++) {
    B c = GetU(x,i);
    if (isC32(c)) {
      u32 v = o2cu(c);
      if (v>=256) thrF("•FromUTF8: Argument contained a character with codepoint %i", v);
      chrs[i] = v;
    } else {
      i32 v = o2i(c);
      if (v<=-128 | v>=256) thrF("•FromUTF8: Argument contained %i", v);
      chrs[i] = v&0xff;
    }
  }
  B r = fromUTF8(chrs, ia);
  dec(x);
  TFREE(chrs);
  return r;
}

extern char** environ;

#if __has_include(<spawn.h>)
#include <spawn.h>
B sh_c1(B t, B x) {
  if (isAtm(x) || rnk(x)>1) thrM("•SH: 𝕩 must be a vector of strings");
  usz xia = a(x)->ia;
  if (xia==0) thrM("•SH: 𝕩 must have at least one item");
  TALLOC(char*, argv, xia+1);
  SGetU(x)
  for (u64 i = 0; i < xia; i++) {
    B c = GetU(x, i);
    if (isAtm(c) || rnk(c)!=1) thrM("•SH: 𝕩 must be a vector of strings");
    u64 len = utf8lenB(c);
    TALLOC(char, cstr, len+1);
    toUTF8(c, cstr);
    cstr[len] = 0;
    argv[i] = cstr;
  }
  argv[xia] = NULL;
  
  
  int p_out[2];
  int p_err[2];
  if (pipe(p_out) || pipe(p_err)) thrM("•SH: Failed to create process: Couldn't create pipes");
  
  posix_spawn_file_actions_t a; posix_spawn_file_actions_init(&a);
  posix_spawn_file_actions_addclose(&a, p_out[0]); posix_spawn_file_actions_adddup2(&a, p_out[1], 1); posix_spawn_file_actions_addclose(&a, p_out[1]);
  posix_spawn_file_actions_addclose(&a, p_err[0]); posix_spawn_file_actions_adddup2(&a, p_err[1], 2); posix_spawn_file_actions_addclose(&a, p_err[1]);
  
  pid_t pid;
  if(posix_spawnp(&pid, argv[0], &a, NULL, argv, environ) != 0) thrF("•SH: Failed to create process: %S", strerror(errno));
  
  for (u64 i = 0; i < xia; i++) TFREE(argv[i]);
  TFREE(argv);
  close(p_out[1]); B s_out = emptyCVec();
  close(p_err[1]); B s_err = emptyCVec();
  
  const u64 bufsz = 1024;
  TALLOC(char, buf, bufsz);
  struct pollfd ps[] = {{.fd=p_out[0], .events=POLLIN}, {.fd=p_err[0], .events=POLLIN}};
  while (poll(&ps[0], 2, -1) > 0) {
    bool stop=true;
    if (ps[0].revents & POLLIN) while(true) { stop=false; u64 len = read(p_out[0], &buf[0], bufsz); if(len==0)break; s_out = vec_join(s_out, fromUTF8(buf, len)); }
    if (ps[1].revents & POLLIN) while(true) { stop=false; u64 len = read(p_err[0], &buf[0], bufsz); if(len==0)break; s_err = vec_join(s_err, fromUTF8(buf, len)); }
    if (ps[0].revents & POLLHUP  &&  ps[1].revents & POLLHUP) break; // idk, made osx work
    if (stop) break;
  }
  TFREE(buf);
  
  int status;
  waitpid(pid, &status, 0);
  
  posix_spawn_file_actions_destroy(&a);
  dec(x);
  return m_hVec3(m_i32(WEXITSTATUS(status)), s_out, s_err);
}
#else
B sh_c1(B t, B x) {
  thrM("•SH: CBQN was built without <spawn.h>");
}
#endif
B sh_c2(B t, B w, B x) {
  dec(w);
  return sh_c1(t, x);
}


B getInternalNS(void);
B getMathNS(void);

static B file_nsGen;
B sys_c1(B t, B x) {
  assert(isArr(x));
  usz i = 0;
  HArr_p r = m_harrs(a(x)->ia, &i);
  SGetU(x)
  B fileNS = m_f64(0);
  B path = m_f64(0);
  B wdpath = m_f64(0);
  #define REQ_PATH ({ if(!path.u) path = path_abs(path_dir(inc(comp_currPath))); path; })
  for (; i < a(x)->ia; i++) {
    B c = GetU(x,i);
    if (eqStr(c, U"out")) r.a[i] = incG(bi_out);
    else if (eqStr(c, U"show")) r.a[i] = incG(bi_show);
    else if (eqStr(c, U"exit")) r.a[i] = incG(bi_exit);
    else if (eqStr(c, U"getline")) r.a[i] = incG(bi_getLine);
    else if (eqStr(c, U"file")) {
      if(!fileNS.u) {
        REQ_PATH;
        #define F(X) m_nfn(X##Desc, inc(path))
        B arg =    m_caB(6, (B[]){q_N(path)? m_c32(0) : inc(path), F(fileAt), F(list), F(fBytes), F(fChars), F(fLines)});
        #undef F
        fileNS = c1(file_nsGen,arg);
      }
      r.a[i] = inc(fileNS);
    }
    else if (eqStr(c, U"wdpath")) {
      if (!wdpath.u) wdpath = path_abs(inc(cdPath));
      r.a[i] = inc(wdpath);
    }
    else if (eqStr(c, U"internal")) r.a[i] = getInternalNS();
    else if (eqStr(c, U"math")) r.a[i] = getMathNS();
    else if (eqStr(c, U"type")) r.a[i] = incG(bi_type);
    else if (eqStr(c, U"sh")) r.a[i] = incG(bi_sh);
    else if (eqStr(c, U"decompose")) r.a[i] = incG(bi_decp);
    else if (eqStr(c, U"while")) r.a[i] = incG(bi_while);
    else if (eqStr(c, U"primind")) r.a[i] = incG(bi_primInd);
    else if (eqStr(c, U"bqn")) r.a[i] = incG(bi_bqn);
    else if (eqStr(c, U"cmp")) r.a[i] = incG(bi_cmp);
    else if (eqStr(c, U"unixtime")) r.a[i] = incG(bi_unixTime);
    else if (eqStr(c, U"monotime")) r.a[i] = incG(bi_monoTime);
    else if (eqStr(c, U"timed")) r.a[i] = incG(bi_timed);
    else if (eqStr(c, U"delay")) r.a[i] = incG(bi_delay);
    else if (eqStr(c, U"hash")) r.a[i] = incG(bi_hash);
    else if (eqStr(c, U"repr")) r.a[i] = incG(bi_repr);
    else if (eqStr(c, U"fmt")) r.a[i] = incG(bi_fmt);
    else if (eqStr(c, U"glyph")) r.a[i] = incG(bi_glyph);
    else if (eqStr(c, U"makerand")) r.a[i] = incG(bi_makeRand);
    else if (eqStr(c, U"rand")) r.a[i] = getRandNS();
    else if (eqStr(c, U"rebqn")) r.a[i] = incG(bi_reBQN);
    else if (eqStr(c, U"fromutf8")) r.a[i] = incG(bi_fromUtf8);
    else if (eqStr(c, U"path")) r.a[i] = inc(REQ_PATH);
    else if (eqStr(c, U"fchars")) r.a[i] = m_nfn(fCharsDesc, inc(REQ_PATH));
    else if (eqStr(c, U"fbytes")) r.a[i] = m_nfn(fBytesDesc, inc(REQ_PATH));
    else if (eqStr(c, U"flines")) r.a[i] = m_nfn(fLinesDesc, inc(REQ_PATH));
    else if (eqStr(c, U"import")) r.a[i] = m_nfn(importDesc, inc(REQ_PATH));
    else if (eqStr(c, U"args")) {
      if(q_N(comp_currArgs)) thrM("No arguments present for •args");
      r.a[i] = inc(comp_currArgs);
    } else { dec(x); thrF("Unknown system function •%R", c); }
  }
  #undef REQ_PATH
  dec(fileNS);
  dec(path);
  dec(wdpath);
  return harr_fcd(r, x);
}

B cdPath;
void sysfn_init() {
  cdPath = m_str8(1, "."); gc_add(cdPath);
  fCharsDesc = registerNFn(m_str8l("(file).Chars"), fchars_c1, fchars_c2);
  fileAtDesc = registerNFn(m_str8l("(file).At"), fileAt_c1, fileAt_c2);
  fLinesDesc = registerNFn(m_str8l("(file).Lines"), flines_c1, flines_c2);
  fBytesDesc = registerNFn(m_str8l("(file).Bytes"), fbytes_c1, fbytes_c2);
  importDesc = registerNFn(m_str32(U"•Import"), import_c1, import_c2);
  reBQNDesc = registerNFn(m_str8l("(REPL)"), repl_c1, repl_c2);
  listDesc = registerNFn(m_str32(U"•file.List"), list_c1, c2_bad);
}
void sysfnPost_init() {
  file_nsGen = bqn_exec(m_str32(U"{⟨path,At,List,Bytes,Chars,Lines⟩⇐𝕩}"), emptyCVec(), emptySVec()); gc_add(file_nsGen);
}
