#include <unistd.h>
#include <poll.h>
#include <errno.h>


#include "../core.h"
#include "../utils/hash.h"
#include "../utils/file.h"
#include "../utils/wyhash.h"
#include "../utils/mut.h"
#include "../utils/time.h"
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
  if (RARE(r==-1)) {
    if (x.u == bi_optOut.u) thrM("Reading variable that was optimized out by F↩ after error");
    print(x); err(": getting type");
  }
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
    decG(x);
    return r;
  }
  u8 ty = v(x)->type;
  if (ty==t_funBI) { B r = fromUTF8l(pfn_repr(c(Fun,x)->extra)); decG(x); return r; }
  if (ty==t_md1BI) { B r = fromUTF8l(pm1_repr(c(Md1,x)->extra)); decG(x); return r; }
  if (ty==t_md2BI) { B r = fromUTF8l(pm2_repr(c(Md2,x)->extra)); decG(x); return r; }
  if (ty==t_nfn) { B r = nfn_name(x); decG(x); return r; }
  if (ty==t_funBl) { decG(x); return m_str8l("(function block)"); }
  if (ty==t_md1Bl) { decG(x); return m_str8l("(1-modifier block)"); }
  if (ty==t_md2Bl) { decG(x); return m_str8l("(2-modifier block)"); }
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
        if (bitp_get(rp,n)) { decG(r); goto r_i32; }
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
  r_r: decG(x); return r;
}
B grLen_c1(B t,      B x) { return grLen_both(         -1, x); } // assumes valid arguments
B grLen_c2(B t, B w, B x) { return grLen_both(o2i64u(w)-1, x); } // assumes valid arguments

B grOrd_c2(B t, B w, B x) { // assumes valid arguments
  usz wia = a(w)->ia;
  usz xia = a(x)->ia;
  if (wia==0) { decG(w); decG(x); return emptyIVec(); }
  if (xia==0) { decG(w); return x; }
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
  decG(w); decG(x); TFREE(tmp);
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
B out_c1(B t, B x) {
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
  if (!elChr(TI(x,elType))) {
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
  decG(w);
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
  i32* rp; B r = m_i32arrv(&rp, 2);
  rp[0] = (u32)(rv>>32);
  rp[1] = (u32)(rv    );
  return r;
}
B hash_c1(B t, B x) {
  u64 rv = bqn_hash(x, wy_secret);
  dec(x);
  i32* rp; B r = m_i32arrv(&rp, 2);
  rp[0] = (u32)(rv>>32);
  rp[1] = (u32)(rv    );
  return r;
}



static Body* rand_ns;
static B rand_rangeName;   static NFnDesc* rand_rangeDesc;
static B rand_dealName;    static NFnDesc* rand_dealDesc;
static B rand_subsetName;  static NFnDesc* rand_subsetDesc;
#define RAND_START Scope* sc = c(NS,nfn_objU(t))->sc; \
                   u64 seed = sc->vars[0].u | sc->vars[1].u<<32;
#define RAND_END sc->vars[0].u = seed>>32; \
                 sc->vars[1].u = seed&0xFFFFFFFF;
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
  rand_ns = m_nnsDesc("seed1", "seed2", "range", "deal", "subset");
  NSDesc* d = rand_ns->nsDesc;
  d->expGIDs[0] = d->expGIDs[1] = -1;
  rand_rangeName  = m_str8l("range");  gc_add(rand_rangeName);  rand_rangeDesc  = registerNFn(m_str8l("(rand).Range"), rand_range_c1, rand_range_c2);
  rand_dealName   = m_str8l("deal");   gc_add(rand_dealName);   rand_dealDesc   = registerNFn(m_str8l("(rand).Deal"),   rand_deal_c1, rand_deal_c2);
  rand_subsetName = m_str8l("subset"); gc_add(rand_subsetName); rand_subsetDesc = registerNFn(m_str8l("(rand).Subset"),       c1_bad, rand_subset_c2);
}
B makeRand_c1(B t, B x) {
  if (!isNum(x)) thrM("•MakeRand: 𝕩 must be a number");
  if (rand_ns==NULL) rand_init();
  B r = m_nns(rand_ns, b(x.u>>32), b(x.u&0xFFFFFFFF), m_nfn(rand_rangeDesc, bi_N), m_nfn(rand_dealDesc, bi_N), m_nfn(rand_subsetDesc, bi_N));
  Scope* sc = c(NS,r)->sc;
  for (i32 i = 2; i < 5; i++) nfn_swapObj(sc->vars[i], inc(r));
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
  B repl = ns_getC(x, "repl");
  B prim = ns_getC(x, "primitives");
  i32 replVal = q_N(repl) || eqStr(repl,U"none")? 0 : eqStr(repl,U"strict")? 1 : eqStr(repl,U"loose")? 2 : 3;
  if (replVal==3) thrM("•ReBQN: Invalid repl value");
  Block* initBlock = bqn_comp(m_str8l("\"(REPL initializer)\""), inc(cdPath), m_f64(0));
  B scVal;
  if (replVal==0) {
    scVal = bi_N;
  } else {
    Scope* sc = m_scope(initBlock->bodies[0], NULL, 0, 0, NULL);
    scVal = tag(sc,OBJ_TAG);
  }
  ptr_dec(initBlock);
  HArr_p d = m_harrUv(5); d.a[0] = m_f64(replVal); d.a[1] = scVal;
  d.a[2]=d.a[3]=d.a[4]=bi_N;
  init_comp(d.a+2, prim);
  decG(x);
  return m_nfn(reBQNDesc, d.b);
}
B repl_c2(B t, B w, B x) {
  vfyStr(x, "REPL", "𝕩");
  B fullpath;
  B args = args_path(&fullpath, w, "REPL");
  return rebqn_exec(x, fullpath, args, nfn_objU(t));
}
B repl_c1(B t, B x) {
  return repl_c2(t, emptyHVec(), x);
}

#if CATCH_ERRORS
B lastErrMsg;
B currentError_c1(B t, B x) {
  dec(x);
  if (q_N(lastErrMsg)) thrM("•CurrentError: Not currently within any ⎊");
  return inc(lastErrMsg);
}
#else
B currentError_c1(B t, B x) { thrM("•CurrentError: No errors as error catching has been disabled"); }
#endif

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
  return path_chars(path_rel(nfn_objU(d), x));
}
B fchars_c2(B d, B w, B x) {
  if (!isArr(x)) thrM("•FChars: Non-array 𝕩");
  B p = path_rel(nfn_objU(d), w);
  path_wChars(inc(p), x);
  dec(x);
  return p;
}
static NFnDesc* fBytesDesc;
B fbytes_c1(B d, B x) {
  I8Arr* tf = path_bytes(path_rel(nfn_objU(d), x));
  usz ia = tf->ia; u8* p = (u8*)tf->a;
  u8* rp; B r = m_c8arrv(&rp, ia);
  for (i64 i = 0; i < ia; i++) rp[i] = p[i];
  ptr_dec(tf);
  return r;
}
B fbytes_c2(B d, B w, B x) {
  if (!isArr(x)) thrM("•FBytes: Non-array 𝕩");
  B p = path_rel(nfn_objU(d), w);
  path_wBytes(inc(p), x);
  dec(x);
  return p;
}
static NFnDesc* fLinesDesc;
B flines_c1(B d, B x) {
  return path_lines(path_rel(nfn_objU(d), x));
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
  path_wChars(inc(p), s);
  decG(s);
  return p;
}
static NFnDesc* importDesc;



B import_c2(B d, B w, B x) {
  return bqn_execFile(path_rel(nfn_objU(d), x), w);
}

// defined in fns.c
i32 getPrevImport(B path);
void setPrevImport(B path, i32 pos);

static B importKeyList; // exists for GC roots as the hashmap doesn't
static B importValList;
B import_c1(B d, B x) {
  if (importKeyList.u==0) {
    importKeyList = emptyHVec();
    importValList = emptyHVec();
  }
  B path = path_abs(path_rel(nfn_objU(d), x));
  
  i32 prevIdx = getPrevImport(path);
  if (prevIdx>=0) {
    // print_fmt("cached: %R @ %i/%i\n", path, prevIdx, a(importKeyList)->ia);
    dec(path);
    return IGet(importValList, prevIdx);
  }
  if (prevIdx==-2) thrF("•Import: cyclic import of \"%R\"", path);
  B r = bqn_execFile(inc(path), emptySVec());
  
  i32 prevLen = a(importKeyList)->ia;
  // print_fmt("caching: %R @ %i\n", path, prevLen);
  setPrevImport(path, prevLen);
  importKeyList = vec_add(importKeyList, path);
  importValList = vec_add(importValList, inc(r));
  return r;
}
static void sys_gcFn() {
  mm_visit(importKeyList);
  mm_visit(importValList);
  mm_visit(thrownMsg);
  #if CATCH_ERRORS
  mm_visit(lastErrMsg);
  #endif
}


static NFnDesc* fTypeDesc;
static NFnDesc* fExistsDesc;
static NFnDesc* fListDesc;
static NFnDesc* fMapBytesDesc;
B list_c1(B d, B x) {
  return path_list(path_rel(nfn_objU(d), x));
}

B ftype_c1(B d, B x) {
  char ty = path_type(path_rel(nfn_objU(d), x));
  if (ty==0) thrM("•file.Type: Error while accessing file");
  return m_c32(ty);
}
B fexists_c1(B d, B x) {
  char ty = path_type(path_rel(nfn_objU(d), x));
  return m_f64(ty!=0);
}

B fName_c1(B t, B x) {
  if (!isArr(x) || rnk(x)!=1) thrM("•file.Name: Argument must be a character vector");
  return path_name(x);
}

B mapBytes_c1(B d, B x) {
  return mmap_file(path_rel(nfn_objU(d), x));
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
  decG(x);
  TFREE(chrs);
  return r;
}

extern char** environ;

#if __has_include(<spawn.h>) && __has_include(<sys/wait.h>) && !WASM
#include <spawn.h>
#include <fcntl.h>
#include <sys/wait.h>
typedef struct pollfd pollfd;
void shClose(int fd) { if (close(fd)) err("bad file descriptor close"); }

// #define shDbg printf
#define shDbg(...) 

B sh_c2(B t, B w, B x) {
  
  // parse options
  B inObj = bi_N;
  if (!q_N(w)) {
    if (!isNsp(w)) thrM("•SH: 𝕨 must be a namespace");
    inObj = ns_getC(w, "stdin");
    if (!q_N(inObj) && !isArr(inObj)) thrM("•SH: Invalid stdin value");
  }
  u64 iLen = q_N(inObj)? 0 : utf8lenB(inObj);
  
  // allocate args
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
  
  // create pipes
  int p_in[2];
  int p_out[2];
  int p_err[2];
  if (pipe(p_in) || pipe(p_out) || pipe(p_err)) thrM("•SH: Failed to create process: Couldn't create pipes"); // TODO these pipes will easily leak
  shDbg("pipes: %d %d %d %d %d %d\n", p_in[0], p_in[1], p_out[0], p_out[1], p_err[0], p_err[1]);
  fcntl(p_in[1], F_SETFL, O_NONBLOCK); // make our side of pipes never block because we're working on multiple
  fcntl(p_out[0], F_SETFL, O_NONBLOCK);
  fcntl(p_err[0], F_SETFL, O_NONBLOCK);
  
  posix_spawn_file_actions_t a; posix_spawn_file_actions_init(&a);
  // bind the other ends of pipes to the ones in the new process, and close the originals afterwards
  posix_spawn_file_actions_adddup2(&a, p_in [0],  STDIN_FILENO); posix_spawn_file_actions_addclose(&a, p_in [0]); posix_spawn_file_actions_addclose(&a, p_in [1]);
  posix_spawn_file_actions_adddup2(&a, p_out[1], STDOUT_FILENO); posix_spawn_file_actions_addclose(&a, p_out[0]); posix_spawn_file_actions_addclose(&a, p_out[1]);
  posix_spawn_file_actions_adddup2(&a, p_err[1], STDERR_FILENO); posix_spawn_file_actions_addclose(&a, p_err[0]); posix_spawn_file_actions_addclose(&a, p_err[1]);
  
  // spawn the actual process
  pid_t pid;
  if(posix_spawnp(&pid, argv[0], &a, NULL, argv, environ) != 0) thrF("•SH: Failed to create process: %S", strerror(errno));
  posix_spawn_file_actions_destroy(&a); // used now
  shClose(p_in[0]); // close the useless pipes on this side
  shClose(p_out[1]);
  shClose(p_err[1]);
  
  // free args
  for (u64 i = 0; i < xia; i++) TFREE(argv[i]);
  TFREE(argv);
  
  // allocate stdin
  u64 iOff = 0;
  TALLOC(char, iBuf, iLen);
  if (iLen>0) toUTF8(inObj, iBuf);
  bool iDone = false;
  // allocate output buffer
  B s_out = emptyCVec();
  B s_err = emptyCVec();
  
  // polling mess
  const u64 bufsz = 1024;
  TALLOC(char, oBuf, bufsz);
  
  pollfd ps[3];
  i32 plen = 0;
  i32 err_i, out_i, in_i;
  ps[err_i = plen++] = (pollfd){.fd=p_err[0], .events=POLLIN};
  ps[out_i = plen++] = (pollfd){.fd=p_out[0], .events=POLLIN};
  ps[in_i  = plen++] = (pollfd){.fd=p_in[1], .events=POLLOUT};
  
  while (poll(&ps[0], plen - iDone, -1) > 0) {
    shDbg("next poll; revents: out:%d err:%d in:%d\n", ps[0].revents, ps[1].revents, ps[2].revents);
    if (ps[out_i].revents & POLLIN) while(true) { i64 len = read(p_out[0], &oBuf[0], bufsz); shDbg("read stdout "N64d"\n",len); if(len<=0)break; s_out = vec_join(s_out, fromUTF8(oBuf, len)); }
    if (ps[err_i].revents & POLLIN) while(true) { i64 len = read(p_err[0], &oBuf[0], bufsz); shDbg("read stderr "N64d"\n",len); if(len<=0)break; s_err = vec_join(s_err, fromUTF8(oBuf, len)); }
     if (!iDone && ps[in_i].revents & POLLOUT) {
      shDbg("writing "N64u"\n", iLen-iOff);
      ssize_t ww = write(p_in[1], iBuf+iOff, iLen-iOff);
      shDbg("written %zd/"N64u"\n", ww, iLen-iOff);
      if (ww >= 0) {
        iOff+= ww;
        if (iOff==iLen) { iDone=true; shClose(p_in[1]); TFREE(iBuf); shDbg("writing done\n"); }
      }
    }
    if (ps[out_i].revents & POLLHUP  &&  ps[err_i].revents & POLLHUP) { shDbg("HUP end\n"); break; }
  }
  
  // free output buffer
  TFREE(oBuf);
  // free our ends of pipes
  if (!iDone) { shClose(p_in[1]); TFREE(iBuf); shDbg("only got to write "N64u"/"N64u"\n", iOff, iLen); }
  shClose(p_out[0]);
  shClose(p_err[0]);
  
  int status;
  waitpid(pid, &status, 0);
  
  dec(w); dec(x);
  return m_hVec3(m_i32(WEXITSTATUS(status)), s_out, s_err);
}
#else
B sh_c2(B t, B w, B x) { thrM("•SH: CBQN was compiled without <spawn.h>"); }
#endif
B sh_c1(B t, B x) { return sh_c2(t, bi_N, x); }



#if __has_include(<termios.h>)
#include<termios.h>
#include <fcntl.h>
B tRawMode_c1(B t, B x) {
  struct termios term;
  tcgetattr(STDIN_FILENO, &term);
  if (o2b(x)) term.c_lflag&= ~(ICANON | ECHO);
  else term.c_lflag|= ICANON | ECHO;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);
  return x;
}
B tCharN_c1(B t, B x) {
  dec(x);
  fcntl(0, F_SETFL, O_NONBLOCK);
  int n = fgetc(stdin);
  fcntl(0, F_SETFL, 0);
  return n>=0? m_c32(n) : m_f64(0);
}
#else
B tRawMode_c1(B t, B x) { thrM("•term.RawMode not available"); }
B tCharN_c1(B t, B x) { thrM("•term.CharN not available"); }
#endif

B tCharB_c1(B t, B x) {
  dec(x);
  int n = fgetc(stdin);
  return n>=0? m_c32(n) : m_f64(0);
}
B tFlush_c1(B t, B x) {
  fflush(stdout);
  fflush(stderr);
  return x;
}
B tOutRaw_c1(B t, B x) {
  file_wBytes(stdout, bi_N, x);
  return x;
}
B tErrRaw_c1(B t, B x) {
  file_wBytes(stderr, bi_N, x);
  return x;
}

static B termNS;
B getTermNS() {
  if (termNS.u == 0) {
    #define F(X) inc(bi_##X),
    Body* d = m_nnsDesc("flush", "rawmode", "charb", "charn", "outraw", "errraw");
    termNS =  m_nns(d,F(tFlush)F(tRawMode)F(tCharB)F(tCharN)F(tOutRaw)F(tErrRaw));
    #undef F
    gc_add(termNS);
  }
  return inc(termNS);
}



typedef struct CastType { usz s; bool c; } CastType;
static bool isCharType(u8 t) {
  return t==t_c8arr   || t==t_c16arr   || t==t_c32arr
      || t==t_c8slice || t==t_c16slice || t==t_c32slice;
}
static CastType getCastType(B e, B v) {
  usz s; bool c;
  if (isNum(e)) {
    s = o2s(e);
    c = q_N(v) ? 0 : isCharType(v(v)->type);
  } else {
    if (!isArr(e) || rnk(e)!=1 || a(e)->ia!=2) thrM("•bit._cast: 𝕗 elements must be numbers or two-element lists");
    SGetU(e)
    s = o2s(GetU(e,0));
    u32 t = o2c(GetU(e,1));
    c = t=='c';
    if (t=='n'); // generic number
    else if (c     ) { if (s<8||s>32) thrM("•bit._cast: unsupported character width"); }
    else if (t=='i') { if (s<8||s>32) thrM("•bit._cast: unsupported integer width"); }
    else if (t=='u') { if (     s>32) thrM("•bit._cast: unsupported integer width"); }
    else if (t=='f') { if (s!=64) thrM("•bit._cast: type f only supports width 64"); }
    else thrM("•bit._cast: type descriptor in 𝕗 must be one of \"iufnc\"");
  }
  return (CastType) { s, c };
}
static B convert(CastType t, B x) {
  switch (t.s) {
    case  1: return taga(toBitArr(x));
    case  8: return t.c ? toC8Any (x) : toI8Any (x);
    case 16: return t.c ? toC16Any(x) : toI16Any(x);
    case 32: return t.c ? toC32Any(x) : toI32Any(x);
    case 64: return toF64Any(x);
    default: thrM("•bit._cast: unsupported input width");
  }
}
static TyArr* copy(CastType t, B x) {
  switch (t.s) {
    case  1: return cpyBitArr(x);
    case  8: return t.c ? cpyC8Arr (x) : cpyI8Arr (x);
    case 16: return t.c ? cpyC16Arr(x) : cpyI16Arr(x);
    case 32: return t.c ? cpyC32Arr(x) : cpyI32Arr(x);
    case 64: return cpyF64Arr(x);
    default: thrM("•bit._cast: unsupported input width");
  }
}
static u8 typeOfCast(CastType t) {
  switch (t.s) {
    case  1: return t_bitarr;
    case  8: return t.c ? t_c8arr  : t_i8arr ;
    case 16: return t.c ? t_c16arr : t_i16arr;
    case 32: return t.c ? t_c32arr : t_i32arr;
    case 64: return t_f64arr;
    default: thrM("•bit._cast: unsupported result width");
  }
}
B bitcast_impl(B el0, B el1, B x) {
  ur xr;
  if (!isArr(x) || (xr=rnk(x))<1) thrM("•bit._cast: 𝕩 must have rank at least 1");
  
  CastType xt = getCastType(el0, x);
  CastType zt = getCastType(el1, bi_N);
  usz* sh = a(x)->sh;
  usz s=xt.s*sh[xr-1], zl=s/zt.s;
  if (zl*zt.s != s) thrM("•bit._cast: incompatible lengths");
  // Convert to input type
  B r = convert(xt, x);
  u8 rt = typeOfCast(zt);
  if (rt==t_bitarr && (v(r)->refc!=1 || IS_SLICE(v(r)->type))) {
    r = taga(copy(xt, r));
  } else if (v(r)->refc!=1) {
    B pr = r;
    r = taga(TI(r,slice)(r, 0, a(r)->ia));
    arr_shSetI(a(r), xr, shObj(pr)); // safe to use pr because r has refcount>1 and slice only consumes one, leaving some behind
  }
  // Cast to output type
  v(r)->type = IS_SLICE(v(r)->type) ? TO_SLICE(rt) : rt;
  // Adjust shape
  if (xr<=1) {
    Arr* a = a(r);
    a->ia = zl;
    a->sh = &a->ia;
  } else {
    if (shObj(r)->refc>1) {
      shObj(r)->refc--; // won't go to zero as refc>1; preparation for being overwritten by new shape
      usz* zsh = arr_shAlloc(a(r), xr);
      memcpy(zsh, sh, (xr-1)*sizeof(usz));
      sh = zsh;
    }
    sh[xr-1]=zl;
    usz ia=zl; for (usz i=0;i<xr-1;i++)ia*=sh[i]; a(r)->ia=ia;
  }
  return r;
}

B bitcast_c1(Md1D* d, B x) { B f = d->f;
  if (!isArr(f) || rnk(f)!=1 || a(f)->ia!=2) thrM("•bit._cast: 𝕗 must be a 2-element list (from‿to)");
  SGetU(f)
  return bitcast_impl(GetU(f,0), GetU(f,1), x);
}

B bitcast_im(Md1D* d, B x) { B f = d->f;
  if (!isArr(f) || rnk(f)!=1 || a(f)->ia!=2) thrM("•bit._cast: 𝕗 must be a 2-element list (from‿to)");
  SGetU(f)
  return bitcast_impl(GetU(f,1), GetU(f,0), x);
}
static B bitNS;
B getBitNS() {
  if (bitNS.u == 0) {
    #define F(X) inc(bi_bit##X),
    Body* d = m_nnsDesc("cast");
    bitNS = m_nns(d,   F(cast));
    #undef F
    gc_add(bitNS);
  }
  return inc(bitNS);
}

B getInternalNS(void);
B getMathNS(void);
B getPrimitives(void);

static Body* file_nsGen;
B sys_c1(B t, B x) {
  assert(isArr(x));
  M_HARR(r, a(x)->ia) SGetU(x)
  B fileNS = m_f64(0);
  B path = m_f64(0);
  B name = m_f64(0);
  B wdpath = m_f64(0);
  #define REQ_PATH ({ if(!path.u) path = q_N(comp_currPath)? bi_N : path_abs(path_dir(inc(comp_currPath))); path; })
  #define REQ_NAME ({ if(!name.u) name = path_name(inc(comp_currPath)); name; })
  for (usz i = 0; i < a(x)->ia; i++) {
    B c = GetU(x,i);
    B cr;
    if (eqStr(c, U"out")) cr = incG(bi_out);
    else if (eqStr(c, U"show")) cr = incG(bi_show);
    else if (eqStr(c, U"exit")) cr = incG(bi_exit);
    else if (eqStr(c, U"getline")) cr = incG(bi_getLine);
    else if (eqStr(c, U"file")) {
      if(!fileNS.u) {
        REQ_PATH;
        #define F(X) m_nfn(X##Desc, inc(path))
        fileNS = m_nns(file_nsGen, q_N(path)? m_c32(0) : inc(path), F(fileAt), F(fList), F(fBytes), F(fChars), F(fLines), F(fType), F(fExists), inc(bi_fName), F(fMapBytes));
        #undef F
      }
      cr = inc(fileNS);
    }
    else if (eqStr(c, U"wdpath")) {
      if (!wdpath.u) wdpath = path_abs(inc(cdPath));
      cr = inc(wdpath);
    }
    else if (eqStr(c, U"term")) cr = getTermNS();
    else if (eqStr(c, U"internal")) cr = getInternalNS();
    else if (eqStr(c, U"math")) cr = getMathNS();
    else if (eqStr(c, U"bit")) cr = getBitNS();
    else if (eqStr(c, U"type")) cr = incG(bi_type);
    else if (eqStr(c, U"sh")) cr = incG(bi_sh);
    else if (eqStr(c, U"decompose")) cr = incG(bi_decp);
    else if (eqStr(c, U"while")) cr = incG(bi_while);
    else if (eqStr(c, U"primind")) cr = incG(bi_primInd);
    else if (eqStr(c, U"bqn")) cr = incG(bi_bqn);
    else if (eqStr(c, U"cmp")) cr = incG(bi_cmp);
    else if (eqStr(c, U"unixtime")) cr = incG(bi_unixTime);
    else if (eqStr(c, U"monotime")) cr = incG(bi_monoTime);
    else if (eqStr(c, U"timed")) cr = incG(bi_timed);
    else if (eqStr(c, U"delay")) cr = incG(bi_delay);
    else if (eqStr(c, U"hash")) cr = incG(bi_hash);
    else if (eqStr(c, U"repr")) cr = incG(bi_repr);
    else if (eqStr(c, U"fmt")) cr = incG(bi_fmt);
    else if (eqStr(c, U"glyph")) cr = incG(bi_glyph);
    else if (eqStr(c, U"makerand")) cr = incG(bi_makeRand);
    else if (eqStr(c, U"rand")) cr = getRandNS();
    else if (eqStr(c, U"rebqn")) cr = incG(bi_reBQN);
    else if (eqStr(c, U"primitives")) cr = getPrimitives();
    else if (eqStr(c, U"fromutf8")) cr = incG(bi_fromUtf8);
    else if (eqStr(c, U"path")) cr = inc(REQ_PATH);
    else if (eqStr(c, U"name")) cr = inc(REQ_NAME);
    else if (eqStr(c, U"fchars")) cr = m_nfn(fCharsDesc, inc(REQ_PATH));
    else if (eqStr(c, U"fbytes")) cr = m_nfn(fBytesDesc, inc(REQ_PATH));
    else if (eqStr(c, U"flines")) cr = m_nfn(fLinesDesc, inc(REQ_PATH));
    else if (eqStr(c, U"import")) cr = m_nfn(importDesc, inc(REQ_PATH));
    else if (eqStr(c, U"currenterror")) cr = inc(bi_currentError);
    else if (eqStr(c, U"state")) {
      if (q_N(comp_currArgs)) thrM("No arguments present for •state");
      cr = m_hVec3(inc(REQ_PATH), inc(REQ_NAME), inc(comp_currArgs));
    } else if (eqStr(c, U"args")) {
      if (q_N(comp_currArgs)) thrM("No arguments present for •args");
      cr = inc(comp_currArgs);
    } else { dec(x); thrF("Unknown system function •%R", c); }
    HARR_ADD(r, i, cr);
  }
  #undef REQ_PATH
  dec(fileNS);
  dec(path);
  dec(name);
  dec(wdpath);
  return HARR_FCD(r, x);
}

B cdPath;
void sysfn_init() {
  #if CATCH_ERRORS
  lastErrMsg = bi_N;
  #endif
  cdPath = m_str8(1, "."); gc_add(cdPath); gc_addFn(sys_gcFn);
  fCharsDesc = registerNFn(m_str8l("(file).Chars"), fchars_c1, fchars_c2);
  fileAtDesc = registerNFn(m_str8l("(file).At"), fileAt_c1, fileAt_c2);
  fLinesDesc = registerNFn(m_str8l("(file).Lines"), flines_c1, flines_c2);
  fBytesDesc = registerNFn(m_str8l("(file).Bytes"), fbytes_c1, fbytes_c2);
  fListDesc  = registerNFn(m_str8l("(file).List"), list_c1, c2_bad);
  fTypeDesc  = registerNFn(m_str8l("(file).Type"), ftype_c1, c2_bad);
  fMapBytesDesc = registerNFn(m_str8l("(file).MapBytes"), mapBytes_c1, c2_bad);
  fExistsDesc = registerNFn(m_str8l("(file).Exists"), fexists_c1, c2_bad);
  importDesc = registerNFn(m_str32(U"•Import"), import_c1, import_c2);
  reBQNDesc = registerNFn(m_str8l("(REPL)"), repl_c1, repl_c2);
}
void sysfnPost_init() {
  file_nsGen = m_nnsDesc("path","at","list","bytes","chars","lines","type","exists","name","mmap");
  c(BMd1,bi_bitcast)->im = bitcast_im;
}
