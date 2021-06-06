#include "../core.h"
#include "../utils/mut.h"
#include "../utils/utf.h"

u64 allocB; // currently allocated number of bytes
B bi_emptyHVec, bi_emptyIVec, bi_emptyCVec, bi_emptySVec;

NOINLINE void arr_print(B x) { // should accept refc=0 arguments for debugging purposes
  ur r = rnk(x);
  BS2B xgetU = TI(x).getU;
  usz ia = a(x)->ia;
  if (r!=1) {
    if (r==0) {
      printf("<");
      print(xgetU(x,0));
      return;
    }
    usz* sh = a(x)->sh;
    for (i32 i = 0; i < r; i++) {
      if(i==0)printf("%d",sh[i]);
      else printf("‿%d",sh[i]);
    }
    printf("⥊");
  } else if (ia>0) {
    for (usz i = 0; i < ia; i++) {
      B c = xgetU(x,i);
      if (!isC32(c) || (u32)c.u=='\n') goto reg;
    }
    printf("\"");
    for (usz i = 0; i < ia; i++) printUTF8((u32)xgetU(x,i).u); // c32, no need to decrement
    printf("\"");
    return;
  }
  reg:;
  printf("⟨");
  for (usz i = 0; i < ia; i++) {
    if (i!=0) printf(", ");
    print(xgetU(x,i));
  }
  printf("⟩");
}

NOINLINE void print(B x) {
  if (isF64(x)) {
    printf("%.14g", x.f);
  } else if (isC32(x)) {
    if ((u32)x.u>=32) { printf("'"); printUTF8((u32)x.u); printf("'"); }
    else if((u32)x.u>15) printf("\\x%x", (u32)x.u);
    else printf("\\x0%x", (u32)x.u);
  } else if (isI32(x)) {
    printf("%d", (i32)x.u);
  } else if (isVal(x)) {
    #ifdef DEBUG
    if (isVal(x) && (v(x)->type==t_freed || v(x)->type==t_empty)) {
      u8 t = v(x)->type;
      v(x)->type = v(x)->flags;
      printf(t==t_freed?"FREED:":"EMPTY:");
      TI(x).print(x);
      v(x)->type = t;
      return;
    }
    #endif
    TI(x).print(x);
  }
  else if (isVar(x)) printf("(var d=%d i=%d)", (u16)(x.u>>32), (i32)x.u);
  else if (isExt(x)) printf("(extvar d=%d i=%d)", (u16)(x.u>>32), (i32)x.u);
  else if (x.u==bi_N.u) printf("·");
  else if (x.u==bi_optOut.u) printf("(value optimized out)");
  else if (x.u==bi_noVar.u) printf("(unset variable placeholder)");
  else if (x.u==bi_badHdr.u) printf("(bad header note)");
  else if (x.u==bi_noFill.u) printf("(no fill placeholder)");
  else printf("(todo tag "N64x")", x.u>>48);
}

NOINLINE void printRaw(B x) {
  if (isAtm(x)) {
    if (isF64(x)) printf("%.14g", x.f);
    else if (isC32(x)) printUTF8((u32)x.u);
    else thrM("bad printRaw argument: atom arguments should be either numerical or characters");
  } else {
    usz ia = a(x)->ia;
    BS2B xgetU = TI(x).getU;
    for (usz i = 0; i < ia; i++) {
      B c = xgetU(x,i);
      #if CATCH_ERRORS
      if (c.u==0 || noFill(c)) { printf(" "); continue; }
      #endif
      if (!isC32(c)) thrM("bad printRaw argument: expected all character items");
      printUTF8((u32)c.u);
    }
  }
}
NOINLINE B append_fmt(B s, char* p, ...) {
  va_list a;
  va_start(a, p);
  char buf[30];
  char c;
  char* lp = p;
  while (*p != 0) { c = *p++;
    if (c!='%') continue;
    if (lp!=p-1) s = vec_join(s, fromUTF8(lp, p-1-lp));
    switch(c = *p++) { default: printf("Unknown format character '%c'", c); UD;
      case 'R': { // TODO proper
        B b = va_arg(a, B);
        if (isNum(b)) {
          AFMT("%f", o2f(b));
        } else { assert(isArr(b) && rnk(b)==1);
          s = vec_join(s, inc(b));
        }
        break;
      }
      case 'H': {
        B o = va_arg(a, B);
        ur r = isArr(o)? rnk(o) : 0;
        usz* sh = isArr(o)? a(o)->sh : NULL;
        if (r==0) AU("⟨⟩");
        else if (r==1) AFMT("⟨%s⟩", sh[0]);
        else {
          for (i32 i = 0; i < r; i++) {
            if(i) AU("‿");
            AFMT("%s", sh[i]);
          }
        }
        break;
      }
      case 'S': {
        A8(va_arg(a, char*));
        break;
      }
      case 'U': {
        AU(va_arg(a, char*));
        break;
      }
      case 'u': case 'x': case 'i': case 'l': {
        i32 mode = c=='u'? 1 : c=='x'? 2 : 0;
        if (mode) c = *p++;
        assert(c);
        if (mode) {
         assert(c=='l'||c=='i');
          if (c=='i') snprintf(buf, 30, mode==1? "%u" : "%x", va_arg(a, u32));
          else        snprintf(buf, 30, mode==1? N64u : N64x, va_arg(a, u64));
        } else {
          if (c=='i') {
            i32 v = va_arg(a, i32);
            if (v<0) AU("¯");
            snprintf(buf, 30, N64u, (u64)(v<0?-v:v));
          } else { assert(c=='l');
            i64 v = va_arg(a, i64);
            if (v<0) AU("¯");
            if (v==I64_MIN) snprintf(buf, 30, "9223372036854775808");
            else snprintf(buf, 30, N64u, (u64)(v<0?-v:v));
          }
        }
        A8(buf);
        break;
      }
      case 's': {
        usz v = va_arg(a, usz);
        snprintf(buf, 30, sizeof(usz)==4? "%u" : N64u, v);
        A8(buf);
        break;
      }
      case 'p': {
        snprintf(buf, 30, "%p", va_arg(a, void*));
        A8(buf);
        break;
      }
      case 'f': {
        f64 f = va_arg(a, f64);
        if (f<0) {
          AU("¯");
          f=-f;
        }
        snprintf(buf, 30, "%.14g", f);
        A8(buf);
        break;
      }
      case 'c': {
        buf[0] = va_arg(a, int); buf[1] = 0;
        A8(buf);
        break;
      }
      case '%': {
        buf[0] = '%'; buf[1] = 0;
        A8(buf);
        break;
      }
    }
    lp = p;
  }
  if (lp!=p) s = vec_join(s, fromUTF8(lp, p-lp));
  va_end(a);
  return s;
}

#define CMP(W,X) ({ AUTO wt = (W); AUTO xt = (X); (wt>xt?1:0)-(wt<xt?1:0); })
NOINLINE i32 compareR(B w, B x) {
  if (isNum(w) & isC32(x)) return -1;
  if (isC32(w) & isNum(x)) return  1;
  if (isAtm(w) & isAtm(x)) thrM("Invalid comparison");
  bool wa=isAtm(w); usz wia; ur wr; usz* wsh; BS2B wgetU;
  bool xa=isAtm(x); usz xia; ur xr; usz* xsh; BS2B xgetU;
  if(wa) { wia=1; wr=0; wsh=NULL; wgetU=def_getU; } else { wia=a(w)->ia; wr=rnk(w); wsh=a(w)->sh; wgetU=TI(w).getU; }
  if(xa) { xia=1; xr=0; xsh=NULL; xgetU=def_getU; } else { xia=a(x)->ia; xr=rnk(x); xsh=a(x)->sh; xgetU=TI(x).getU; }
  if (wia==0 || xia==0) return CMP(wia, xia);
  
  i32 rc = CMP(wr+(wa?0:1), xr+(xa?0:1));
  ur rr = wr<xr? wr : xr;
  i32 ri = 0; // matching shape tail
  i32 rm = 1;
  while (ri<rr  &&  wsh[wr-1-ri] == xsh[xr-1-ri]) {
    rm*= wsh[wr-ri-1];
    ri++;
  }
  if (ri<rr) {
    usz wm = wsh[wr-1-ri];
    usz xm = xsh[xr-1-ri];
    rc = CMP(wm, xm);
    rm*= wm<xm? wm : xm;
  }
  for (int i = 0; i < rm; i++) {
    int c = compare(wgetU(w,i), xgetU(x,i));
    if (c!=0) return c;
  }
  return rc;
}
#undef CMP

NOINLINE bool atomEqualR(B w, B x) {
  if (v(w)->type!=v(x)->type) return false;
  B2B dcf = TI(w).decompose;
  if (dcf == def_decompose) return false;
  B wd=dcf(inc(w)); B* wdp = harr_ptr(wd);
  B xd=dcf(inc(x)); B* xdp = harr_ptr(xd);
  if (o2i(wdp[0])<=1) { dec(wd);dec(xd); return false; }
  usz wia = a(wd)->ia;
  if (wia!=a(xd)->ia) { dec(wd);dec(xd); return false; }
  for (i32 i = 0; i<wia; i++) if(!equal(wdp[i], xdp[i]))
                      { dec(wd);dec(xd); return false; }
                        dec(wd);dec(xd); return true;
}
NOINLINE bool equal(B w, B x) { // doesn't consume
  bool wa = isAtm(w);
  bool xa = isAtm(x);
  if (wa!=xa) return false;
  if (wa) return atomEqual(w, x);
  if (!eqShape(w,x)) return false;
  usz ia = a(x)->ia;
  u8 we = TI(w).elType;
  u8 xe = TI(x).elType;
  if (we<=el_f64 && xe<=el_f64) { assert(we==el_i32|we==el_f64); assert(xe==el_i32|xe==el_f64);
    if (we==el_i32) { i32* wp = i32any_ptr(w);
      if(xe==el_i32) { i32* xp = i32any_ptr(x); for (usz i = 0; i < ia; i++) if(wp[i]!=xp[i]) return false; }
      else           { f64* xp = f64any_ptr(x); for (usz i = 0; i < ia; i++) if(wp[i]!=xp[i]) return false; }
    } else { f64* wp = f64any_ptr(w);
      if(xe==el_i32) { i32* xp = i32any_ptr(x); for (usz i = 0; i < ia; i++) if(wp[i]!=xp[i]) return false; }
      else           { f64* xp = f64any_ptr(x); for (usz i = 0; i < ia; i++) if(wp[i]!=xp[i]) return false; }
    }
    return true;
  }
  if (we==el_c32 && xe==el_c32) {
    u32* wp = c32any_ptr(w);
    u32* xp = c32any_ptr(x);
    for (usz i = 0; i < ia; i++) if(wp[i]!=xp[i]) return false;
    return true;
  }
  BS2B xgetU = TI(x).getU;
  BS2B wgetU = TI(w).getU;
  for (usz i = 0; i < ia; i++) if(!equal(wgetU(w,i),xgetU(x,i))) return false;
  return true;
}

bool atomEEqual(B w, B x) { // doesn't consume (not that that matters really currently)
  if (w.u==x.u) return true;
  if(isNum(w)|isNum(x)) return false;
  if (!isVal(w) | !isVal(x)) return false;
  if (v(w)->type!=v(x)->type) return false;
  B2B dcf = TI(w).decompose;
  if (dcf == def_decompose) return false;
  B wd=dcf(inc(w)); B* wdp = harr_ptr(wd);
  B xd=dcf(inc(x)); B* xdp = harr_ptr(xd);
  if (o2i(wdp[0])<=1) { dec(wd);dec(xd); return false; }
  usz wia = a(wd)->ia;
  if (wia!=a(xd)->ia) { dec(wd);dec(xd); return false; }
  for (i32 i = 0; i<wia; i++) if(!eequal(wdp[i], xdp[i]))
                      { dec(wd);dec(xd); return false; }
                        dec(wd);dec(xd); return true;
}
bool eequal(B w, B x) { // doesn't consume
  if (w.u==x.u) return true;
  bool wa = isAtm(w);
  bool xa = isAtm(x);
  if (wa!=xa) return false;
  if (wa) return atomEEqual(w, x);
  // B wf = getFillQ(w);
  // B xf = getFillQ(x);
  // bool feq = eequal(wf, xf);
  // dec(wf); dec(xf);
  // if (!feq) return false;
  if (!eqShape(w,x)) return false;
  usz ia = a(x)->ia;
  BS2B xgetU = TI(x).getU;
  BS2B wgetU = TI(w).getU;
  for (usz i = 0; i < ia; i++) if(!eequal(wgetU(w,i),xgetU(x,i))) return false;
  return true;
}

u64 depth(B x) { // doesn't consume
  if (isAtm(x)) return 0;
  if (TI(x).arrD1) return 1;
  u64 r = 0;
  usz ia = a(x)->ia;
  BS2B xgetU = TI(x).getU;
  for (usz i = 0; i < ia; i++) {
    u64 n = depth(xgetU(x,i));
    if (n>r) r = n;
  }
  return r+1;
}
void slice_free(Value* x) { dec(((Slice*)x)->p); decSh(x); }
void slice_visit(Value* x) { mm_visit(((Slice*)x)->p); }
void slice_print(B x) { arr_print(x); }

char* format_type(u8 u) {
  switch(u) { default: return"(unknown type)";
    case t_empty:return"empty"; case t_shape:return"shape";
    case t_funBI:return"builtin fun"; case t_fun_block:return"fun_block";
    case t_md1BI:return"builtin md1"; case t_md1_block:return"md1_block";
    case t_md2BI:return"builtin md2"; case t_md2_block:return"md2_block";
    case t_fork:return"fork"; case t_atop:return"atop";
    case t_md1D:return"md1D"; case t_md2D:return"md2D"; case t_md2H:return"md2H";
    case t_harr  :return"harr"  ; case t_i8arr  :return"i8arr"  ; case t_i32arr  :return"i32arr"  ; case t_fillarr  :return"fillarr"  ; case t_c32arr  :return"c32arr"  ; case t_f64arr  :return"f64arr"  ;
    case t_hslice:return"hslice"; case t_i8slice:return"i8slice"; case t_i32slice:return"i32slice"; case t_fillslice:return"fillslice"; case t_c32slice:return"c32slice"; case t_f64slice:return"f64slice";
    case t_comp:return"comp"; case t_block:return"block"; case t_body:return"body"; case t_scope:return"scope"; case t_scopeExt:return"scope extension"; case t_blBlocks: return "block list";
    case t_ns:return"ns"; case t_nsDesc:return"nsDesc"; case t_fldAlias:return"alias"; case t_hashmap:return"hashmap"; case t_temp:return"temporary";
    case t_freed:return"(freed by GC)"; case t_harrPartial:return"partHarr";
    #ifdef RT_WRAP
    case t_funWrap:return"wrapped function"; case t_md1Wrap:return"wrapped 1-modifier"; case t_md2Wrap:return "wrapped 2-modifier";
    #endif
  }
}
bool isPureFn(B x) { // doesn't consume
  if (isCallable(x)) {
    if (v(x)->flags) return true;
    B2B dcf = TI(x).decompose;
    B xd = dcf(inc(x));
    B* xdp = harr_ptr(xd);
    i32 t = o2iu(xdp[0]);
    if (t<2) { dec(xd); return t==0; }
    usz xdia = a(xd)->ia;
    for (i32 i = 1; i<xdia; i++) if(!isPureFn(xdp[i])) { dec(xd); return false; }
    dec(xd); return true;
  } else if (isArr(x)) {
    usz ia = a(x)->ia;
    BS2B xgetU = TI(x).getU;
    for (usz i = 0; i < ia; i++) if (!isPureFn(xgetU(x,i))) return false;
    return true;
  } else return isNum(x) || isC32(x);
}

B bqn_squeeze(B x) { // consumes
  assert(isArr(x));
  u8 xe = TI(x).elType;
  if (xe==el_i32 || xe==el_c32) return x;
  usz ia = a(x)->ia;
  if (ia==0) return x;
  if (xe==el_f64) {
    f64* xp = f64any_ptr(x);
    for (usz i = 0; i < ia; i++) if (xp[i] != (f64)(i32)xp[i]) return x;
    return tag(toI32Arr(x), ARR_TAG);
  }
  assert(xe==el_B);
  BS2B xgetU = TI(x).getU;
  B x0 = xgetU(x, 0);
  if (isNum(x0)) {
    for (usz i = 0; i < ia; i++) {
      B c = xgetU(x, i);
      if (!isNum(c)) return x;
      if (!q_i32(c)) {
        for (i++; i < ia; i++) if (!isNum(xgetU(x, i))) return x;
        return tag(toF64Arr(x), ARR_TAG);
      }
    }
    return tag(toI32Arr(x), ARR_TAG);
  } else if (isC32(x0)) {
    for (usz i = 1; i < ia; i++) {
      B c = xgetU(x, i);
      if (!isC32(c)) return x;
    }
    return tag(toC32Arr(x), ARR_TAG);
  } else return x;
}
B bqn_merge(B x) { // consumes
  assert(isArr(x));
  usz xia = a(x)->ia;
  ur xr = rnk(x);
  if (xia==0) {
    B xf = getFillE(x);
    if (isAtm(xf)) { dec(xf); return x; }
    i32 xfr = rnk(xf);
    B xff = getFillQ(xf);
    B r = m_fillarrp(0);
    fillarr_setFill(r, xff);
    if (xr+xfr > UR_MAX) thrM(">: Result rank too large");
    usz* rsh = arr_shAllocI(r, 0, xr+xfr);
    if (rsh) {
      memcpy       (rsh   , a(x)->sh,  xr *sizeof(usz));
      if(xfr)memcpy(rsh+xr, a(xf)->sh, xfr*sizeof(usz));
    }
    return r;
  }
  
  BS2B xgetU = TI(x).getU;
  B x0 = xgetU(x, 0);
  usz* elSh = isArr(x0)? a(x0)->sh : NULL;
  ur elR = isArr(x0)? rnk(x0) : 0;
  usz elIA = isArr(x0)? a(x0)->ia : 1;
  B fill = getFillQ(x0);
  if (xr+elR > UR_MAX) thrM(">: Result rank too large");
  
  MAKE_MUT(r, xia*elIA);
  usz rp = 0;
  for (usz i = 0; i < xia; i++) {
    B c = xgetU(x, i);
    if (isArr(c)? (elR!=rnk(c) || !eqShPrefix(elSh, a(c)->sh, elR)) : elR!=0) { mut_pfree(r, rp); thrF(">: Elements didn't have equal shapes (contained %H and %H)", x0, c); }
    if (isArr(c)) mut_copy(r, rp, c, 0, elIA);
    else mut_set(r, rp, c);
    if (!noFill(fill)) fill = fill_or(fill, getFillQ(c));
    rp+= elIA;
  }
  B rb = mut_fp(r);
  usz* rsh = arr_shAllocR(rb, xr+elR);
  if (rsh) {
    memcpy         (rsh   , a(x)->sh, xr *sizeof(usz));
    if (elSh)memcpy(rsh+xr, elSh,     elR*sizeof(usz));
  }
  dec(x);
  return withFill(rb,fill);
}

#ifdef ALLOC_STAT
  u64* ctr_a = 0;
  u64* ctr_f = 0;
  u64 actrc = 21000;
  u64 talloc = 0;
  #ifdef ALLOC_SIZES
    u32** actrs;
  #endif
#endif

NOINLINE void printAllocStats() {
  #ifdef ALLOC_STAT
    printf("total ever allocated: "N64u"\n", talloc);
    printf("allocated heap size:  "N64u"\n", mm_heapAlloc);
    printf("used heap size:       "N64u"\n", mm_heapUsed());
    ctr_a[t_harr]+= ctr_a[t_harrPartial];
    ctr_a[t_harrPartial] = 0;
    printf("ctrA←"); for (i64 i = 0; i < t_COUNT; i++) { if(i)printf("‿"); printf(N64u, ctr_a[i]); } printf("\n");
    printf("ctrF←"); for (i64 i = 0; i < t_COUNT; i++) { if(i)printf("‿"); printf(N64u, ctr_f[i]); } printf("\n");
    printf("names←⟨"); for (i64 i = 0; i < t_COUNT; i++) { if(i)printf(","); printf("\"%s\"", format_type(i)); } printf("⟩\n");
    u64 leakedCount = 0;
    for (i64 i = 0; i < t_COUNT; i++) leakedCount+= ctr_a[i]-ctr_f[i];
    printf("leaked object count: "N64u"\n", leakedCount);
    #ifdef ALLOC_SIZES
      for(i64 i = 0; i < actrc; i++) {
        u32* c = actrs[i];
        bool any = false;
        for (i64 j = 0; j < t_COUNT; j++) if (c[j]) any=true;
        if (any) {
          printf(N64u, i*4);
          for (i64 k = 0; k < t_COUNT; k++) printf("‿%u", c[k]);
          printf("\n");
        }
      }
    #endif
  #endif
}
