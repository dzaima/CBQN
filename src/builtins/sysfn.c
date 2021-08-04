#include "../core.h"
#include "../utils/hash.h"
#include "../utils/file.h"
#include "../utils/wyhash.h"
#include "../utils/builtins.h"
#include "../ns.h"
#include "../nfns.h"

B type_c1(B t, B x) {
  i32 r = -1;
       if (isArr(x)) r = 0;
  else if (isI32(x)) r = 1;
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
  if (!isVal(x)) return m_v2(m_i32(-1), x);
  if (v(x)->flags) return m_v2(m_i32(0), x);
  return TI(x,decompose)(x);
}

B primInd_c1(B t, B x) {
  if (!isVal(x)) return m_i32(rtLen);
  if (v(x)->flags) { B r = m_i32(v(x)->flags-1); dec(x); return r; }
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
  u8 fl = v(x)->flags;
  if (fl==0 || fl>rtLen) {
    u8 ty = v(x)->type;
    if (ty==t_funBI) { B r = fromUTF8l(format_pf (c(Fun,x)->extra)); dec(x); return r; }
    if (ty==t_md1BI) { B r = fromUTF8l(format_pm1(c(Md1,x)->extra)); dec(x); return r; }
    if (ty==t_md2BI) { B r = fromUTF8l(format_pm2(c(Md2,x)->extra)); dec(x); return r; }
    if (ty==t_nfn) { B r = nfn_name(x); dec(x); return r; }
    if (ty==t_fun_block) { dec(x); return m_str8l("(function block)"); }
    if (ty==t_md1_block) { dec(x); return m_str8l("(1-modifier block)"); }
    if (ty==t_md2_block) { dec(x); return m_str8l("(2-modifier block)"); }
    if (ty==t_ns) return nsFmt(x);
    return m_str32(U"(•Glyph: given object with unexpected type)");
  }
  dec(x);
  return m_c32(U"+-×÷⋆√⌊⌈|¬∧∨<>≠=≤≥≡≢⊣⊢⥊∾≍↑↓↕«»⌽⍉/⍋⍒⊏⊑⊐⊒∊⍷⊔!˙˜˘¨⌜⁼´˝`∘○⊸⟜⌾⊘◶⎉⚇⍟⎊"[fl-1]);
}

B repr_c1(B t, B x) {
  #define BL 100
  if (isF64(x)) {
    char buf[BL];
    snprintf(buf, BL, "%.14g", x.f);
    return m_str8(strlen(buf), buf);
  } else {
    #if FORMATTER
      return bqn_repr(x);
    #else
      thrM("•Repr: Cannot represent non-numbers");
    #endif
  }
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
  BS2B xgetU = TI(x,getU);
  for (usz i = 0; i < ia; i++) {
    i64 c = o2i64u(xgetU(x, i));
    if (c>ria) ria = c;
  }
  if (ria>USZ_MAX-1) thrOOM();
  ria++;
  i32* rp; B r = m_i32arrv(&rp, ria);
  for (usz i = 0; i < ria; i++) rp[i] = 0;
  for (usz i = 0; i < ia; i++) {
    i64 n = o2i64u(xgetU(x, i));
    if (n>=0) rp[n]++;
    assert(n>=-1);
  }
  dec(x);
  return r;
}
B grLen_c1(B t,      B x) { return grLen_both(         -1, x); } // assumes valid arguments
B grLen_c2(B t, B w, B x) { return grLen_both(o2i64u(w)-1, x); } // assumes valid arguments

B grOrd_c2(B t, B w, B x) { // assumes valid arguments
  usz wia = a(w)->ia;
  usz xia = a(x)->ia;
  if (wia==0) { dec(w); dec(x); return emptyIVec(); }
  if (xia==0) { dec(w); return x; }
  BS2B wgetU = TI(w,getU);
  BS2B xgetU = TI(x,getU);
  TALLOC(usz, tmp, wia);
  tmp[0] = 0;
  for (usz i = 1; i < wia; i++) tmp[i] = tmp[i-1]+o2su(wgetU(w,i-1));
  usz ria = tmp[wia-1]+o2su(wgetU(w,wia-1));
  i32* rp; B r = m_i32arrv(&rp, ria);
  if (xia>=I32_MAX) thrM("⊔: Too large");
  for (usz i = 0; i < xia; i++) {
    i64 c = o2i64(xgetU(x,i));
    if (c>=0) rp[tmp[c]++] = i;
  }
  dec(w); dec(x); TFREE(tmp);
  return r;
}

B asrt_c1(B t, B x) {
  if (isI32(x) && 1==(i32)x.u) return x;
  if (isF64(x) && 1==x.f) return x;
  dec(x);
  thrM("Assertion error");
}
B asrt_c2(B t, B w, B x) {
  if (isI32(x) && 1==(u32)x.u) { dec(w); return x; }
  if (isF64(x) && 1==x.f) { dec(w); return x; }
  dec(x);
  thr(w);
}
B casrt_c1(B t, B x) {
  if (isI32(x) && 1==(i32)x.u) return x;
  if (isF64(x) && 1==x.f) return x;
  unwindCompiler();
  dec(x);
  thrM("Compilation error");
}
B casrt_c2(B t, B w, B x) {
  if (isI32(x) && 1==(u32)x.u) { dec(w); return x; }
  if (isF64(x) && 1==x.f) { dec(w); return x; }
  unwindCompiler();
  dec(x);
  if (isArr(w) && a(w)->ia==2) {
    B w0 = TI(w,getU)(w,0);
    if (!isArr(w0) || a(w0)->ia<2) goto base;
    B s = TI(w,get)(w,1);
    BS2B w0getU = TI(w0,getU);
    AFMT("\n");
    s = vm_fmtPoint(comp_currSrc, s, comp_currPath, o2s(w0getU(w0,0)), o2s(w0getU(w0,1))+1);
    dec(w);
    thr(s);
  }
  base:
  thr(w);
}

B sys_c1(B t, B x);
B  out_c1(B t, B x) { printRaw(x); putchar('\n'); return x; }
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

B bqn_c1(B t, B x) {
  if (isAtm(x) || rnk(x)!=1) thrM("•BQN: Argument must be a character vector");
  if (a(x)->type!=t_c32arr && a(x)->type!=t_c32slice) {
    usz ia = a(x)->ia;
    BS2B xgetU = TI(x,getU);
    for (usz i = 0; i < ia; i++) if (!isC32(xgetU(x,i))) thrM("•BQN: Argument must be a character vector");
  }
  return bqn_exec(x, bi_N, bi_N);
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
  usz am = o2s(w);
  i64 max = o2i64(x);
  RAND_START;
  B r;
  if (max<1) {
    if (max!=0) thrM("(rand).Range: 𝕩 cannot be negative");
    f64* rp; r = m_f64arrv(&rp, am);
    for (usz i = 0; i < am; i++) rp[i] = wy2u01(wyrand(&seed));
  } else if (max > I32_MAX) {
    if (max >= 1LL<<53) thrM("(rand).Range: 𝕩 must be less than 2⋆53");
    f64* rp; r = m_f64arrv(&rp, am);
    for (usz i = 0; i < am; i++) rp[i] = wy2u0k(wyrand(&seed), max);
  } else {
    i32* rp; r = m_i32arrv(&rp, am);
    for (usz i = 0; i < am; i++) rp[i] = wy2u0k(wyrand(&seed), max);
  }
  RAND_END;
  return r;
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
  rand_rangeName  = m_str32(U"range");  gc_add(rand_rangeName);  rand_rangeDesc  = registerNFn(m_str32(U"(rand).Range"), rand_range_c1, rand_range_c2);
  rand_dealName   = m_str32(U"deal");   gc_add(rand_dealName);   rand_dealDesc   = registerNFn(m_str32(U"(rand).Deal"),   rand_deal_c1, rand_deal_c2);
  rand_subsetName = m_str32(U"subset"); gc_add(rand_subsetName); rand_subsetDesc = registerNFn(m_str32(U"(rand).Subset"),   c1_invalid, rand_subset_c2);
  B tmp = c2(rand_ns, m_f64(0), m_f64(0));
  rand_a = ns_pos(tmp, m_str32(U"a"));
  rand_b = ns_pos(tmp, m_str32(U"b"));
  dec(tmp);
}
B makeRand_c1(B t, B x) {
  if (!isNum(x)) thrM("•MakeRand: 𝕩 must be a number");
  if (rand_ns.u==0) rand_init();
  B r = c2(rand_ns, b(x.u>>32), b(x.u&0xFFFFFFFF));
  ns_set(r, rand_rangeName,   m_nfn(rand_rangeDesc,   inc(r)));
  ns_set(r, rand_dealName,    m_nfn(rand_dealDesc,    inc(r)));
  ns_set(r, rand_subsetName,  m_nfn(rand_subsetDesc,  inc(r)));
  return r;
}

static NFnDesc* fCharsDesc;
B fchars_c1(B d, B x) {
  return file_chars(path_resolve(nfn_objU(d), x));
}
B fchars_c2(B d, B w, B x) {
  file_wChars(path_resolve(nfn_objU(d), w), x);
  return x;
}
static NFnDesc* fBytesDesc;
B fbytes_c1(B d, B x) {
  TmpFile* tf = file_bytes(path_resolve(nfn_objU(d), x));
  usz ia = tf->ia; u8* p = (u8*)tf->a;
  u32* rp; B r = m_c32arrv(&rp, ia);
  for (i64 i = 0; i < ia; i++) rp[i] = p[i];
  ptr_dec(tf);
  return r;
}
static NFnDesc* fLinesDesc;
B flines_c1(B d, B x) {
  return file_lines(path_resolve(nfn_objU(d), x));
}
static NFnDesc* importDesc;
B import_c1(B d,      B x) { return bqn_execFile(path_resolve(nfn_objU(d), x), emptySVec()); }
B import_c2(B d, B w, B x) { return bqn_execFile(path_resolve(nfn_objU(d), x), w); }
static NFnDesc* listDesc;
B list_c1(B d, B x) {
  return file_list(path_resolve(nfn_objU(d), x));
}

B delay_c1(B t, B x) {
  f64 sf = o2f(x);
  if (sf<0 || sf>1ULL<<63) thrF("•Delay: Bad argument: %f", sf);
  struct timespec ts;
  u64 s = (u64)sf;
  ts.tv_sec = (u64)sf;
  ts.tv_nsec = (u64)((sf-s)*1e9);
  nanosleep(&ts, &ts);
  return x; // TODO figure out how to return an actually correct thing
}
B exit_c1(B t, B x) {
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

B getInternalNS(void);
B getMathNS(void);

static B file_nsGen;
B sys_c1(B t, B x) {
  assert(isArr(x));
  usz i = 0;
  HArr_p r = m_harrs(a(x)->ia, &i);
  BS2B xgetU = TI(x,getU);
  B fileNS = m_f64(0);
  for (; i < a(x)->ia; i++) {
    B c = xgetU(x,i);
    if (eqStr(c, U"out")) r.a[i] = inc(bi_out);
    else if (eqStr(c, U"show")) r.a[i] = inc(bi_show);
    else if (eqStr(c, U"exit")) r.a[i] = inc(bi_exit);
    else if (eqStr(c, U"getline")) r.a[i] = inc(bi_getLine);
    else if (eqStr(c, U"file")) {
      if(fileNS.u==m_f64(0).u) {
        #define F(X) m_nfn(X##Desc, path_dir(inc(comp_currPath))),
        B arg =    m_caB(4, (B[]){F(list)F(fBytes)F(fChars)F(fLines)});
        #undef F
        fileNS = c1(file_nsGen,arg);
      }
      r.a[i] = inc(fileNS);
    }
    else if (eqStr(c, U"internal")) r.a[i] = getInternalNS();
    else if (eqStr(c, U"math")) r.a[i] = getMathNS();
    else if (eqStr(c, U"type")) r.a[i] = inc(bi_type);
    else if (eqStr(c, U"decompose")) r.a[i] = inc(bi_decp);
    else if (eqStr(c, U"primind")) r.a[i] = inc(bi_primInd);
    else if (eqStr(c, U"bqn")) r.a[i] = inc(bi_bqn);
    else if (eqStr(c, U"cmp")) r.a[i] = inc(bi_cmp);
    else if (eqStr(c, U"timed")) r.a[i] = inc(bi_timed);
    else if (eqStr(c, U"delay")) r.a[i] = inc(bi_delay);
    else if (eqStr(c, U"hash")) r.a[i] = inc(bi_hash);
    else if (eqStr(c, U"repr")) r.a[i] = inc(bi_repr);
    else if (eqStr(c, U"glyph")) r.a[i] = inc(bi_glyph);
    else if (eqStr(c, U"makerand")) r.a[i] = inc(bi_makeRand);
    else if (eqStr(c, U"fchars")) r.a[i] = m_nfn(fCharsDesc, path_dir(inc(comp_currPath)));
    else if (eqStr(c, U"fbytes")) r.a[i] = m_nfn(fBytesDesc, path_dir(inc(comp_currPath)));
    else if (eqStr(c, U"flines")) r.a[i] = m_nfn(fLinesDesc, path_dir(inc(comp_currPath)));
    else if (eqStr(c, U"import")) r.a[i] = m_nfn(importDesc, path_dir(inc(comp_currPath)));
    else if (eqStr(c, U"args")) {
      if(isNothing(comp_currArgs)) thrM("No arguments present for •args");
      r.a[i] = inc(comp_currArgs);
    } else { dec(x); thrF("Unknown system function •%R", c); }
  }
  dec(fileNS);
  return harr_fcd(r, x);
}

void sysfn_init() {
  fCharsDesc = registerNFn(m_str32(U"•FChars"), fchars_c1, fchars_c2);
  fLinesDesc = registerNFn(m_str32(U"•FLines"), flines_c1, c2_invalid);
  fBytesDesc = registerNFn(m_str32(U"•FBytes"), fbytes_c1, c2_invalid);
  importDesc = registerNFn(m_str32(U"•Import"), import_c1, import_c2);
  listDesc = registerNFn(m_str32(U"•file.List"), list_c1, c2_invalid);
}
void sysfnPost_init() {
  file_nsGen = bqn_exec(m_str32(U"{⟨List,   Bytes,   Chars,   Lines⟩⇐𝕩}"), emptyCVec(), emptySVec()); gc_add(file_nsGen);
}
