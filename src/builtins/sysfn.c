#include "../core.h"
#include "../utils/hash.h"
#include "../utils/calls.h"
#include "../utils/file.h"
#include "../utils/wyhash.h"
#include "../utils/time.h"
#include "../builtins.h"
#include "../ns.h"
#include "../nfns.h"

#include <unistd.h>
#if !defined(_WIN32) && !defined(_WIN64)
  #include <poll.h>
#else
  #include "../windows/getline.c"
#endif
#include <errno.h>

static bool eqStr(B w, u32* x) {
  if (isAtm(w) || RNK(w)!=1) return false;
  SGetU(w)
  u64 i = 0;
  while (x[i]) {
    B c = GetU(w, i);
    if (!isC32(c) || x[i]!=(u32)c.u) return false;
    i++;
  }
  return i==IA(w);
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
  if (!isVal(x)) return m_c32vec_0(U"(•Glyph: not given a function)");
  #ifdef RT_WRAP
    x = rtWrap_unwrap(x);
  #endif
  if (isPrim(x)) {
    B r = m_c32(U"+-×÷⋆√⌊⌈|¬∧∨<>≠=≤≥≡≢⊣⊢⥊∾≍⋈↑↓↕«»⌽⍉/⍋⍒⊏⊑⊐⊒∊⍷⊔!˙˜˘¨⌜⁼´˝`∘○⊸⟜⌾⊘◶⎉⚇⍟⎊"[v(x)->flags-1]);
    decG(x);
    return r;
  }
  u8 ty = TY(x);
  if (ty==t_funBI) { B r = utf8Decode0(pfn_repr(c(Fun,x)->extra)); decG(x); return r; }
  if (ty==t_md1BI) { B r = utf8Decode0(pm1_repr(c(Md1,x)->extra)); decG(x); return r; }
  if (ty==t_md2BI) { B r = utf8Decode0(pm2_repr(c(Md2,x)->extra)); decG(x); return r; }
  if (ty==t_nfn) { B r = nfn_name(x); decG(x); return r; }
  if (ty==t_funBl) { decG(x); return m_c8vec_0("(function block)"); }
  if (ty==t_md1Bl) { decG(x); return m_c8vec_0("(1-modifier block)"); }
  if (ty==t_md2Bl) { decG(x); return m_c8vec_0("(2-modifier block)"); }
  if (ty==t_ns) return nsFmt(x);
  return m_c32vec_0(U"(•Glyph: given object with unexpected type)");
}

#if !NO_RYU
B ryu_d2s(double f);
bool ryu_s2d_n(u8* buffer, int len, f64* result);
#endif

B repr_c1(B t, B x) {
  if (isF64(x)) {
    #if NO_RYU
      NUM_FMT_BUF(buf, x.f);
      return utf8Decode(buf, strlen(buf));
    #else
      return ryu_d2s(o2fG(x));
    #endif
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

#if NO_RYU
B parseFloat_c1(B t, B x) { thrM("•ParseFloat: Not supported with Ryu disabled"); }
#else
B parseFloat_c1(B t, B x) {
  if (isAtm(x)) thrM("•ParseFloat: Expected a character list argument");
  if (TI(x,elType)!=el_c8) {
    x = chr_squeeze(x);
    if (TI(x,elType)!=el_c8) thrM("•ParseFloat: Expected a character list argument"); 
  }
  usz ia = IA(x);
  if (RNK(x)!=1) thrM("•ParseFloat: Input must have rank 1");
  if (ia==0) thrM("•ParseFloat: Input was empty");
  if (ia >= (1<<20)) thrM("•ParseFloat: Input too long"); // otherwise things like 
  u8* data = c8any_ptr(x);
  f64 res;
  if (!ryu_s2d_n(data, ia, &res)) thrM("•ParseFloat: Malformed input");
  decG(x);
  return m_f64(res);
}
#endif

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
  usz ia = IA(x);
  SGetU(x)
  for (usz i = 0; i < ia; i++) {
    i64 c = o2i64G(GetU(x, i));
    if (c>ria) ria = c;
  }
  if (ria > (i64)(USZ_MAX-1)) thrOOM();
  ria++;
  B r;
  {
    u64* rp; r = m_bitarrv(&rp, ria);
    for (usz i = 0; i < BIT_N(ria); i++) rp[i] = 0;
    for (usz i = 0; i < ia; i++) {
      i64 n = o2i64G(GetU(x, i)); assert(n>=-1);
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
    i64 n = o2i64G(GetU(x, i)); assert(n>=-1);
    if (n>=0) rp[n]++;
  }
  r_r: decG(x); return r;
}
B grLen_c1(B t,      B x) { return grLen_both(         -1, x); } // assumes valid arguments
B grLen_c2(B t, B w, B x) { return grLen_both(o2i64G(w)-1, x); } // assumes valid arguments

B grOrd_c2(B t, B w, B x) { // assumes valid arguments
  usz wia = IA(w);
  usz xia = IA(x);
  if (wia==0) { decG(w); decG(x); return emptyIVec(); }
  if (xia==0) { decG(w); return x; }
  SGetU(w)
  SGetU(x)
  TALLOC(usz, tmp, wia);
  tmp[0] = 0;
  for (usz i = 1; i < wia; i++) tmp[i] = tmp[i-1]+o2sG(GetU(w,i-1));
  usz ria = tmp[wia-1]+o2sG(GetU(w,wia-1));
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
  if (LIKELY(isF64(x) && o2fG(x)==1)) return x;
  if (isF64(x)) thrM("Assertion error");
  thr(x);
}
B asrt_c2(B t, B w, B x) {
  if (LIKELY(isF64(x) && o2fG(x)==1)) { dec(w); return x; }
  dec(x);
  thr(w);
}
B casrt_c2(B t, B w, B x) {
  if (LIKELY(isF64(x) && o2fG(x)==1)) { dec(w); return x; }
  unwindCompiler();
  dec(x);
  if (isArr(w) && IA(w)==2) {
    B w0 = IGetU(w,0);
    if (isNum(w0)) {
      B s = IGet(w,1);
      AFMT("\n");
      usz pos = o2s(w0);
      s = vm_fmtPoint(comp_currSrc, s, comp_currPath, pos, pos+1);
      dec(w);
      thr(s);
    }
    if (isArr(w0) && RNK(w0)==1 && IA(w0)>=1) {
      B s = IGet(w,1); AFMT("\n");
      usz pos = o2s(IGetU(w0,0));
      s = vm_fmtPoint(comp_currSrc, s, comp_currPath, pos, pos+1);
      dec(w);
      thr(s);
    }
    if (isArr(w0) && RNK(w0)==2 && IA(w0)>=2) {
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
  if (LIKELY(isF64(x) && o2fG(x)==1)) return x;
  casrt_c2(t, inc(x), x); UD;
}

B sys_c1(B t, B x);
B out_c1(B t, B x) {
  if (isArr(x) && RNK(x)>1) thrF("•Out: Argument cannot have rank %i", RNK(x));
  printRaw(x); putchar('\n');
  return x;
}
B show_c1(B t, B x) {
  #ifndef NO_SHOW
    #if FORMATTER
      B fmt = bqn_fmt(inc(x));
      printRaw(fmt); dec(fmt);
    #else
      print(x);
    #endif
    putchar('\n');
  #endif
  return x;
}

B vfyStr(B x, char* name, char* arg) {
  if (isAtm(x) || RNK(x)!=1) thrF("%U: %U must be a character vector", name, arg);
  if (!elChr(TI(x,elType))) {
    usz ia = IA(x);
    SGetU(x)
    for (usz i = 0; i < ia; i++) if (!isC32(GetU(x,i))) thrF("%U: %U must be a character vector", name, arg);
  }
  return x;
}

B cdPath;
static B args_path(B* fullpath, B w, char* name) { // consumes w, returns args, writes to fullpath
  if (!isArr(w) || RNK(w)!=1 || IA(w)>3) thrF("%U: 𝕨 must be a vector with at most 3 items, but had shape %H", name, w);
  usz ia = IA(w);
  SGet(w)
  B path = ia>0? vfyStr(Get(w,0),name,"path"    ) : inc(cdPath);
  B file = ia>1? vfyStr(Get(w,1),name,"filename") : emptyCVec();
  B args = ia>2?        Get(w,2)                  : emptySVec();
  *fullpath = vec_join(vec_addN(path, m_c32('/')), file);
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
    if (RNK(w) > 1) thrM("(rand).Range: 𝕨 must be a valid shape");
    SGetU(w);
    usz wia = IA(w);
    for (u64 i = 0; i < wia; i++) mulOn(am, o2s(GetU(w, i)));
  } else {
    am = o2s(w);
  }
  
  RAND_START;
  Arr* r;
  if (max<1) {
    if (max!=0) thrM("(rand).Range: 𝕩 cannot be negative");
    f64* rp; r = m_f64arrp(&rp, am);
    PLAINLOOP for (usz i = 0; i < am; i++) rp[i] = wy2u01(wyrand(&seed));
  } else if (max > (1ULL<<31)) {
    if (max >= 1LL<<53) thrM("(rand).Range: 𝕩 must be less than 2⋆53");
    f64* rp; r = m_f64arrp(&rp, am);
    PLAINLOOP for (usz i = 0; i < am; i++) rp[i] = wy2u0k(wyrand(&seed), max);
  } else {
    u8 t; usz u64am;
    
    if (max>128) {
      if (max>32768)   { u64am = (am+ 1)>>1; t=t_i32arr; }
      else             { u64am = (am+ 3)>>2; t=t_i16arr; }
    } else if (max!=2) { u64am = (am+ 7)>>3; t=t_i8arr;  }
    else               { u64am = (am+63)>>6; t=t_bitarr; }
    
    u64 aExact = t==t_bitarr? BIT_N(am)<<3 : am<<arrTypeWidthLog(t);
    u64 aFilled = u64am<<3;
    assert(aFilled >= aExact);
    r = m_arr(offsetof(TyArr,a) + aFilled, t, am);
    void* rp = ((TyArr*)r)->a;
    if (max & (max-1)) { // not power of two
      if      (t==t_i32arr) PLAINLOOP for (usz i = 0; i < am; i++) ((i32*)rp)[i] = wy2u0k(wyrand(&seed), max);
      else if (t==t_i16arr) PLAINLOOP for (usz i = 0; i < am; i++) ((i16*)rp)[i] = wy2u0k(wyrand(&seed), max);
      else if (t==t_i8arr)  PLAINLOOP for (usz i = 0; i < am; i++) (( i8*)rp)[i] = wy2u0k(wyrand(&seed), max);
      else UD; // bitarr will be max==2, i.e. a power of two
    } else {
      u64 mask;
      if (t==t_bitarr) { mask = ~0L; goto end; }
      mask = max-1;
      mask|= mask<<32;
      if (t==t_i32arr) { goto end; } mask|= mask<<16;
      if (t==t_i16arr) { goto end; } mask|= mask<<8;
      
      end:;
      PLAINLOOP for (usz i = 0; i < u64am; i++) ((u64*)rp)[i] = wyrand(&seed) & mask;
    }
    FINISH_OVERALLOC(r, offsetof(TyArr,a) + aExact, offsetof(TyArr,a) + aFilled);
  }

  RAND_END;
  if (isArr(w)) {
    usz wia = IA(w);
    switch (wia) {
      case 0: { arr_shAlloc(r, 0); break; }
      case 1: { arr_shVec(r); break; }
      default: {
        usz* sh = arr_shAlloc(r, wia);
        SGetU(w);
        for (usz i = 0; i < wia; i++) sh[i] = o2sG(GetU(w, i));
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

B ud_c1(B t, B x);
B slash_c1(B t, B x);
extern void filter_ne_i32(i32* dst, i32* src, usz len, usz sum, i32 val); // slash.c

B rand_subset_c2(B t, B w, B x) {
  i32 wi = o2i(w);
  i32 xi = o2i(x);
  if (RARE(wi<0)) thrM("(rand).Subset: 𝕨 cannot be negative");
  if (RARE(xi<0)) thrM("(rand).Subset: 𝕩 cannot be negative");
  if (RARE(wi>xi)) thrM("(rand).Subset: 𝕨 cannot exceed 𝕩");
  if (wi==0) return emptyIVec();
  if (wi==xi) return ud_c1(t, x); // Only one complete subset; will hang without this
  
  B r;
  RAND_START;
  if (wi > xi/8) {
    // Bit set (as bytes)
    i8* set; B s = m_i8arrv(&set, xi);
    bool invert = wi > xi/2;
    i32 wn = invert ? xi-wi : wi;
    for (i64 i = 0; i < xi; i++) set[i] = 0;
    for (i32 i = xi-wn; i < xi; i++) {
      i32 j = wy2u0k(wyrand(&seed), i+1);
      if (set[j]) j=i;
      set[j] = 1;
    }
    s = taga(cpyBitArr(s));
    if (invert) s = bit_negate(s);
    return slash_c1(t, s);
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
    filter_ne_i32(rp, hash, sz, wi, xi);
    TFREE(hash);
    r = xi<=128? taga(cpyI8Arr(r)) : xi<=32768? taga(cpyI16Arr(r)) : r;
  }
  RAND_END;
  return r;
}

#if USE_VALGRIND
u64 vgRandSeed;
u64 vgRand64Range(u64 range) {
  return wy2u0k(wyrand(&vgRandSeed), range);
}
u64 vgRand64() {
  return wyrand(&vgRandSeed);
}
#endif

static NOINLINE void rand_init() {
  #if USE_VALGRIND
    vgRandSeed = nsTime();
  #endif
  rand_ns = m_nnsDesc("seed1", "seed2", "range", "deal", "subset");
  NSDesc* d = rand_ns->nsDesc;
  d->expGIDs[0] = d->expGIDs[1] = -1;
  rand_rangeName  = m_c8vec_0("range");  gc_add(rand_rangeName);  rand_rangeDesc  = registerNFn(m_c8vec_0("(rand).Range"), rand_range_c1, rand_range_c2);
  rand_dealName   = m_c8vec_0("deal");   gc_add(rand_dealName);   rand_dealDesc   = registerNFn(m_c8vec_0("(rand).Deal"),   rand_deal_c1, rand_deal_c2);
  rand_subsetName = m_c8vec_0("subset"); gc_add(rand_subsetName); rand_subsetDesc = registerNFn(m_c8vec_0("(rand).Subset"),       c1_bad, rand_subset_c2);
}
B makeRand_c1(B t, B x) {
  if (!isNum(x)) thrM("•MakeRand: 𝕩 must be a number");
  if (rand_ns==NULL) rand_init();
  B r = m_nns(rand_ns, b(x.u>>32), b(x.u&0xFFFFFFFF), m_nfn(rand_rangeDesc, bi_N), m_nfn(rand_dealDesc, bi_N), m_nfn(rand_subsetDesc, bi_N));
  Scope* sc = c(NS,r)->sc;
  for (i32 i = 2; i < 5; i++) nfn_swapObj(sc->vars[i], incG(r));
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
  B sys = ns_getC(x, "system");
  i32 replVal = q_N(repl) || eqStr(repl,U"none")? 0 : eqStr(repl,U"strict")? 1 : eqStr(repl,U"loose")? 2 : 3;
  if (replVal==3) thrM("•ReBQN: Invalid repl value");
  Block* initBlock = bqn_comp(m_c8vec_0("\"(REPL initializer)\""), inc(cdPath), m_f64(0));
  B scVal;
  if (replVal==0) {
    scVal = bi_N;
  } else {
    Scope* sc = m_scope(initBlock->bodies[0], NULL, 0, 0, NULL);
    scVal = tag(sc,OBJ_TAG);
  }
  ptr_dec(initBlock);
  HArr_p d = m_harrUv(7); d.a[0] = m_f64(replVal); d.a[1] = scVal;
  for (usz i=2; i<7; i++) d.a[i] = bi_N;
  init_comp(d.a+2, prim, sys);
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
  path_wChars(incG(p), x);
  dec(x);
  return p;
}
static NFnDesc* fBytesDesc;
B fbytes_c1(B d, B x) {
  I8Arr* tf = path_bytes(path_rel(nfn_objU(d), x));
  usz ia = PIA(tf);
  u8* rp; B r = m_c8arrv(&rp, ia);
  COPY_TO(rp, el_i8, 0, taga(tf), 0, ia);
  ptr_dec(tf);
  return r;
}
B fbytes_c2(B d, B w, B x) {
  if (!isArr(x)) thrM("•FBytes: Non-array 𝕩");
  B p = path_rel(nfn_objU(d), w);
  path_wBytes(incG(p), x);
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
  usz ia = IA(x);
  SGet(x)
  for (u64 i = 0; i < ia; i++) {
    nl = Get(x, i);
    if (!isArr(nl)) thrM("•FLines: Non-array element of 𝕩");
    s = vec_join(s, nl);
    //if (windows) s = vec_add(s, m_c32('\r')); TODO figure out whether or not this is a thing that should be done
    s = vec_addN(s, m_c32('\n'));
  }
  dec(x);
  B p = path_rel(nfn_objU(d), w);
  path_wChars(incG(p), s);
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
void clearImportCacheMap(void);

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
    // print_fmt("cached: %R @ %i/%i\n", path, prevIdx, IA(importKeyList));
    dec(path);
    return IGet(importValList, prevIdx);
  }
  if (prevIdx==-2) thrF("•Import: cyclic import of \"%R\"", path);
  if (CATCH) {
    setPrevImport(path, -1);
    rethrow();
  }
  
  i32 prevLen = IA(importValList);
  importKeyList = vec_addN(importKeyList, path);
  importValList = vec_addN(importValList, bi_N);
  
  B r = bqn_execFile(incG(path), emptySVec());
  
  harr_ptr(importValList)[prevLen] = inc(r);
  setPrevImport(path, prevLen);
  popCatch();
  
  return r;
}
void clearImportCache() {
  if (importKeyList.u!=0) {
    dec(importKeyList); importKeyList = m_f64(0);
    dec(importValList); importValList = m_f64(0);
  }
  clearImportCacheMap();
}


static NFnDesc* fTypeDesc;
static NFnDesc* fCreatedDesc;
static NFnDesc* fAccessedDesc;
static NFnDesc* fModifiedDesc;
static NFnDesc* fSizeDesc;
static NFnDesc* fExistsDesc;
static NFnDesc* fListDesc;
static NFnDesc* fMapBytesDesc;
static NFnDesc* createdirDesc;
static NFnDesc* renameDesc;
static NFnDesc* removeDesc;

B list_c1(B d, B x) {
  return path_list(path_rel(nfn_objU(d), x));
}
B createdir_c1(B d, B x) {
  B p = path_rel(nfn_objU(d), x);
  if (dir_create(p)) return p;
  thrM("(file).CreateDir: Failed to create directory");
}

B rename_c2(B d, B w, B x) {
  d = nfn_objU(d);
  B p = path_rel(d, w);
  if (path_rename(path_rel(d, x), p)) return p;
  thrM("(file).Rename: Failed to rename file");
}

B remove_c1(B d, B x) {
  if (path_remove(path_rel(nfn_objU(d), x))) return m_i32(1);
  thrM("(file).Remove: Failed to remove file");
}

B ftype_c1(B d, B x) {
  char ty = path_type(path_rel(nfn_objU(d), x));
  if (ty==0) thrM("•file.Type: Error while accessing file");
  return m_c32(ty);
}

B fcreated_c1 (B d, B x) { return path_info(path_rel(nfn_objU(d), x), 0); }
B faccessed_c1(B d, B x) { return path_info(path_rel(nfn_objU(d), x), 1); }
B fmodified_c1(B d, B x) { return path_info(path_rel(nfn_objU(d), x), 2); }
B fsize_c1    (B d, B x) { return path_info(path_rel(nfn_objU(d), x), 3); }

B fexists_c1(B d, B x) {
  char ty = path_type(path_rel(nfn_objU(d), x));
  return m_f64(ty!=0);
}

B fName_c1(B t, B x) {
  if (!isArr(x) || RNK(x)!=1) thrM("•file.Name: Argument must be a character vector");
  return path_name(x);
}
B fParent_c1(B t, B x) {
  if (!isArr(x) || RNK(x)!=1) thrM("•file.Parent: Argument must be a character vector");
  return path_parent(x);
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
  B r = utf8Decode(ln, strlen(ln)-1);
  free(ln);
  return r;
}

B fromUtf8_c1(B t, B x) {
  if (!isArr(x)) thrM("•FromUTF8: Argument must be a character or number array");
  usz ia = IA(x);
  TALLOC(char, chrs, ia);
  SGetU(x)
  for (u64 i = 0; i < ia; i++) {
    B c = GetU(x,i);
    if (isC32(c)) {
      u32 v = o2cG(c);
      if (v>=256) thrF("•FromUTF8: Argument contained a character with codepoint %i", v);
      chrs[i] = v;
    } else {
      i32 v = o2i(c);
      if (v<=-128 | v>=256) thrF("•FromUTF8: Argument contained %i", v);
      chrs[i] = v&0xff;
    }
  }
  B r = utf8Decode(chrs, ia);
  decG(x);
  TFREE(chrs);
  return r;
}

B toUtf8_c1(B t, B x) {
  if (!isArr(x)) thrM("•ToUTF8: Argument must be a character or number array");
  u64 len = utf8lenB(x);
  u8* rp; B r = m_c8arrv(&rp, len);
  toUTF8(x, (char*)rp);
  dec(x);
  return r;
}

extern char** environ;

#if __has_include(<spawn.h>) && __has_include(<sys/wait.h>) && !WASM
#define HAS_SH 1
#include <spawn.h>
#include <fcntl.h>
#include <sys/wait.h>
typedef struct pollfd pollfd;
void shClose(int fd) { if (close(fd)) err("bad file descriptor close"); }

// #define shDbg(...) printf(__VA_ARGS__); fflush(stdout)
#define shDbg(...)

B sh_c2(B t, B w, B x) {
  
  // parse options
  B inObj = bi_N;
  bool raw = false;
  if (!q_N(w)) {
    if (!isNsp(w)) thrM("•SH: 𝕨 must be a namespace");
    inObj = ns_getC(w, "stdin");
    if (!q_N(inObj) && !isArr(inObj)) thrM("•SH: Invalid stdin value");
    B rawObj = ns_getC(w, "raw");
    if (!q_N(rawObj)) raw = o2b(rawObj);
  }
  u64 iLen = q_N(inObj)? 0 : (raw? IA(inObj) : utf8lenB(inObj));
  
  // allocate args
  if (isAtm(x) || RNK(x)>1) thrM("•SH: 𝕩 must be a vector of strings");
  usz xia = IA(x);
  if (xia==0) thrM("•SH: 𝕩 must have at least one item");
  TALLOC(char*, argv, xia+1);
  SGetU(x)
  for (u64 i = 0; i < xia; i++) {
    B c = GetU(x, i);
    if (isAtm(c) || RNK(c)!=1) thrM("•SH: 𝕩 must be a vector of strings");
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
  char* iBuf;
  CharBuf iBufRaw;
  if (iLen>0) {
    if (raw) {
      iBufRaw = get_chars(inObj);
      iBuf = iBufRaw.data;
    } else {
      iBuf = TALLOCP(char, iLen);
      toUTF8(inObj, iBuf);
    }
  } else iBuf = NULL;
  #define FREE_INPUT do { if (iLen>0) { if (raw) free_chars(iBufRaw); else TFREE(iBuf); } } while(0)
  
  bool iDone = false;
  // allocate output buffer
  B s_out = emptyCVec();
  B s_err = emptyCVec();
  
  // polling mess
  const u64 bufsz = 1024;
  u8* oBuf;
  B oBufObj = m_c8arrv(&oBuf, bufsz);
  usz* oBufIA = &a(oBufObj)->ia;
  
  pollfd ps[3];
  i32 plen = 0;
  i32 err_i, out_i, in_i;
  ps[err_i = plen++] = (pollfd){.fd=p_err[0], .events=POLLIN};
  ps[out_i = plen++] = (pollfd){.fd=p_out[0], .events=POLLIN};
  ps[in_i  = plen++] = (pollfd){.fd=p_in[1], .events=POLLOUT};
  
  while (poll(&ps[0], plen - iDone, -1) > 0) {
    shDbg("next poll; revents: out:%d err:%d in:%d\n", ps[0].revents, ps[1].revents, ps[2].revents);
    bool any = false;
    if (ps[out_i].revents & POLLIN) while(true) { i64 len = read(p_out[0], &oBuf[0], bufsz); shDbg("read stdout "N64d"\n",len); if(len<=0) break; else any=true; *oBufIA = len; s_out = vec_join(s_out, incG(oBufObj)); }
    if (ps[err_i].revents & POLLIN) while(true) { i64 len = read(p_err[0], &oBuf[0], bufsz); shDbg("read stderr "N64d"\n",len); if(len<=0) break; else any=true; *oBufIA = len; s_err = vec_join(s_err, incG(oBufObj)); }
     if (!iDone && ps[in_i].revents & POLLOUT) {
      shDbg("writing "N64u"\n", iLen-iOff);
      ssize_t ww = write(p_in[1], iBuf+iOff, iLen-iOff);
      shDbg("written %zd/"N64u"\n", ww, iLen-iOff);
      if (ww >= 0) {
        iOff+= ww;
        if (iOff==iLen) { iDone=true; shClose(p_in[1]); FREE_INPUT; shDbg("writing done\n"); }
        any = true;
      }
    }
    #ifdef __CYGWIN__
    if (!any) { shDbg("no events despite poll returning; end\n"); break; }
    #else
    (void)any;
    #endif
    if (ps[out_i].revents & POLLHUP  &&  ps[err_i].revents & POLLHUP) { shDbg("HUP end\n"); break; }
  }
  
  // free output buffer
  assert(reusable(oBufObj));
  #if VERIFY_TAIL
  *oBufIA = bufsz;
  #endif
  mm_free(v(oBufObj));
  // free our ends of pipes
  if (!iDone) { shClose(p_in[1]); FREE_INPUT; shDbg("only got to write "N64u"/"N64u"\n", iOff, iLen); }
  shClose(p_out[0]);
  shClose(p_err[0]);
  
  int status;
  waitpid(pid, &status, 0);
  
  dec(w); dec(x);
  B s_outRaw = toC8Any(s_out);
  B s_errRaw = toC8Any(s_err);
  B s_outObj;
  B s_errObj;
  if (raw) {
    s_outObj = s_outRaw;
    s_errObj = s_errRaw;
  } else {
    s_outObj = utf8Decode((char*)c8any_ptr(s_outRaw), IA(s_outRaw)); dec(s_outRaw);
    s_errObj = utf8Decode((char*)c8any_ptr(s_errRaw), IA(s_errRaw)); dec(s_errRaw);
  }
  return m_hVec3(m_i32(WEXITSTATUS(status)), s_outObj, s_errObj);
}
#else
#define HAS_SH 0
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
  if (isAtm(x)) thrM("•term.OutRaw: 𝕩 must be an array");
  file_wBytes(stdout, bi_N, x);
  return x;
}
B tErrRaw_c1(B t, B x) {
  if (isAtm(x)) thrM("•term.ErrRaw: 𝕩 must be an array");
  file_wBytes(stderr, bi_N, x);
  return x;
}

static B termNS;
B getTermNS() {
  if (termNS.u == 0) {
    #define F(X) incG(bi_##X),
    Body* d = m_nnsDesc("flush", "rawmode", "charb", "charn", "outraw", "errraw");
    termNS =  m_nns(d,F(tFlush)F(tRawMode)F(tCharB)F(tCharN)F(tOutRaw)F(tErrRaw));
    #undef F
    gc_add(termNS);
  }
  return incG(termNS);
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
    c = q_N(v) ? 0 : isCharType(TY(v));
  } else {
    if (!isArr(e) || RNK(e)!=1 || IA(e)!=2) thrM("•bit._cast: 𝕗 elements must be numbers or two-element lists");
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
static B set_bit_result(B r, u8 rt, ur rr, usz rl, usz *sh) {
  // Cast to output type
  v(r)->type = IS_SLICE(v(r)->type) ? TO_SLICE(rt) : rt;
  // Adjust shape
  Arr* a = a(r);
  if (rr<=1) {
    a->ia = rl;
    a->sh = &a->ia;
  } else {
    if (shObj(r)->refc>1) {
      shObj(r)->refc--; // won't go to zero as refc>1; preparation for being overwritten by new shape
      usz* rsh = a->sh = m_shArr(rr)->a;
      shcpy(rsh, sh, rr-1);
      sh = rsh;
      SPRNK(a, rr);
    }
    sh[rr-1] = rl;
    a->ia = rl * shProd(sh, 0, rr-1);
  }
  return r;
}

B bitcast_impl(B el0, B el1, B x) {
  ur xr;
  if (!isArr(x) || (xr=RNK(x))<1) thrM("•bit._cast: 𝕩 must have rank at least 1");
  
  CastType xct = getCastType(el0, x);
  CastType rct = getCastType(el1, bi_N);
  usz* sh = SH(x);
  u64 s=xct.s*(u64)sh[xr-1], rl=s/rct.s;
  if (rl*rct.s != s) thrM("•bit._cast: incompatible lengths");
  if (rl>=USZ_MAX) thrM("•bit._cast: output too large");
  B r = convert(xct, x);
  u8 rt = typeOfCast(rct);
  if (rt==t_bitarr && (v(r)->refc!=1 || IS_SLICE(TY(r)))) {
    r = taga(copy(xct, r));
  } else if (v(r)->refc!=1) {
    B pr = r;
    Arr* r2 = TI(r,slice)(r, 0, IA(r));
    r = taga(arr_shSetI(r2, xr, shObj(pr))); // safe to use pr because r has refcount>1 and slice only consumes one, leaving some behind
  } else {
    #if VERIFY_TAIL
      if (xct.s==1 && rct.s!=1) {
        FINISH_OVERALLOC(a(r), offsetof(TyArr,a)+IA(r)/8, offsetof(TyArr,a) + (BIT_N(IA(r))<<3));
      }
    #endif
  }
  return set_bit_result(r, rt, xr, rl, sh);
}

B bitcast_c1(Md1D* d, B x) { B f = d->f;
  if (!isArr(f) || RNK(f)!=1 || IA(f)!=2) thrM("•bit._cast: 𝕗 must be a 2-element list (from‿to)");
  SGetU(f)
  return bitcast_impl(GetU(f,0), GetU(f,1), x);
}
B bitcast_im(Md1D* d, B x) { B f = d->f;
  if (!isArr(f) || RNK(f)!=1 || IA(f)!=2) thrM("•bit._cast: 𝕗 must be a 2-element list (from‿to)");
  SGetU(f)
  return bitcast_impl(GetU(f,1), GetU(f,0), x);
}

static usz req2(usz s, char* name) {
  usz top = 1ull << (8*sizeof(usz)-1); // Prevent 0 from passing
  if ((top|s) & (s-1)) thrF("•bit._%U: sizes in 𝕗 must be powers of 2 (contained %s)", name, s);
  return s;
}

enum BitOp1 { op_not, op_neg };
B bitop1(B f, B x, enum BitOp1 op, char* name) {
  usz ow, rw, xw; // Operation width, result width, x width
  if (isAtm(f)) {
    ow = rw = xw = req2(o2s(f), name);
  } else {
    if (RNK(f)>1) thrF("•bit._%U: 𝕗 must have rank at most 1 (%i≡≠𝕗)", name, RNK(f));
    usz ia = IA(f);
    if (ia<1 || ia>3) thrF("•bit._%U: 𝕗 must contain between 1 and 3 numbers (%s≡≠𝕗)", name, ia);
    SGetU(f)
    usz t[3];
    for (usz i=0 ; i<ia; i++) t[i] = req2(o2s(GetU(f, i)), name);
    for (usz i=ia; i<3 ; i++) t[i] = t[ia-1];
    ow = t[0]; rw = t[1]; xw = t[2];
  }
  
  ur xr;
  if (!isArr(x) || (xr=RNK(x))<1) thrF("•bit._%U: 𝕩 must have rank at least 1", name);
  
  usz* sh = SH(x);
  usz rws = CTZ(rw);
  usz xws = CTZ(xw);
  u64 n = IA(x) << xws;
  u64 s = (u64)sh[xr-1] << xws;
  u64 rl = s >> rws;
  if ((s & (ow-1)) || (rl<<rws != s)) thrF("•bit._%U: incompatible lengths", name);
  if (rl>=USZ_MAX) thrF("•bit._%U: output too large", name);
  
  x = convert((CastType){ xw, isCharType(TY(x)) }, x);
  u8 rt = typeOfCast((CastType){ rw, 0 });
  u64* xp = tyany_ptr(x);
  B r; u64* rp;
  if (v(x)->refc!=1 || (rt==t_bitarr && IS_SLICE(TY(x)))) {
    Arr* ra = m_arr(offsetof(TyArr,a) + (n+7)/8, rt, n>>rws);
    arr_shCopyUnchecked(ra, x);
    r = taga(ra); rp = tyany_ptr(r);
  } else {
    r = inc(x); rp = xp;
  }
  switch (op) { default: UD;
    case op_not: {
      usz l = n/64; bit_negatePtr(rp, xp, l);
      usz q = (-n)%64; if (q) rp[l] ^= (~(u64)0 >> q) & (rp[l]^~xp[l]);
    } break;
    case op_neg: switch (ow) {
      default: thrF("•bit._%U: unhandled width %s", name, ow);
      #define CASE(W) case W: \
        NOUNROLL for (usz i=0; i<n/W; i++) ((u##W*)rp)[i] = -((u##W*)xp)[i]; \
        break;
      CASE(8) CASE(16) CASE(32) CASE(64)
      #undef CASE
    } break;
  }
  set_bit_result(r, rt, xr, rl, sh);
  decG(x);
  return r;
}
B bitnot_c1(Md1D* d, B x) { return bitop1(d->f, x, op_not, "not"); }
B bitneg_c1(Md1D* d, B x) { return bitop1(d->f, x, op_neg, "neg"); }

enum BitOp2 { op_and, op_or, op_xor, op_add, op_sub, op_mul };
B bitop2(B f, B w, B x, enum BitOp2 op, char* name) {
  usz ow, rw, xw, ww; // Operation width, result width, x width, w width
  if (isAtm(f)) {
    ow = rw = xw = ww = req2(o2s(f), name);
  } else {
    if (RNK(f)>1) thrF("•bit._%U: 𝕗 must have rank at most 1 (%i≡≠𝕗)", name, RNK(f));
    usz ia = IA(f);
    if (ia<1 || ia>4) thrF("•bit._%U: 𝕗 must contain between 1 and 4 numbers (%s≡≠𝕗)", name, ia);
    SGetU(f)
    usz t[4];
    for (usz i=0 ; i<ia; i++) t[i] = req2(o2s(GetU(f, i)), name);
    for (usz i=ia; i<4 ; i++) t[i] = t[ia-1];
    ow = t[0]; rw = t[1]; xw = t[2]; ww = t[3];
  }
  
  if (isAtm(x)) x = m_atomUnit(x);
  if (isAtm(w)) w = m_atomUnit(w);
  ur wr=RNK(w); usz* wsh = SH(w); u64 s = wr==0? ww : ww*(u64)wsh[wr-1];
  ur xr=RNK(x); usz*  sh = SH(x); u64 t = xr==0? xw : xw*(u64) sh[xr-1];
  bool negw = 0; // Negate 𝕨 to subtract from 𝕩
  bool noextend = wr == xr && s == t;
  if (wr==xr && xr==0) thrF("•bit._%U: some argument must have rank at least 1", name);
  if (noextend) {
    for (usz i=0; i<xr-1; i++) if (sh[i]!=wsh[i]) thrF("•bit._%U: 𝕨 and 𝕩 leading shapes must match", name);
  } else {
    if (wr>1 || s!=ow || xr==0) { // Need to extend 𝕩
      if (xr>1 || t!=ow || wr==0) {
        if (wr!=xr && wr>1 && xr>1) thrF("•bit._%U: 𝕨 and 𝕩 must have equal ranks if more than 1", name);
        thrF("•bit._%U: 𝕨 or 𝕩 1-cell width must equal operation width if extended", name);
      }
      { B t=w; w=x; x=t; }
      { usz t=ww; ww=xw; xw=t; }
      negw=op==op_sub; if (negw) op=op_add;
      t = s; xr = wr; sh = wsh;
    }
  }
  usz rws = CTZ(rw);
  u64 n = IA(x) << CTZ(xw);
  u64 rl = t >> rws;
  if ((t & (ow-1)) || (rl<<rws != t)) thrF("•bit._%U: incompatible lengths", name);
  if (rl>=USZ_MAX) thrF("•bit._%U: output too large", name);
  
  w = convert((CastType){ ww, isCharType(TY(w)) }, w);
  x = convert((CastType){ xw, isCharType(TY(x)) }, x);
  u8 rt = typeOfCast((CastType){ rw, 0 });
  Arr* ra = m_arr(offsetof(TyArr,a) + (n+7)/8, rt, n>>rws);
  arr_shCopyUnchecked(ra, x);
  B r = taga(ra);
  u64* wp = tyany_ptr(w);
  u64* xp = tyany_ptr(x);
  u64* rp = tyany_ptr(r);
  
  #define CASES(O,Q,P) case op_##O: \
    switch(ow) { default: thrF("•bit._%U: unhandled width %s", name, ow); \
      CASE(8,Q,P) CASE(16,Q,P) CASE(32,Q,P) CASE(64,Q,P)                  \
    } break;
  #define SWITCH \
    switch (op) { default: UD;                     \
      BINOP(and,&) BINOP(or,|) BINOP(xor,^)        \
      CASES(add,u,+) CASES(sub,u,-) CASES(mul,i,*) \
    }
  if (noextend) {
    #define BINOP(O,P) case op_##O: { \
      usz l = n/64; NOUNROLL for (usz i=0; i<l; i++) rp[i] = wp[i] P xp[i];      \
      usz q = (-n)%64; if (q) rp[l] ^= (~(u64)0 >> q) & (rp[l]^(wp[l] P xp[l])); \
      } break;
    #define CASE(W, Q, P) case W: \
      NOUNROLL for (usz i=0; i<n/W; i++)                  \
        ((Q##W*)rp)[i] = ((Q##W*)wp)[i] P ((Q##W*)xp)[i]; \
      break;
    SWITCH
    #undef BINOP
    #undef CASE
  } else {
    u64 wn; if (negw) { wn=-*wp; wp=&wn; }
    #define BINOP(O,P) case op_##O: { \
      if (ow>64) thrF("•bit._%U: scalar extension with width over 64 unhandled", name); \
      u64 wv = *wp & (~(u64)0>>(64-ow));                                      \
      for (usz tw=ow; tw<64; tw*=2) wv|=wv<<tw;                               \
      usz l = n/64; NOUNROLL for (usz i=0; i<l; i++) rp[i] = wv P xp[i];      \
      usz q = (-n)%64; if (q) rp[l] ^= (~(u64)0 >> q) & (rp[l]^(wv P xp[l])); \
      } break;
    #define CASE(W, Q, P) case W: { \
      Q##W wv = *(Q##W*)wp;                   \
      NOUNROLL for (usz i=0; i<n/W; i++)      \
        ((Q##W*)rp)[i] = wv P ((Q##W*)xp)[i]; \
      } break;
    SWITCH
    #undef BINOP
    #undef CASE
  }
  #undef CASES
  #undef SWITCH
  set_bit_result(r, rt, xr, rl, sh);
  decG(w); decG(x);
  return r;
}
#define DEF_OP2(OP) \
  B bit##OP##_c2(Md1D* d, B w, B x) { return bitop2(d->f, w, x, op_##OP, #OP); }
DEF_OP2(and) DEF_OP2(or) DEF_OP2(xor)
DEF_OP2(add) DEF_OP2(sub) DEF_OP2(mul)
#undef DEF_OP2

static B bitNS;
B getBitNS() {
  if (bitNS.u == 0) {
    #define F(X) incG(bi_bit##X),
    Body* d = m_nnsDesc("cast","not","neg","and","or","xor","add","sub","mul");
    bitNS = m_nns(d,   F(cast)F(not)F(neg)F(and)F(or)F(xor)F(add)F(sub)F(mul));
    #undef F
    gc_add(bitNS);
  }
  return incG(bitNS);
}

B getInternalNS(void);
B getMathNS(void);
B getPrimitives(void);
void getSysvals(B* res);

static Body* file_nsGen;

#if FFI || FOR_BUILD
#define FFIOPT 1
#else
#define FFIOPT 0
#endif

#define OPTSYS_0(X)
#define OPTSYS_1(X) X
#define OPTSYS_B(COND) OPTSYS_##COND
#define OPTSYS(COND) OPTSYS_B(COND)

#define FOR_DEFAULT_SYSVALS(F) \
  F("out", U"•Out", bi_out) \
  F("show", U"•Show", bi_show) \
  F("exit", U"•Exit", bi_exit) \
  F("getline", U"•GetLine", bi_getLine) \
  F("type", U"•Type", bi_type) \
  OPTSYS(HAS_SH)(F("sh", U"•SH", bi_sh)) \
  F("decompose", U"•Decompose", bi_decp) \
  F("while", U"•_while_", bi_while) \
  F("bqn", U"•BQN", bi_bqn) \
  F("cmp", U"•Cmp", bi_cmp) \
  F("unixtime", U"•UnixTime", bi_unixTime) \
  F("monotime", U"•MonoTime", bi_monoTime) \
  F("timed", U"•_timed", bi_timed) \
  F("delay", U"•Delay", bi_delay) \
  F("hash", U"•Hash", bi_hash) \
  F("repr", U"•Repr", bi_repr) \
  F("parsefloat", U"•ParseFloat", bi_parseFloat) \
  F("fmt", U"•Fmt", bi_fmt) \
  F("glyph", U"•Glyph", bi_glyph) \
  F("makerand", U"•MakeRand", bi_makeRand) \
  F("rebqn", U"•ReBQN", bi_reBQN) \
  F("fromutf8", U"•FromUTF8", bi_fromUtf8) \
  F("toutf8", U"•ToUTF8", bi_toUtf8) \
  F("currenterror", U"•CurrentError", bi_currentError) \
  F("math", U"•math", tag(0,VAR_TAG)) \
  F("rand", U"•rand", tag(1,VAR_TAG)) \
  F("term", U"•term", tag(2,VAR_TAG)) \
  F("bit", U"•bit", tag(3,VAR_TAG)) \
  F("primitives", U"•primitives", tag(4,VAR_TAG)) \
  F("internal", U"•internal", tag(5,VAR_TAG)) \
  F("fchars", U"•FChars", tag(6,VAR_TAG)) \
  F("fbytes", U"•FBytes", tag(7,VAR_TAG)) \
  F("flines", U"•FLines", tag(8,VAR_TAG)) \
  F("import", U"•Import", tag(9,VAR_TAG)) \
  OPTSYS(FFIOPT)(F("ffi", U"•FFI", tag(10,VAR_TAG))) \
  F("name", U"•name", tag(11,VAR_TAG)) \
  F("path", U"•path", tag(12,VAR_TAG)) \
  F("wdpath", U"•wdpath", tag(13,VAR_TAG)) \
  F("file", U"•file", tag(14,VAR_TAG)) \
  F("state", U"•state", tag(15,VAR_TAG)) \
  F("args", U"•args", tag(16,VAR_TAG)) \
  F("listsys", U"•listsys", tag(17,VAR_TAG))

NFnDesc* ffiloadDesc;
B ffiload_c2(B t, B w, B x);
B indexOf_c2(B t, B w, B x);
bool fileInit;

static void initFileNS() {
  if (fileInit) return;
  fileInit = true;
  file_nsGen = m_nnsDesc("path","at","list","bytes","chars","lines","type","created","accessed","modified","size","exists","name","parent","mapbytes","createdir","rename","remove");
  fCharsDesc   = registerNFn(m_c8vec_0("(file).Chars"), fchars_c1, fchars_c2);
  fileAtDesc   = registerNFn(m_c8vec_0("(file).At"), fileAt_c1, fileAt_c2);
  fLinesDesc   = registerNFn(m_c8vec_0("(file).Lines"), flines_c1, flines_c2);
  fBytesDesc   = registerNFn(m_c8vec_0("(file).Bytes"), fbytes_c1, fbytes_c2);
  fListDesc    = registerNFn(m_c8vec_0("(file).List"), list_c1, c2_bad);
  fTypeDesc    = registerNFn(m_c8vec_0("(file).Type"), ftype_c1, c2_bad);
  fCreatedDesc = registerNFn(m_c8vec_0("(file).Created"),  fcreated_c1, c2_bad);
  fModifiedDesc= registerNFn(m_c8vec_0("(file).Modified"), fmodified_c1, c2_bad);
  fAccessedDesc= registerNFn(m_c8vec_0("(file).Accessed"), faccessed_c1, c2_bad);
  fSizeDesc    = registerNFn(m_c8vec_0("(file).Size"), fsize_c1, c2_bad);
  createdirDesc= registerNFn(m_c8vec_0("(file).CreateDir"), createdir_c1, c2_bad);
  renameDesc   = registerNFn(m_c8vec_0("(file).Rename"), c1_bad, rename_c2);
  removeDesc   = registerNFn(m_c8vec_0("(file).Remove"), remove_c1, c2_bad);
  fMapBytesDesc= registerNFn(m_c8vec_0("(file).MapBytes"), mapBytes_c1, c2_bad);
  fExistsDesc  = registerNFn(m_c8vec_0("(file).Exists"), fexists_c1, c2_bad);
  importDesc   = registerNFn(m_c32vec_0(U"•Import"), import_c1, import_c2);
  ffiloadDesc  = registerNFn(m_c32vec_0(U"•FFI"), c1_bad, ffiload_c2);
}


B sys_c1(B t, B x) {
  assert(isArr(x));
  B tmp[2]; getSysvals(tmp);
  B curr_ns = tmp[0];
  B curr_vs = tmp[1]; SGetU(curr_vs)
  B idxs = indexOf_c2(m_f64(0), incG(curr_ns), incG(x)); SGetU(idxs)
  
  B fileNS = m_f64(0);
  B path = m_f64(0);
  B name = m_f64(0);
  B wdpath = m_f64(0);
  #define REQ_PATH ({ if(!path.u) path = q_N(comp_currPath)? bi_N : path_abs(path_parent(inc(comp_currPath))); path; })
  #define REQ_NAME ({ if(!name.u) name = path_name(inc(comp_currPath)); name; })
  
  M_HARR(r, IA(x))
  for (usz i = 0; i < IA(x); i++) {
    i32 ci = o2iG(GetU(idxs,i));
    if (ci>=IA(curr_vs)) thrF("Unknown system function •%R", IGetU(x,i));
    B c = GetU(curr_vs,ci);
    B cr;
    if (!isVar(c)) {
      cr = inc(c);
    } else switch ((u32)c.u) { default: thrM("Bad dynamically-loaded system value");
      case 0: cr = getMathNS(); break; // •math
      case 1: cr = getRandNS(); break; // •rand
      case 2: cr = getTermNS(); break; // •term
      case 3: cr = getBitNS(); break; // •bit
      case 4: cr = getPrimitives(); break; // •primitives
      case 5: cr = getInternalNS(); break; // •internal
      case 6: initFileNS(); cr = m_nfn(fCharsDesc, inc(REQ_PATH)); break; // •FChars
      case 7: initFileNS(); cr = m_nfn(fBytesDesc, inc(REQ_PATH)); break; // •FBytes
      case 8: initFileNS(); cr = m_nfn(fLinesDesc, inc(REQ_PATH)); break; // •FLines
      case 9: initFileNS(); cr = m_nfn(importDesc, inc(REQ_PATH)); break; // •Import
      case 10: initFileNS(); cr = m_nfn(ffiloadDesc, inc(REQ_PATH)); break; // •FFI
      case 11: if (q_N(comp_currPath)) thrM("No path present for •name"); cr = inc(REQ_NAME); break; // •name
      case 12: if (q_N(comp_currPath)) thrM("No path present for •path"); cr = inc(REQ_PATH); break; // •path
      case 13: { // •wdpath
        if (!wdpath.u) wdpath = path_abs(inc(cdPath));
        cr = inc(wdpath);
        break;
      }
      case 14: { // •file
        if(!fileNS.u) {
          initFileNS();
          REQ_PATH;
          #define F(X) m_nfn(X##Desc, inc(path))
          fileNS = m_nns(file_nsGen, q_N(path)? m_c32(0) : inc(path), F(fileAt), F(fList), F(fBytes), F(fChars), F(fLines), F(fType), F(fCreated), F(fAccessed), F(fModified), F(fSize), F(fExists), inc(bi_fName), inc(bi_fParent), F(fMapBytes), F(createdir), F(rename), F(remove));
          #undef F
        }
        cr = incG(fileNS);
        break;
      }
      case 15: { // •state
        if (q_N(comp_currArgs)) thrM("No arguments present for •state");
        if (q_N(comp_currPath)) thrM("No path present for •state");
        cr = m_hVec3(inc(REQ_PATH), inc(REQ_NAME), inc(comp_currArgs));
        break;
      }
      case 16: { // •args
        if (q_N(comp_currArgs)) thrM("No arguments present for •args");
        cr = inc(comp_currArgs);
        break;
      }
      case 17: { // •listsys
        cr = incG(curr_ns);
      }
    }
    HARR_ADD(r, i, cr);
  }
  #undef REQ_PATH
  #undef REQ_NAME
  dec(fileNS);
  dec(path);
  dec(name);
  dec(wdpath);
  decG(idxs);
  return HARR_FCD(r, x);
}


static char* dsv_strs[] = {
  #define F(L,N,B) L,
  FOR_DEFAULT_SYSVALS(F)
  #undef F
};


u32* dsv_text[] = {
  #define F(L,N,B) N,
  FOR_DEFAULT_SYSVALS(F)
  #undef F
  U"•bit._add",U"•bit._and",U"•bit._cast",U"•bit._mul",U"•bit._neg",U"•bit._not",U"•bit._or",U"•bit._sub",U"•bit._xor",
  
  U"•file.Accessed",U"•file.At",U"•file.Bytes",U"•file.Chars",U"•file.Created",U"•file.CreateDir",U"•file.Exists",U"•file.Lines",U"•file.List",
  U"•file.MapBytes",U"•file.Modified",U"•file.Name",U"•file.Parent",U"•file.path",U"•file.Remove",U"•file.Rename",U"•file.Size",U"•file.Type",
  
  U"•internal.ClearRefs",U"•internal.DeepSqueeze",U"•internal.EEqual",U"•internal.ElType",U"•internal.HeapDump",U"•internal.Info",U"•internal.IsPure",U"•internal.ListVariations",U"•internal.Refc",U"•internal.Squeeze",U"•internal.Temp",U"•internal.Type",U"•internal.Unshare",U"•internal.Variation",
  U"•math.Acos",U"•math.Acosh",U"•math.Asin",U"•math.Asinh",U"•math.Atan",U"•math.Atan2",U"•math.Atanh",U"•math.Cbrt",U"•math.Comb",U"•math.Cos",U"•math.Cosh",U"•math.Erf",U"•math.ErfC",U"•math.Expm1",U"•math.Fact",U"•math.GCD",U"•math.Hypot",U"•math.LCM",U"•math.Log10",U"•math.Log1p",U"•math.Log2",U"•math.LogFact",U"•math.Sin",U"•math.Sinh",U"•math.Sum",U"•math.Tan",U"•math.Tanh",
  U"•rand.Deal",U"•rand.Range",U"•rand.Subset",
  U"•term.CharB",U"•term.CharN",U"•term.ErrRaw",U"•term.Flush",U"•term.OutRaw",U"•term.RawMode",
  NULL
};

B dsv_ns, dsv_vs;
void sysfn_init() {
  usz dsv_num = sizeof(dsv_strs)/sizeof(char*);
  usz i = 0;
  HArr_p dsv_ns0 = m_harrUv(dsv_num); dsv_ns=dsv_ns0.b; gc_add(dsv_ns);
  for (usz i = 0; i < dsv_num; i++) dsv_ns0.a[i] = m_c8vec_0(dsv_strs[i]);
  HArr_p dsv_vs0 = m_harrUv(dsv_num); dsv_vs=dsv_vs0.b; gc_add(dsv_vs);
  #define F(L,N,B) dsv_vs0.a[i] = inc(B); i++;
  FOR_DEFAULT_SYSVALS(F)
  #undef F
  
  #if CATCH_ERRORS
  lastErrMsg = bi_N;
  gc_add_ref(&lastErrMsg);
  #endif
  cdPath = m_c8vec(".", 1); gc_add(cdPath);
  
  gc_add_ref(&importKeyList);
  gc_add_ref(&importValList);
  gc_add_ref(&thrownMsg);
  
  reBQNDesc = registerNFn(m_c8vec_0("(REPL)"), repl_c1, repl_c2);
}
void sysfnPost_init() {
  c(BMd1,bi_bitcast)->im = bitcast_im;
}
