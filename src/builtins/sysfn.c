#include "../core.h"
#include "../utils/hash.h"
#include "../utils/file.h"
#include "../utils/wyhash.h"
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
  if (r==-1) { print(x); err(": getting type"); }
  decR(x);
  return m_i32(r);
}

B decp_c1(B t, B x) {
  if (!isVal(x)) return m_v2(m_i32(-1), x);
  if (v(x)->flags) return m_v2(m_i32(0), x);
  return TI(x).decompose(x);
}

B primInd_c1(B t, B x) {
  if (!isVal(x)) return m_i32(rtLen);
  if (v(x)->flags) { B r = m_i32(v(x)->flags-1); dec(x); return r; }
  dec(x);
  return m_i32(rtLen);
}

B glyph_c1(B t, B x) {
  return x;
}

B repr_c1(B t, B x) {
  #define BL 100
  if (isF64(x)) {
    char buf[BL];
    snprintf(buf, BL, "%.14g", x.f);
    return m_str8(strlen(buf), buf);
  } else {
    #ifdef FORMATTER
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
  BS2B xgetU = TI(x).getU;
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
  if (wia==0) { dec(w); dec(x); return inc(bi_emptyIVec); }
  if (xia==0) { dec(w); return x; }
  BS2B wgetU = TI(w).getU;
  BS2B xgetU = TI(x).getU;
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
  thrM("assertion error");
}
B asrt_c2(B t, B w, B x) {
  if (isI32(x) && 1==(u32)x.u) { dec(w); return x; }
  if (isF64(x) && 1==x.f) { dec(w); return x; }
  dec(x);
  thr(w);
}

B sys_c1(B t, B x);
B  out_c1(B t, B x) { printRaw(x); putchar('\n'); return x; }
B show_c1(B t, B x) {
  #ifdef FORMATTER
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
    BS2B xgetU = TI(x).getU;
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
static B rand_rangeName;
static i32 rand_a, rand_b;
static NFnDesc* rand_range;
B rand_range_c1(B t, B x) {
  i64 xv = o2i64(x);
  if (xv<0) thrM("(rand).Range: 𝕩 cannot be negative");
  Scope* sc = c(NS,nfn_objU(t))->sc;
  u64 seed = sc->vars[rand_a].u | sc->vars[rand_b].u<<32;
  u64 rnd = wyrand(&seed);
  sc->vars[rand_a].u = seed>>32;
  sc->vars[rand_a].u = seed&0xFFFFFFFF;
  return xv? m_f64(wy2u0k(rnd, xv)) : m_f64(wy2u01(rnd));
}
B rand_range_c2(B t, B w, B x) {
  Scope* sc = c(NS,nfn_objU(t))->sc;
  u64 seed = sc->vars[rand_a].u | sc->vars[rand_b].u<<32;
  usz am = o2s(w);
  i64 max = o2i64(x);
  B r;
  if (max<1) {
    if (max!=0) thrM("(rand).Range: 𝕩 cannot be negative");
    f64* rp; r = m_f64arrv(&rp, am);
    for (usz i = 0; i < am; i++) rp[i] = wy2u01(wyrand(&seed));
  } else if (max > I32_MAX) {
    if (max >= 1ULL<<53) thrM("(rand).Range: 𝕩 must be less than 2⋆53");
    f64* rp; r = m_f64arrv(&rp, am);
    for (usz i = 0; i < am; i++) rp[i] = wy2u0k(wyrand(&seed), max);
  } else {
    i32* rp; r = m_i32arrv(&rp, am);
    for (usz i = 0; i < am; i++) rp[i] = wy2u0k(wyrand(&seed), max);
  }
  sc->vars[rand_a].u = seed>>32;
  sc->vars[rand_a].u = seed&0xFFFFFFFF;
  return r;
}

static NOINLINE void rand_init() {
  rand_ns = bqn_exec(m_str32(U"{a←𝕨⋄b←𝕩⋄range⇐0}"), inc(bi_emptyHVec), inc(bi_emptyHVec)); gc_add(rand_ns);
  rand_rangeName = m_str32(U"range"); gc_add(rand_rangeName);
  rand_range = registerNFn(m_str32(U"(rand).Range"), rand_range_c1, rand_range_c2);
  B tmp = c1(rand_ns, m_f64(0));
  rand_a = ns_pos(tmp, m_str32(U"a"));
  rand_b = ns_pos(tmp, m_str32(U"b"));
  dec(tmp);
}
B makeRand_c1(B t, B x) {
  if (!isNum(x)) thrM("•MakeRand: 𝕩 must be a number");
  if (rand_ns.u==0) rand_init();
  B r = c2(rand_ns, b(x.u>>32), b(x.u&0xFFFFFFFF));
  ns_set(r, rand_rangeName, m_nfn(rand_range, inc(r)));
  return r;
}



static B makeRel(B md) { // doesn't consume
  return m1_d(inc(md), path_dir(inc(comp_currPath)));
}

B getInternalNS();

B sys_c1(B t, B x) {
  assert(isArr(x));
  usz i = 0;
  HArr_p r = m_harrs(a(x)->ia, &i);
  BS2B xgetU = TI(x).getU;
  for (; i < a(x)->ia; i++) {
    B c = xgetU(x,i);
    if (eqStr(c, U"out")) r.a[i] = inc(bi_out);
    else if (eqStr(c, U"show")) r.a[i] = inc(bi_show);
    else if (eqStr(c, U"internal")) r.a[i] = getInternalNS();
    else if (eqStr(c, U"type")) r.a[i] = inc(bi_type);
    else if (eqStr(c, U"decompose")) r.a[i] = inc(bi_decp);
    else if (eqStr(c, U"primind")) r.a[i] = inc(bi_primInd);
    else if (eqStr(c, U"bqn")) r.a[i] = inc(bi_bqn);
    else if (eqStr(c, U"cmp")) r.a[i] = inc(bi_cmp);
    else if (eqStr(c, U"timed")) r.a[i] = inc(bi_timed);
    else if (eqStr(c, U"hash")) r.a[i] = inc(bi_hash);
    else if (eqStr(c, U"repr")) r.a[i] = inc(bi_repr);
    else if (eqStr(c, U"makerand")) r.a[i] = inc(bi_makeRand);
    else if (eqStr(c, U"fchars")) r.a[i] = makeRel(bi_fchars);
    else if (eqStr(c, U"fbytes")) r.a[i] = makeRel(bi_fbytes);
    else if (eqStr(c, U"flines")) r.a[i] = makeRel(bi_flines);
    else if (eqStr(c, U"import")) r.a[i] = makeRel(bi_import);
    else if (eqStr(c, U"args")) {
      if(isNothing(comp_currArgs)) thrM("No arguments present for •args");
      r.a[i] = inc(comp_currArgs);
    } else { dec(x); thrF("Unknown system function •%R", c); }
  }
  return harr_fcd(r, x);
}
