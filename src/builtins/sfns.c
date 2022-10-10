#include "../core.h"
#include "../utils/each.h"
#include "../utils/mut.h"
#include "../utils/talloc.h"
#include "../builtins.h"

static Arr* take_impl(usz ria, B x) { // consumes x; returns v↑⥊𝕩 without set shape; v is non-negative
  usz xia = IA(x);
  if (ria>xia) {
    B xf = getFillE(x);
    MAKE_MUT(r, ria); mut_init(r, el_or(TI(x,elType), selfElType(xf)));
    MUTG_INIT(r);
    mut_copyG(r, 0, x, 0, xia);
    mut_fillG(r, xia, xf, ria-xia);
    decG(x);
    if (r->fns->elType!=el_B) { dec(xf); return mut_fp(r); }
    return a(withFill(mut_fv(r), xf));
  } else {
    return TI(x,slice)(x,0,ria);
  }
}

B m_vec1(B a) {
  if (isF64(a)) {
    i32 i = (i32)a.f;
    if (RARE(a.f != i))   { f64* rp; B r = m_f64arrv(&rp, 1); rp[0] = a.f; return r; }
    else if (q_ibit(i))   { u64* rp; B r = m_bitarrv(&rp, 1); rp[0] = i; return r; }
    else if (i == (i8 )i) { i8*  rp; B r = m_i8arrv (&rp, 1); rp[0] = i; return r; }
    else if (i == (i16)i) { i16* rp; B r = m_i16arrv(&rp, 1); rp[0] = i; return r; }
    else                  { i32* rp; B r = m_i32arrv(&rp, 1); rp[0] = i; return r; }
  }
  if (isC32(a)) {
    u32 c = o2cG(a);
    if      (LIKELY(c<U8_MAX )) { u8*  rp; B r = m_c8arrv (&rp, 1); rp[0] = c; return r; }
    else if (LIKELY(c<U16_MAX)) { u16* rp; B r = m_c16arrv(&rp, 1); rp[0] = c; return r; }
    else                        { u32* rp; B r = m_c32arrv(&rp, 1); rp[0] = c; return r; }
  }
  Arr* ra = arr_shVec(m_fillarrp(1));
  fillarr_ptr(ra)[0] = a;
  fillarr_setFill(ra, m_f64(0));
  fillarr_setFill(ra, asFill(inc(a)));
  return taga(ra);
}

FORCE_INLINE B m_vec2Base(B a, B b, bool fills) {
  if (isAtm(a)&isAtm(b)) {
    if (LIKELY(isNum(a)&isNum(b))) {
      i32 ai=a.f; i32 bi=b.f;
      if (RARE(ai!=a.f | bi!=b.f))        { f64* rp; B r = m_f64arrv(&rp, 2); rp[0]=o2fG(a); rp[1]=o2fG(b); return r; }
      else if (q_ibit(ai)  &  q_ibit(bi)) { u64* rp; B r = m_bitarrv(&rp, 2); rp[0]=ai | (bi<<1);           return r; }
      else if (ai==(i8 )ai & bi==(i8 )bi) { i8*  rp; B r = m_i8arrv (&rp, 2); rp[0]=ai;      rp[1]=bi;      return r; }
      else if (ai==(i16)ai & bi==(i16)bi) { i16* rp; B r = m_i16arrv(&rp, 2); rp[0]=ai;      rp[1]=bi;      return r; }
      else                                { i32* rp; B r = m_i32arrv(&rp, 2); rp[0]=ai;      rp[1]=bi;      return r; }
    }
    if (isC32(b)&isC32(a)) {
      u32 ac=o2cG(a); u32 bc=o2cG(b);
      if      (ac==(u8 )ac & bc==(u8 )bc) { u8*  rp; B r = m_c8arrv (&rp, 2); rp[0]=ac; rp[1]=bc; return r; }
      else if (ac==(u16)ac & bc==(u16)bc) { u16* rp; B r = m_c16arrv(&rp, 2); rp[0]=ac; rp[1]=bc; return r; }
      else                                { u32* rp; B r = m_c32arrv(&rp, 2); rp[0]=ac; rp[1]=bc; return r; }
    }
  }
  if (fills) {
    if (isAtm(a) || isAtm(b)) goto noFills;
    B af = asFill(incG(a));
    if (noFill(af)) goto noFills;
    B bf = asFill(incG(b));
    if (noFill(bf)) { dec(af); goto noFills; }
    if (!fillEqual(af,bf)) { dec(bf); dec(af); goto noFills; }
    dec(bf);
    Arr* ra = arr_shVec(m_fillarrp(2));
    fillarr_setFill(ra, af);
    fillarr_ptr(ra)[0] = a;
    fillarr_ptr(ra)[1] = b;
    return taga(ra);
  }
  noFills:
  return m_hVec2(a,b);
}

B m_vec2(B a, B b) { return m_vec2Base(a, b, false); }

B pair_c1(B t,      B x) { return m_vec1(x); }
B pair_c2(B t, B w, B x) { return m_vec2Base(w, x, true); }

B shape_c1(B t, B x) {
  if (isAtm(x)) return m_vec1(x);
  if (RNK(x)==1) return x;
  usz ia = IA(x);
  if (ia==1 && TI(x,elType)<el_B) {
    B n = IGet(x,0);
    decG(x);
    return m_vec1(n);
  }
  if (reusable(x)) { FL_KEEP(x, fl_squoze);
    decSh(v(x)); arr_shVec(a(x));
    return x;
  }
  return taga(arr_shVec(TI(x,slice)(x, 0, ia)));
}
static B truncReshape(B x, usz xia, usz nia, ur nr, ShArr* sh) { // consumes all
  B r; Arr* ra;
  if (reusable(x) && xia==nia) { r = x; decSh(v(x)); ra = (Arr*)v(r); }
  else { ra = TI(x,slice)(x, 0, nia); r = taga(ra); }
  arr_shSetU(ra, nr, sh);
  return r;
}
static void fill_words(void* rp, u64 v, u64 bytes) {
  usz wds = bytes/8;
  usz ext = bytes%8;
  u64* p = rp;
  for (usz i=0; i<wds; i++) p[i] = v;
  if (ext) memcpy(p+wds, &v, ext);
}
B shape_c2(B t, B w, B x) {
  usz xia = isArr(x)? IA(x) : 1;
  usz nia = 1;
  ur nr;
  ShArr* sh;
  if (isF64(w)) {
    nia = o2s(w);
    nr = 1;
    sh = NULL;
  } else {
    if (isAtm(w)) w = m_atomUnit(w);
    if (RNK(w)>1) thrM("⥊: 𝕨 must have rank at most 1");
    if (IA(w)>UR_MAX) thrM("⥊: Result rank too large");
    nr = IA(w);
    sh = nr<=1? NULL : m_shArr(nr);
    if (TI(w,elType)==el_i32) {
      i32* wi = i32any_ptr(w);
      if (nr>1) for (i32 i = 0; i < nr; i++) sh->a[i] = wi[i];
      bool bad=false, good=false;
      for (i32 i = 0; i < nr; i++) {
        if (wi[i]<0) thrF("⥊: 𝕨 contained %i", wi[i]);
        bad|= mulOn(nia, wi[i]);
        good|= wi[i]==0;
      }
      if (bad && !good) thrM("⥊: 𝕨 too large");
    } else {
      SGetU(w)
      i32 unkPos = -1;
      i32 unkInd;
      bool bad=false, good=false;
      for (i32 i = 0; i < nr; i++) {
        B c = GetU(w, i);
        if (isF64(c)) {
          usz v = o2s(c);
          if (sh) sh->a[i] = v;
          bad|= mulOn(nia, v);
          good|= v==0;
        } else {
          if (isArr(c) || !isVal(c)) thrM("⥊: 𝕨 must consist of natural numbers or ∘ ⌊ ⌽ ↑");
          if (unkPos!=-1) thrM("⥊: 𝕨 contained multiple computed axes");
          unkPos = i;
          if (!isPrim(c)) thrM("⥊: 𝕨 must consist of natural numbers or ∘ ⌊ ⌽ ↑");
          unkInd = ((i32)v(c)->flags) - 1;
          good|= xia==0 | unkInd==n_floor;
        }
      }
      if (bad && !good) thrM("⥊: 𝕨 too large");
      if (unkPos!=-1) {
        if (unkInd!=n_atop & unkInd!=n_floor & unkInd!=n_reverse & unkInd!=n_take) thrM("⥊: 𝕨 must consist of natural numbers or ∘ ⌊ ⌽ ↑");
        if (nia==0) thrM("⥊: Can't compute axis when the rest of the shape is empty");
        i64 div = xia/nia;
        i64 mod = xia%nia;
        usz item;
        bool fill = false;
        if (unkInd == n_atop) {
          if (mod!=0) thrM("⥊: Shape must be exact when reshaping with ∘");
          item = div;
        } else if (unkInd == n_floor) {
          item = div;
        } else if (unkInd == n_reverse) {
          item = mod? div+1 : div;
        } else if (unkInd == n_take) {
          item = mod? div+1 : div;
          fill = true;
        } else UD;
        if (sh) sh->a[unkPos] = item;
        nia = uszMul(nia, item);
        if (fill) {
          if (!isArr(x)) x = m_atomUnit(x);
          x = taga(arr_shVec(take_impl(nia, x)));
          xia = nia;
        }
      }
    }
    decG(w);
  }
  
  Arr* r;
  if (isArr(x)) {
    if (nia <= xia) {
      return truncReshape(x, xia, nia, nr, sh);
    } else {
      if (xia <= 1) {
        if (RARE(xia == 0)) thrM("⥊: Empty 𝕩 and non-empty result");
        B n = IGet(x,0);
        decG(x);
        x = n;
        goto unit;
      }
      if (xia <= nia/2) x = any_squeeze(x);
      
      u8 xl = arrTypeBitsLog(TY(x));
      u8 xt = arrNewType(TY(x));
      u8* rp;
      u64 bi, bf; // Bytes present, bytes wanted
      if (xl == 0) { // Bits
        u64* rq; r = m_bitarrp(&rq, nia);
        rp = (u8*)rq;
        usz nw = BIT_N(nia);
        u64* xp = bitarr_ptr(x);
        u64 b = xia;
        if (b % 8) {
          if (b < 64) {
            // Need to avoid calling bit_cpy with arguments <64 bits apart
            u64 v = xp[0] & (~(u64)0 >> (64-b));
            do { v |= v<<b; b*=2; } while (b%8 && b<64);
            rq[0] = v;
            if (b>64 && nia>64) rq[1] = v>>(64-b/2);
          } else {
            memcpy(rq, xp, (b+7)/8);
          }
          for (; b%8; b*=2) {
            if (b>nw*32) {
              if (b<nia) bit_cpy(rq, b, rq, 0, nia-b);
              b = 64*nw; // Ensure bi>=bf since bf is rounded up
              break;
            }
            bit_cpy(rq, b, rq, 0, b);
          }
        } else {
          memcpy(rp, xp, b/8);
        }
        bi = b/8;
        bf = 8*nw;
        if (bi == 1) { memset(rp, rp[0], bf); bi=bf; }
      } else {
        if (TI(x,elType) == el_B) {
          B xf = getFillQ(x);
          MAKE_MUT(m, nia); mut_init(m, el_B);
          MUTG_INIT(m);
          i64 div = nia/xia;
          i64 mod = nia%xia;
          for (i64 i = 0; i < div; i++) mut_copyG(m, i*xia, x, 0, xia);
          mut_copyG(m, div*xia, x, 0, mod);
          decG(x);
          Arr* ra = mut_fp(m);
          arr_shSetU(ra, nr, sh);
          return withFill(taga(ra), xf);
        }
        u8 xk = xl - 3;
        rp = m_tyarrp(&r, 1<<xk, nia, xt);
        bi = (u64)xia<<xk;
        bf = (u64)nia<<xk;
        memcpy(rp, tyany_ptr(x), bi);
      }
      decG(x);
      if (bi<=8 && !(bi & (bi-1))) {
        // Divisor of 8: write words
        usz b = bi*8;
        u64 v = *(u64*)rp & (~(u64)0 >> (64-b));
        while (b<64) { v |= v<<b; b*=2; }
        fill_words(rp, v, bf);
      } else {
        // Double up to length l, then copy in blocks
        u64 l = 1<<15; if (l>bf) l=bf;
        for (; bi<=l/2; bi+=bi) memcpy(rp+bi, rp, bi);
        u64 e=bi; for (; e+bi<=bf; e+=bi) memcpy(rp+e, rp, bi);
        if (e<bf) memcpy(rp+e, rp, bf-e);
      }
    }
  } else {
    unit:
    #define FILL(E,T,V) T* rp; r = m_##E##arrp(&rp,nia); fill_words(rp, V, (u64)nia*sizeof(T));
    if (isF64(x)) {
      i32 n = (i32)x.f;
      if (RARE(n!=x.f)) {
        FILL(f64,f64,x.u)
      } else if (n==(i8)n) { // memset can be faster than writing words
        u8 b = n;
        i8* rp; u64 nb = nia;
        if (b <= 1)    { r = m_bitarrp((u64**)&rp,nia); nb = 8*BIT_N(nia); b=-b; }
        else           { r = m_i8arrp (       &rp,nia); }
        memset(rp, b, nb);
      } else {
        if(n==(i16)n)  { FILL(i16,i16,(u16)n*0x0001000100010001) }
        else           { FILL(i32,i32,(u32)n*0x0000000100000001) }
      }
    } else if (isC32(x)) {
      u32 c = o2cG(x);
      if      (c==(u8 )c) { u8* rp; r = m_c8arrp(&rp,nia); memset(rp, c, nia); }
      else if (c==(u16)c) { FILL(c16,u16,c*0x0001000100010001) }
      else                { FILL(c32,u32,c*0x0000000100000001) }
    } else {
      incBy(x, nia); // in addition with the existing reference, this covers the filled amount & asFill
      r = m_fillarrp(nia);
      if (sizeof(B)==8) fill_words(fillarr_ptr(r), x.u, (u64)nia*8);
      else for (usz i = 0; i < nia; i++) fillarr_ptr(r)[i] = x;
      fillarr_setFill(r, asFill(x));
    }
    #undef FILL
  }
  arr_shSetU(r,nr,sh);
  return taga(r);
}

B pick_c1(B t, B x) {
  if (isAtm(x)) return x;
  if (RARE(IA(x)==0)) {
    thrM("⊑: Argument cannot be empty");
    // B r = getFillE(x);
    // dec(x);
    // return r;
  }
  B r = IGet(x, 0);
  decG(x);
  return r;
}

static NOINLINE void checkIndexList(B w, ur xr) {
  SGetU(w)
  usz ia = IA(w);
  for (usz i = 0; i < ia; i++) if (!isNum(GetU(w,i))) thrM("⊑: 𝕨 contained list with mixed-type elements");
  if (ia>xr+xr+10) {
    if (RNK(w)!=1) thrF("⊑: Leaf arrays in 𝕨 must have rank 1 (element in 𝕨 has shape %H)", w);
    thrF("⊑: Leaf array in 𝕨 too large (has shape %H)", w);
  }
}
static B recPick(B w, B x) { // doesn't consume
  assert(isArr(w) && isArr(x));
  usz ia = IA(w);
  ur xr = RNK(x);
  usz* xsh = SH(x);
  switch(TI(w,elType)) { default: UD;
    case el_i8:  { i8*  wp = i8any_ptr (w); if(RNK(w)!=1)goto wrr; if (ia!=xr)goto wrl; usz c=0; for (usz i = 0; i < ia; i++) { c = c*xsh[i] + WRAP(wp[i], xsh[i], goto oob); }; return IGet(x,c); }
    case el_i16: { i16* wp = i16any_ptr(w); if(RNK(w)!=1)goto wrr; if (ia!=xr)goto wrl; usz c=0; for (usz i = 0; i < ia; i++) { c = c*xsh[i] + WRAP(wp[i], xsh[i], goto oob); }; return IGet(x,c); }
    case el_i32: { i32* wp = i32any_ptr(w); if(RNK(w)!=1)goto wrr; if (ia!=xr)goto wrl; usz c=0; for (usz i = 0; i < ia; i++) { c = c*xsh[i] + WRAP(wp[i], xsh[i], goto oob); }; return IGet(x,c); }
    case el_f64: { f64* wp = f64any_ptr(w); if(RNK(w)!=1)goto wrr; if (ia!=xr)goto wrl; usz c=0; for (usz i = 0; i < ia; i++) { i64 ws = (i64)wp[i]; if (wp[i]!=ws) thrM(ws==I64_MIN? "⊑: 𝕨 contained value too large" : "⊑: 𝕨 contained a non-integer");
                                                                                                                                c = c*xsh[i] + WRAP(ws,    xsh[i], goto oob); }; return IGet(x,c); }
    case el_c8: case el_c16: case el_c32: case el_bit:
    case el_B: {
      if (ia==0) {
        if (xr!=0) thrM("⊑: Empty array in 𝕨 must correspond to unit in 𝕩");
        return IGet(x,0);
      }
      SGetU(w)
      if (isNum(GetU(w,0))) {
        if(RNK(w)!=1) goto wrr;
        if (ia!=xr) goto wrl;
        usz c=0;
        for (usz i = 0; i < ia; i++) {
          B cw = GetU(w,i);
          if (!isNum(cw)) thrM("⊑: 𝕨 contained list with mixed-type elements");
          c = c*xsh[i] + WRAP(o2i64(cw), xsh[i], goto oob);
        }
        return IGet(x,c);
      } else {
        M_HARR(r, ia);
        for(usz i=0; i<ia; i++) {
          B c = GetU(w, i);
          if (isAtm(c)) thrM("⊑: 𝕨 contained list with mixed-type elements");
          HARR_ADD(r, i, recPick(c, x));
        }
        return HARR_FC(r, w);
      }
    }
  }
  #undef PICK
  
  wrr: checkIndexList(w, xr); thrF("⊑: Leaf arrays in 𝕨 must have rank 1 (element: %B)", w); // wrong index rank
  wrl: checkIndexList(w, xr); thrF("⊑: Picking item at wrong rank (index %B in array of shape %H)", w, x); // wrong index length
  oob: checkIndexList(w, xr); thrF("⊑: Indexing out-of-bounds (index %B in array of shape %H)", w, x);
}

B pick_c2(B t, B w, B x) {
  if (RARE(isAtm(x))) {
    if (isArr(w) && RNK(w)==1 && IA(w)==0) { dec(w); return x; }
    x = m_atomUnit(x);
  }
  if (isNum(w)) {
    if (RNK(x)!=1) thrF("⊑: 𝕩 must be a list when 𝕨 is a number (%H ≡ ≢𝕩)", x);
    usz p = WRAP(o2i64(w), IA(x), thrF("⊑: indexing out-of-bounds (𝕨≡%R, %s≡≠𝕩)", w, iaW));
    B r = IGet(x, p);
    decG(x);
    return r;
  }
  if (!isArr(w)) thrM("⊑: 𝕨 must be a numeric array");
  B r = recPick(w, x);
  decG(w); decG(x);
  return r;
}

static B slicev(B x, usz s, usz ia) {
  usz xia = IA(x); assert(s+ia <= xia);
  return taga(arr_shVec(TI(x,slice)(x, s, ia)));
}

FORCE_INLINE B affixes(B x, i32 post) {
  if (!isArr(x) || RNK(x)==0) thrM(post? "↓: Argument must have rank at least 1" : "↑: Argument must have rank at least 1");
  ur xr = RNK(x);
  usz* xsh = SH(x);
  u64 cam = *xsh;
  u64 ria = cam+1;
  M_HARR(r, ria);
  BSS2A slice = TI(x,slice);
  if (xr==1) {
    incByG(x, cam);
    for (usz i = 0; i < ria; i++) HARR_ADD(r, i, taga(arr_shVec(slice(x, post?i:0, post?cam-i:i))));
  } else {
    incByG(x, cam+1);
    assert(xr>=2);
    usz csz = arr_csz(x);
    usz* csh = xsh+1;
    for (usz i = 0; i < ria; i++) {
      usz len = post? cam-i : i;
      Arr* c = slice(x, post? i*csz : 0, len*csz);
      usz* sh = arr_shAlloc(c, xr);
      *(sh++) = len;
      shcpy(sh, csh, xr-1);
      HARR_ADD(r, i, taga(c));
    }
    dec(x);
  }
  B rf = incG(HARR_O(r).a[post? cam : 0]);
  return withFill(HARR_FV(r), rf);
}

B take_c1(B t, B x) { return affixes(x, 0); }
B drop_c1(B t, B x) { return affixes(x, 1); }

extern B rt_take, rt_drop;
B take_c2(B t, B w, B x) {
  if (isNum(w)) {
    if (!isArr(x)) x = m_atomUnit(x);
    i64 wv = o2i64(w);
    ur xr = RNK(x);
    usz csz = 1;
    usz* xsh;
    if (xr>1) {
      csz = arr_csz(x);
      xsh = SH(x);
      ptr_inc(shObjS(xsh)); // we'll look at it at the end and dec there
    }
    i64 t = wv*csz; // TODO error on overflow somehow
    Arr* a;
    if (t>=0) {
      a = take_impl(t, x);
    } else {
      t = -t;
      usz xia = IA(x);
      if (t>xia) {
        B xf = getFillE(x);
        MAKE_MUT(r, t); mut_init(r, el_or(TI(x,elType), selfElType(xf)));
        MUTG_INIT(r);
        mut_fillG(r, 0, xf, t-xia);
        mut_copyG(r, t-xia, x, 0, xia);
        decG(x); dec(xf);
        a = mut_fp(r);
      } else {
        a = TI(x,slice)(x,xia-t,t);
      }
    }
    if (xr<=1) {
      arr_shVec(a);
    } else {
      usz* rsh = arr_shAlloc(a, xr); // xr>1, don't have to worry about 0
      rsh[0] = wv<0?-wv:wv;
      shcpy(rsh+1, xsh+1, xr-1);
      ptr_dec(shObjS(xsh));
    }
    return taga(a);
  }
  return c2(rt_take, w, x);
}
B drop_c2(B t, B w, B x) {
  if (isNum(w) && isArr(x) && RNK(x)==1) {
    i64 v = o2i64(w);
    usz ia = IA(x);
    if (v<0) return -v>ia? slicev(x, 0, 0) : slicev(x, 0, v+ia);
    else     return  v>ia? slicev(x, 0, 0) : slicev(x, v, ia-v);
  }
  return c2(rt_drop, w, x);
}

B join_c1(B t, B x) {
  if (isAtm(x)) thrM("∾: Argument must be an array");

  ur xr = RNK(x);
  usz xia = IA(x);
  if (xia==0) {
    B xf = getFillE(x);
    if (isAtm(xf)) {
      decA(xf); decG(x);
      if (!PROPER_FILLS && xr==1) return emptyHVec();
      thrM("∾: Empty array 𝕩 cannot have an atom fill element");
    }
    ur ir = RNK(xf);
    if (ir<xr) thrF("∾: Empty array 𝕩 fill rank must be at least rank of 𝕩 (shape %H and fill shape %H)", x, xf);
    B xff = getFillQ(xf);
    HArr_p r = m_harrUp(0);
    usz* sh = arr_shAlloc((Arr*)r.c, ir);
    if (sh) {
      sh[0] = 0;
      usz* fsh = SH(xf);
      if (xr>1) {
        usz* xsh = SH(x);
        for (usz i = 0; i < xr; i++) sh[i] = xsh[i]*fsh[i];
      }
      shcpy(sh+xr, fsh+xr, ir-xr);
    }
    dec(xf); decG(x);
    return withFill(r.b, xff);

  } else if (xr==1) {
    SGetU(x)
    B x0 = GetU(x,0);
    B rf; if(SFNS_FILLS) rf = getFillQ(x0);
    ur rm = isAtm(x0) ? 0 : RNK(x0); // Maximum element rank seen
    ur rr = rm;      // Result rank, or minimum possible so far
    ur rd = 0;       // Difference of max and min lengths (0 or 1)
    usz* esh = NULL;
    usz cam = 1;     // Result length
    if (rm) {
      esh = SH(x0);
      cam = *esh++;
    } else {
      rr++;
    }

    for (usz i = 1; i < xia; i++) {
      B c = GetU(x, i);
      ur cr = isAtm(c) ? 0 : RNK(c);
      if (cr == 0) {
        if (rm > 1) thrF("∾: Item ranks in a list can differ by at most one (contained ranks %i and %i)", 0, rm);
        rd=rm; cam++;
      } else {
        usz* csh = SH(c);
        ur cd = rm - cr;
        if (RARE(cd > rd)) {
          if ((ur)(cd+1-rd) > 2-rd) thrF("∾: Item ranks in a list can differ by at most one (contained ranks %i and %i)", rm-rd*(cr==rm), cr);
          if (cr > rr) { // Previous elements were cells
            esh--;
            if (cam != i * *esh) thrM("∾: Item trailing shapes must be equal");
            rr=cr; cam=i;
          }
          rm = cr>rm ? cr : rm;
          rd = 1;
        }
        cam += cr < rm ? 1 : *csh++;
        if (!eqShPart(csh, esh, cr-1)) thrF("∾: Item trailing shapes must be equal (contained arrays with shapes %H and %H)", x0, c);
      }
      if (SFNS_FILLS && !noFill(rf)) rf = fill_or(rf, getFillQ(c));
    }
    if (rm==0) thrM("∾: Some item rank must be equal or greater than rank of argument");
    
    usz csz = shProd(esh, 0, rr-1);
    MAKE_MUT(r, cam*csz);
    usz ri = 0;
    for (usz i = 0; i < xia; i++) {
      B c = GetU(x, i);
      if (isArr(c)) {
        usz cia = IA(c);
        mut_copy(r, ri, c, 0, cia);
        ri+= cia;
      } else {
        mut_set(r, ri, inc(c));
        ri++;
      }
    }
    assert(ri==cam*csz);
    Arr* ra = mut_fp(r);
    usz* sh = arr_shAlloc(ra, rr);
    if (sh) {
      sh[0] = cam;
      shcpy(sh+1, esh, rr-1);
    }
    decG(x);
    return SFNS_FILLS? qWithFill(taga(ra), rf) : taga(ra);
  } else if (xr==0) {
    return bqn_merge(x);
  } else {
    SGetU(x)
    B x0 = GetU(x,0);
    B rf; if(SFNS_FILLS) rf = getFillQ(x0);
    ur r0 = isAtm(x0) ? 0 : RNK(x0);

    usz xia = IA(x);
    usz* xsh = SH(x);
    usz tlen = 4*xr+2*r0; for (usz a=0; a<xr; a++) tlen+=xsh[a];
    TALLOC(usz, st, tlen);                                     // Temp buffer
    st[xr-1]=1; for (ur a=xr; a-->1; ) st[a-1] = st[a]*xsh[a]; // Stride
    usz* tsh0 = st+xr; usz* tsh = tsh0+xr+r0;                  // Test shapes
    // Length buffer i is lp+lp[i]
    usz* lp = tsh+xr+r0; lp[0]=xr; for (usz a=1; a<xr; a++) lp[a] = lp[a-1]+xsh[a-1];

    // Expand checked region from the root ⊑𝕩 along each axis in order,
    // so that a non-root element is checked when the axis of the first
    // nonzero in its index is reached.
    ur tr = r0; // Number of root axes remaining
    for (ur a = 0; a < xr; a++) {
      // Check the axis starting at the root, getting axis lengths
      usz n = xsh[a];
      usz *ll = lp+lp[a];
      if (n == 1) {
        if (!tr) thrM("∾: Ranks of argument items too small");
        st[a] = ll[0] = SH(x0)[r0-tr];
        tr--; continue;
      }
      usz step = st[a];
      ll[0] = r0;
      for (usz i=1; i<n; i++) {
        B c = GetU(x, i*step);
        ll[i] = LIKELY(isArr(c)) ? RNK(c) : 0;
      }
      usz r1s=r0; for (usz i=1; i<n; i++) if (ll[i]>r1s) r1s=ll[i];
      ur r1 = r1s;
      ur a0 = r1==r0;  // Root has axis a
      if (tr < a0) thrM("∾: Ranks of argument items too small");
      for (usz i=0; i<n; i++) {
        ur rd = r1 - ll[i];
        if (rd) {
          if (rd>1) thrF("∾: Item ranks along an axis can differ by at most one (contained ranks %i and %i along axis %i)", ll[i], r1, a);
          ll[i] = -1;
        } else {
          B c = GetU(x, i*step);
          ll[i] = SH(c)[r0-tr];
        }
      }

      // Check shapes
      for (usz j=0; j<xia; j+=n*step) {
        B base = GetU(x, j);
        ur r = isAtm(base) ? 0 : RNK(base);
        ur r1 = r+1-a0;
        ur lr = 0;
        if (r) {
          usz* sh=SH(base);
          lr = r - tr;
          shcpy(tsh,sh,r); shcpy(tsh0,sh,r);
          if (!a0) shcpy(tsh +lr+1, tsh +lr  , tr  );
          else     shcpy(tsh0+lr  , tsh0+lr+1, tr-1);
        }
        for (usz i=1; i<n; i++) {
          B c = GetU(x, j+i*step);
          bool rd = ll[i]==-1;
          tsh[lr] = ll[i];
          ur cr=0; usz* sh=NULL; if (!isAtm(c)) { cr=RNK(c); sh=SH(c); }
          if (cr != r1-rd) thrF("∾: Incompatible item ranks", base, c);
          if (!eqShPart(rd?tsh0:tsh, sh, cr)) thrF("∾: Incompatible item shapes (contained arrays with shapes %H and %H along axis %i)", base, c, a);
          if (SFNS_FILLS && !noFill(rf)) rf = fill_or(rf, getFillQ(c));
        }
      }
      tr -= a0;

      // Transform to lengths by changing -1 to 1, and get total
      usz len = 0;
      for (usz i=0; i<n; i++) {
        len += ll[i] &= 1 | -(usz)(ll[i]!=-1);
      }
      st[a] = len;
    }

    // Move the data
    usz* csh = tr ? SH(x0) + r0-tr : NULL;  // Trailing shape
    usz csz = shProd(csh, 0, tr);
    MAKE_MUT(r, shProd(st, 0, xr)*csz);
    // Element index and effective shape, updated progressively
    usz *ei =tsh;  for (usz i=0; i<xr; i++) ei [i]=0;
    usz ri = 0;
    usz *ll = lp+lp[xr-1];
    for (usz i = 0;;) {
      B e = GetU(x, i);
      usz l = ll[ei[xr-1]] * csz;
      if (RARE(isAtm(e))) {
        assert(l==1);
        mut_set(r, ri, inc(e));
      } else {
        usz eia = IA(e);
        if (eia) {
          usz rj = ri;
          usz *ii=tsh0; for (usz k=0; k<xr-1; k++) ii[k]=0;
          usz str0 = st[xr-1]*csz;
          for (usz j=0;;) {
            mut_copy(r, rj, e, j, l);
            j+=l; if (j==eia) break;
            usz str = str0;
            rj += str;
            for (usz a = xr-2; RARE(++ii[a] == lp[lp[a]+ei[a]]); a--) {
              rj -= ii[a]*str;
              ii[a] = 0;
              str *= st[a];
              rj += str;
            }
          }
        }
      }
      if (++i == xia) break;
      ri += l;
      usz str = csz;
      for (usz a = xr-1; RARE(++ei[a] == xsh[a]); ) {
        ei[a] = 0;
        str *= st[a];
        a--;
        ri += (lp[lp[a]+ei[a]]-1) * str;
      }
    }
    Arr* ra = mut_fp(r);
    usz* sh = arr_shAlloc(ra, xr+tr);
    shcpy(sh   , st , xr);
    shcpy(sh+xr, csh, tr);
    TFREE(st);
    decG(x);
    return SFNS_FILLS? qWithFill(taga(ra), rf) : taga(ra);
  }
}
B join_c2(B t, B w, B x) {
  if (isAtm(w)) w = m_atomUnit(w);
  ur wr = RNK(w);
  if (isAtm(x)) {
    if (wr==1 && inplace_add(w, x)) return w;
    x = m_atomUnit(x);
  }
  ur xr = RNK(x);
  B f = fill_both(w, x);
  
  ur c = wr>xr?wr:xr;
  if (c==0) {
    HArr_p r = m_harrUv(2);
    r.a[0] = IGet(w,0); decG(w);
    r.a[1] = IGet(x,0); decG(x);
    return qWithFill(r.b, f);
  }
  if (c-wr > 1 || c-xr > 1) thrF("∾: Argument ranks must differ by 1 or less (%i≡=𝕨, %i≡=𝕩)", wr, xr);
  
  bool reusedW;
  B r = arr_join_inline(w, x, false, &reusedW);
  if (c==1) {
    if (RNK(r)==0) SRNK(r,1);
  } else {
    assert(c>1);
    ur rnk0 = RNK(r);
    ShArr* sh0 = shObj(r);
    usz wia;
    usz* wsh;
    if (wr==1 && reusedW) {
      wia = IA(w)-IA(x);
      wsh = &wia;
    } else {
      wsh = SH(w); // when wr>1, shape object won't be disturbed by arr_join_inline
    }
    usz* xsh = SH(x);
    SRNK(r, 0); // otherwise shape allocation failing may break things
    usz* rsh = arr_shAlloc(a(r), c);
    #if PRINT_JOIN_REUSE
    printf(reusedW? "reuse:1;" : "reuse:0;");
    #endif
    for (i32 i = 1; i < c; i++) {
      usz s = xsh[i+xr-c];
      if (RARE(wsh[i+wr-c] != s)) {
        B msg = make_fmt("∾: Lengths not matchable (%2H ≡ ≢𝕨, %H ≡ ≢𝕩)", wr, wsh, x);
        if (rnk0>1) decShObj(sh0);
        mm_free((Value*)shObjS(rsh));
        arr_shVec(a(r));
        thr(msg);
      }
      rsh[i] = s;
    }
    rsh[0] = (wr==c? wsh[0] : 1) + (xr==c? xsh[0] : 1);
    if (rnk0>1) decShObj(sh0);
  }
  
  decG(x);
  if (!reusedW) decG(w);
  return qWithFill(r, f);
}


B couple_c1(B t, B x) {
  if (isAtm(x)) return m_vec1(x);
  usz rr = RNK(x);
  usz ia = IA(x);
  Arr* r = TI(x,slice)(incG(x),0, ia);
  usz* sh = arr_shAlloc(r, rr+1);
  if (sh) { sh[0] = 1; shcpy(sh+1, SH(x), rr); }
  decG(x);
  return taga(r);
}
B couple_c2(B t, B w, B x) {
  if (isAtm(w)&isAtm(x)) return m_vec2(w, x);
  if (isAtm(w)) w = m_atomUnit(w);
  if (isAtm(x)) x = m_atomUnit(x);
  if (!eqShape(w, x)) thrF("≍: 𝕨 and 𝕩 must have equal shapes (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x);
  usz ia = IA(w);
  ur wr = RNK(w);
  MAKE_MUT(r, ia*2); mut_init(r, el_or(TI(w,elType), TI(x,elType)));
  MUTG_INIT(r);
  mut_copyG(r, 0,  w, 0, ia);
  mut_copyG(r, ia, x, 0, ia);
  Arr* ra = mut_fp(r);
  usz* sh = arr_shAlloc(ra, wr+1);
  if (sh) { sh[0]=2; shcpy(sh+1, SH(w), wr); }
  if (!SFNS_FILLS) { decG(w); decG(x); return taga(ra); }
  B rf = fill_both(w, x);
  decG(w); decG(x);
  return qWithFill(taga(ra), rf);
}


static inline void shift_check(B w, B x) {
  ur wr = RNK(w); usz* wsh = SH(w);
  ur xr = RNK(x); usz* xsh = SH(x);
  if (wr+1!=xr & wr!=xr) thrF("shift: =𝕨 must be =𝕩 or ¯1+=𝕩 (%i≡=𝕨, %i≡=𝕩)", wr, xr);
  for (i32 i = 1; i < xr; i++) if (wsh[i+wr-xr] != xsh[i]) thrF("shift: Lengths not matchable (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x);
}

B shiftb_c1(B t, B x) {
  if (isAtm(x) || RNK(x)==0) thrM("»: Argument cannot be a scalar");
  usz ia = IA(x);
  if (ia==0) return x;
  B xf = getFillE(x);
  usz csz = arr_csz(x);
  
  MAKE_MUT(r, ia); mut_init(r, el_or(TI(x,elType), selfElType(xf)));
  MUTG_INIT(r);
  mut_copyG(r, csz, x, 0, ia-csz);
  mut_fillG(r, 0, xf, csz);
  return qWithFill(mut_fcd(r, x), xf);
}
B shiftb_c2(B t, B w, B x) {
  if (isAtm(x) || RNK(x)==0) thrM("»: 𝕩 cannot be a scalar");
  if (isAtm(w)) w = m_atomUnit(w);
  shift_check(w, x);
  B f = fill_both(w, x);
  usz wia = IA(w);
  usz xia = IA(x);
  MAKE_MUT(r, xia); mut_init(r, el_or(TI(w,elType), TI(x,elType)));
  MUTG_INIT(r);
  int mid = wia<xia? wia : xia;
  mut_copyG(r, 0  , w, 0, mid);
  mut_copyG(r, mid, x, 0, xia-mid);
  decG(w);
  return qWithFill(mut_fcd(r, x), f);
}

B shifta_c1(B t, B x) {
  if (isAtm(x) || RNK(x)==0) thrM("«: Argument cannot be a scalar");
  usz ia = IA(x);
  if (ia==0) return x;
  B xf = getFillE(x);
  usz csz = arr_csz(x);
  MAKE_MUT(r, ia); mut_init(r, el_or(TI(x,elType), selfElType(xf)));
  MUTG_INIT(r);
  mut_copyG(r, 0, x, csz, ia-csz);
  mut_fillG(r, ia-csz, xf, csz);
  return qWithFill(mut_fcd(r, x), xf);
}
B shifta_c2(B t, B w, B x) {
  if (isAtm(x) || RNK(x)==0) thrM("«: 𝕩 cannot be a scalar");
  if (isAtm(w)) w = m_atomUnit(w);
  shift_check(w, x);
  B f = fill_both(w, x);
  usz wia = IA(w);
  usz xia = IA(x);
  MAKE_MUT(r, xia); mut_init(r, el_or(TI(w,elType), TI(x,elType)));
  MUTG_INIT(r);
  if (wia < xia) {
    usz m = xia-wia;
    mut_copyG(r, 0, x, wia, m);
    mut_copyG(r, m, w, 0, wia);
  } else {
    mut_copyG(r, 0, w, wia-xia, xia);
  }
  decG(w);
  return qWithFill(mut_fcd(r, x), f);
}

static u64 bit_reverse(u64 x) {
  u64 c = __builtin_bswap64(x);
  c = (c&0x0f0f0f0f0f0f0f0f)<<4 | (c&0xf0f0f0f0f0f0f0f0)>>4;
  c = (c&0x3333333333333333)<<2 | (c&0xcccccccccccccccc)>>2;
  c = (c&0x5555555555555555)<<1 | (c&0xaaaaaaaaaaaaaaaa)>>1;
  return c;
}
extern B rt_reverse;
B reverse_c1(B t, B x) {
  if (isAtm(x) || RNK(x)==0) thrM("⌽: Argument cannot be a unit");
  usz n = *SH(x);
  if (n==0) return x;
  u8 xl = cellWidthLog(x);
  u8 xt = arrNewType(TY(x));
  if (xl<=6 && (xl>=3 || xl==0)) {
    void* xv = tyany_ptr(x);
    B r;
    switch(xl) { default: UD; break;
      case 0: {
        u64* rp; r = m_bitarrc(&rp, x);
        u64* xp=xv; usz g = BIT_N(n); usz e = g-1;
        for (usz i = 0; i < g; i++) rp[i] = bit_reverse(xp[e-i]);
        if (n&63) {
          u64 sh=(-n)&63;
          for (usz i=0; i<e; i++) rp[i]=rp[i]>>sh|rp[i+1]<<(64-sh);
          rp[e]>>=sh;
        }
        break;
      }
      case 3:                         { u8*  xp=xv; u8*  rp = m_tyarrc(&r, 1, x, xt); for (usz i=0; i<n; i++) rp[i]=xp[n-i-1]; break; }
      case 4:                         { u16* xp=xv; u16* rp = m_tyarrc(&r, 2, x, xt); for (usz i=0; i<n; i++) rp[i]=xp[n-i-1]; break; }
      case 5:                         { u32* xp=xv; u32* rp = m_tyarrc(&r, 4, x, xt); for (usz i=0; i<n; i++) rp[i]=xp[n-i-1]; break; }
      case 6: if (TI(x,elType)!=el_B) { u64* xp=xv; u64* rp = m_tyarrc(&r, 8, x, xt); for (usz i=0; i<n; i++) rp[i]=xp[n-i-1]; break; }
      else {
        HArr_p rp = m_harrUc(x);
        B* xp = arr_bptr(x);
        if (xp!=NULL)  for (usz i=0; i<n; i++) rp.a[i] = inc(xp[n-i-1]);
        else { SGet(x) for (usz i=0; i<n; i++) rp.a[i] = Get(x, n-i-1); }
        r = rp.b;
        B xf = getFillQ(x);
        decG(x);
        return withFill(r, xf);
      }
    }
    decG(x);
    return r;
  }
  B xf = getFillQ(x);
  SLOW1("⌽𝕩", x);
  usz csz = arr_csz(x);
  usz cam = SH(x)[0];
  usz rp = 0;
  usz ip = IA(x);
  MAKE_MUT(r, ip); mut_init(r, TI(x,elType));
  MUTG_INIT(r);
  for (usz i = 0; i < cam; i++) {
    ip-= csz;
    mut_copyG(r, rp, x, ip, csz);
    rp+= csz;
  }
  return withFill(mut_fcd(r, x), xf);
}
B reverse_c2(B t, B w, B x) {
  if (isArr(w)) return c2(rt_reverse, w, x);
  if (isAtm(x) || RNK(x)==0) thrM("⌽: 𝕩 must have rank at least 1 for atom 𝕨");
  usz xia = IA(x);
  if (xia==0) return x;
  B xf = getFillQ(x);
  usz cam = SH(x)[0];
  usz csz = arr_csz(x);
  i64 am = o2i64(w);
  if ((u64)am >= (u64)cam) { am%= (i64)cam; if(am<0) am+= cam; }
  am*= csz;
  MAKE_MUT(r, xia); mut_init(r, TI(x,elType));
  MUTG_INIT(r);
  mut_copyG(r, 0, x, am, xia-am);
  mut_copyG(r, xia-am, x, 0, am);
  return withFill(mut_fcd(r, x), xf);
}

#ifdef __BMI2__
#include <immintrin.h>
#endif

extern B rt_transp;
B transp_c1(B t, B x) {
  if (RARE(isAtm(x))) return m_atomUnit(x);
  ur xr = RNK(x);
  if (xr<=1) return x;
  
  usz ia = IA(x);
  usz* xsh = SH(x);
  usz h = xsh[0];
  usz w = xsh[1] * shProd(xsh, 2, xr);
  
  Arr* r;
  usz xi = 0;
  u8 xe = TI(x,elType);
  if (h==2) {
    if (xe==el_B) {
      B* xp = arr_bptr(x); if (xp==NULL) { HArr* xa=cpyHArr(x); x=taga(xa); xp=xa->a; }
      B* x0 = xp; B* x1 = x0+w;
      HArr_p rp = m_harrUp(ia);
      for (usz i=0; i<w; i++) { rp.a[i*2] = inc(x0[i]); rp.a[i*2+1] = inc(x1[i]); }
      r = (Arr*) rp.c;
    } else {
      #ifndef __BMI2__
      if (xe==el_bit) { x = taga(cpyI8Arr(x)); xsh=SH(x); xe=el_i8; }
      void* rp = m_tyarrp(&r,elWidth(xe),ia,el2t(xe));
      #else
      void* rp = m_tyarrlbp(&r,elWidthLogBits(xe),ia,el2t(xe));
      #endif
      void* xp = tyany_ptr(x);
      switch(xe) { default: UD;
        #ifdef __BMI2__
        case el_bit:;
          u32* x0 = xp;
          Arr* x1o = TI(x,slice)(inc(x),w,w);
          u32* x1 = (u32*) ((TyArr*)x1o)->a;
          for (usz i=0; i<BIT_N(ia); i++) ((u64*)rp)[i] = _pdep_u64(x0[i], 0x5555555555555555) | _pdep_u64(x1[i], 0xAAAAAAAAAAAAAAAA);
          mm_free((Value*)x1o);
          break;
        #endif
        case el_i8: case el_c8:  { u8*  x0=xp; u8*  x1=x0+w; for (usz i=0; i<w; i++) { ((u8* )rp)[i*2] = x0[i]; ((u8* )rp)[i*2+1] = x1[i]; } } break;
        case el_i16:case el_c16: { u16* x0=xp; u16* x1=x0+w; for (usz i=0; i<w; i++) { ((u16*)rp)[i*2] = x0[i]; ((u16*)rp)[i*2+1] = x1[i]; } } break;
        case el_i32:case el_c32: { u32* x0=xp; u32* x1=x0+w; for (usz i=0; i<w; i++) { ((u32*)rp)[i*2] = x0[i]; ((u32*)rp)[i*2+1] = x1[i]; } } break;
        case el_f64:             { u64* x0=xp; u64* x1=x0+w; for (usz i=0; i<w; i++) { ((u64*)rp)[i*2] = x0[i]; ((u64*)rp)[i*2+1] = x1[i]; } } break;
      }
    }
  } else {
    switch(xe) { default: UD;
      case el_bit: x = taga(cpyI8Arr(x)); xsh=SH(x); xe=el_i8; // fallthough; lazy; TODO squeeze
      case el_i8: case el_c8:  { u8*  xp=tyany_ptr(x); u8*  rp = m_tyarrp(&r,1,ia,el2t(xe)); for(usz y=0;y<h;y++) for(usz x=0;x<w;x++) rp[x*h+y] = xp[xi++]; break; }
      case el_i16:case el_c16: { u16* xp=tyany_ptr(x); u16* rp = m_tyarrp(&r,2,ia,el2t(xe)); for(usz y=0;y<h;y++) for(usz x=0;x<w;x++) rp[x*h+y] = xp[xi++]; break; }
      case el_i32:case el_c32: { u32* xp=tyany_ptr(x); u32* rp = m_tyarrp(&r,4,ia,el2t(xe)); for(usz y=0;y<h;y++) for(usz x=0;x<w;x++) rp[x*h+y] = xp[xi++]; break; }
      case el_f64:             { f64* xp=f64any_ptr(x); f64* rp; r=m_f64arrp(&rp,ia);        for(usz y=0;y<h;y++) for(usz x=0;x<w;x++) rp[x*h+y] = xp[xi++]; break; }
      case el_B: { // can't be bothered to implement a bitarr transpose
        B* xp = arr_bptr(x);
        B xf = getFillR(x);
        if (xp==NULL) { HArr* xa=cpyHArr(x); x=taga(xa); xp=xa->a; } // TODO extract this to an inline function
        
        HArr_p p = m_harrUp(ia);
        for(usz y=0;y<h;y++) for(usz x=0;x<w;x++) p.a[x*h+y] = inc(xp[xi++]); // TODO inc afterwards, but don't when there's a method of freeing a HArr without freeing its elements
        
        usz* rsh = arr_shAlloc((Arr*)p.c, xr);
        if (xr==2) {
          rsh[0] = w;
          rsh[1] = h;
        } else {
          shcpy(rsh, xsh+1, xr-1);
          rsh[xr-1] = h;
        }
        decG(x); return qWithFill(p.b, xf);
      }
    }
  }
  usz* rsh = arr_shAlloc(r, xr);
  if (xr==2) {
    rsh[0] = w;
    rsh[1] = h;
  } else {
    shcpy(rsh, xsh+1, xr-1);
    rsh[xr-1] = h;
  }
  decG(x); return taga(r);
}
B transp_c2(B t, B w, B x) { return c2(rt_transp, w, x); }

B transp_im(B t, B x) {
  if (isAtm(x)) thrM("⍉⁼: 𝕩 must not be an atom");
  if (RNK(x)<=2) return transp_c1(t, x);
  return def_fn_im(bi_transp, x);
}


B pick_uc1(B t, B o, B x) { // TODO do in-place like pick_ucw; maybe just call it?
  if (isAtm(x) || IA(x)==0) return def_fn_uc1(t, o, x);
  B xf = getFillQ(x);
  usz ia = IA(x);
  B arg = IGet(x, 0);
  B rep = c1(o, arg);
  MAKE_MUT(r, ia); mut_init(r, el_or(TI(x,elType), selfElType(rep)));
  MUTG_INIT(r);
  mut_setG(r, 0, rep);
  mut_copyG(r, 1, x, 1, ia-1);
  return qWithFill(mut_fcd(r, x), xf);
}

B pick_ucw(B t, B o, B w, B x) {
  if (isArr(w) || isAtm(x) || RNK(x)!=1) return def_fn_ucw(t, o, w, x);
  usz xia = IA(x);
  usz wi = WRAP(o2i64(w), xia, thrF("𝔽⌾(n⊸⊑)𝕩: reading out-of-bounds (n≡%R, %s≡≠𝕩)", w, xia));
  if (TI(x,elType)==el_B) {
    B* xp;
    if (TY(x)==t_harr || TY(x)==t_hslice) {
      if (!(TY(x)==t_harr && reusable(x))) x = taga(cpyHArr(x));
      xp = harr_ptr(x);
    } else if (TY(x)==t_fillarr && reusable(x)) {
      xp = fillarr_ptr(a(x));
    } else {
      Arr* x2 = m_fillarrp(xia);
      fillarr_setFill(x2, getFillQ(x));
      xp = fillarr_ptr(x2);
      COPY_TO(xp, el_B, 0, x, 0, xia);
      arr_shCopy(x2, x);
      dec(x);
      x = taga(x2);
    }
    B c = xp[wi];
    xp[wi] = m_f64(0);
    xp[wi] = c1(o, c);
    return x;
  }
  B arg = IGet(x, wi);
  B rep = c1(o, arg);
  if (reusable(x) && TI(x,canStore)(rep)) { REUSE(x);
    u8 xt = TY(x);
    if      (xt==t_i8arr ) { i8*  xp = i8any_ptr (x); xp[wi] = o2iG(rep); return x; }
    else if (xt==t_i16arr) { i16* xp = i16any_ptr(x); xp[wi] = o2iG(rep); return x; }
    else if (xt==t_i32arr) { i32* xp = i32any_ptr(x); xp[wi] = o2iG(rep); return x; }
    else if (xt==t_f64arr) { f64* xp = f64any_ptr(x); xp[wi] = o2fG(rep); return x; }
    else if (xt==t_c8arr ) { u8*  xp = c8any_ptr (x); xp[wi] = o2cG(rep); return x; }
    else if (xt==t_c16arr) { u16* xp = c16any_ptr(x); xp[wi] = o2cG(rep); return x; }
    else if (xt==t_c32arr) { u32* xp = c32any_ptr(x); xp[wi] = o2cG(rep); return x; }
  }
  MAKE_MUT(r, xia); mut_init(r, el_or(TI(x,elType), selfElType(rep)));
  MUTG_INIT(r);
  mut_setG(r, wi, rep);
  mut_copyG(r, 0, x, 0, wi);
  mut_copyG(r, wi+1, x, wi+1, xia-wi-1);
  B xf = getFillQ(x);
  return qWithFill(mut_fcd(r, x), xf);
}

static B takedrop_ucw(i64 wi, B o, u64 am, B x, size_t xr) {
  usz xia = IA(x);
  usz csz = arr_csz(x);
  usz tk = csz*am; // taken element count
  usz lv = xia-tk; // elements left alone
  
  Arr* arg = TI(x,slice)(incG(x), wi<0? lv : 0, tk);
  usz* ash = arr_shAlloc(arg, xr);
  if (ash) { ash[0] = am; shcpy(ash+1, SH(x)+1, xr-1); }
  
  B rep = c1(o, taga(arg));
  if (isAtm(rep)) thrM("𝔽⌾(n⊸↑): 𝔽 returned an atom");
  usz* repsh = SH(rep);
  if (RNK(rep)==0 || !eqShPart(repsh+1, SH(x)+1, xr-1) || repsh[0]!=am) thrM("𝔽⌾(n⊸↑)𝕩: 𝔽 returned an array with a different shape than n↑𝕩");
  
  MAKE_MUT(r, xia);
  mut_init(r, el_or(TI(x,elType), TI(rep,elType))); MUTG_INIT(r);
  if (wi<0) {
    mut_copyG(r, 0, x, 0, lv);
    mut_copyG(r, lv, rep, 0, tk);
  } else {
    mut_copyG(r, 0, rep, 0, tk);
    mut_copyG(r, tk, x, tk, lv);
  }
  
  dec(rep);
  return mut_fcd(r, x);
}

B take_ucw(B t, B o, B w, B x) {
  if (!isF64(w)) return def_fn_ucw(t, o, w, x);
  i64 wi = o2i64(w);
  u64 am = wi<0? -wi : wi;
  if (isAtm(x)) x = m_vec1(x);
  ur xr = RNK(x); if (xr==0) xr = 1;
  if (am>SH(x)[0]) thrF("𝔽⌾(n⊸↑)𝕩: Cannot modify fill with Under (%l ≡ 𝕨, %H ≡ ≢𝕩)", wi, x);
  return takedrop_ucw(wi, o, am, x, xr);
}

B drop_ucw(B t, B o, B w, B x) {
  if (!isF64(w)) return def_fn_ucw(t, o, w, x);
  i64 wi = o2i64(w);
  u64 am = wi<0? -wi : wi;
  if (isAtm(x)) x = m_vec1(x);
  ur xr = RNK(x); if (xr==0) xr = 1;
  usz cam = SH(x)[0];
  if (am>cam) am = cam;
  return takedrop_ucw(-wi, o, cam-am, x, xr);
}

static B shape_uc1_t(B r, usz ia) {
  if (!isArr(r) || RNK(r)!=1 || IA(r)!=ia) thrM("𝔽⌾⥊: 𝔽 changed the shape of the argument");
  return r;
}
B shape_uc1(B t, B o, B x) {
  if (!isArr(x) || RNK(x)==0) {
    usz xia = isArr(x)? IA(x) : 1;
    return shape_c2(t, emptyIVec(), shape_uc1_t(c1(o, shape_c1(t, x)), xia));
  }
  usz xia = IA(x);
  if (RNK(x)==1) return shape_uc1_t(c1(o, x), xia);
  ur xr = RNK(x);
  ShArr* sh = ptr_inc(shObj(x));
  return truncReshape(shape_uc1_t(c1(o, shape_c1(t, x)), xia), xia, xia, xr, sh);
}

B select_ucw(B t, B o, B w, B x);

B  transp_uc1(B t, B o, B x) { return  transp_im(t, c1(o,  transp_c1(t, x))); }
B reverse_uc1(B t, B o, B x) { return reverse_c1(t, c1(o, reverse_c1(t, x))); }

NOINLINE B enclose_im(B t, B x) {
  if (isAtm(x) || RNK(x)!=0) thrM("<⁼: Argument wasn't a rank 0 array");
  B r = IGet(x, 0);
  dec(x);
  return r;
}
B enclose_uc1(B t, B o, B x) {
  return enclose_im(t, c1(o, m_atomUnit(x)));
}

void sfns_init() {
  c(BFn,bi_pick)->uc1 = pick_uc1;
  c(BFn,bi_reverse)->uc1 = reverse_uc1;
  c(BFn,bi_pick)->ucw = pick_ucw;
  c(BFn,bi_select)->ucw = select_ucw; // TODO move to new init fn
  c(BFn,bi_shape)->uc1 = shape_uc1;
  c(BFn,bi_transp)->uc1 = transp_uc1;
  c(BFn,bi_transp)->im = transp_im;
  c(BFn,bi_take)->ucw = take_ucw;
  c(BFn,bi_drop)->ucw = drop_ucw;
  c(BFn,bi_lt)->im = enclose_im;
  c(BFn,bi_lt)->uc1 = enclose_uc1;
}
