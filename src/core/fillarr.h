typedef struct FillArr {
  struct Arr;
  B fill;
  B a[];
} FillArr;
typedef struct FillSlice {
  struct Slice;
  B* a;
  B fill;
} FillSlice;

B asFill(B x); // consumes
void validateFill(B x);

B withFill(B x, B fill); // consumes both
static B qWithFill(B x, B fill) { // consumes both
  assert(isArr(x));
  if (noFill(fill) || TI(x,elType)!=el_B) return x;
  return withFill(x, fill);
}

NOINLINE bool fillEqualF(B w, B x);
static bool fillEqual(B w, B x) {
  if (w.u==x.u) return true;
  if (isAtm(w)|isAtm(x)) return false;
  return fillEqualF(w, x);
}


static B getFillN(B x) { // doesn't consume, doesn't increment result; can return bi_noFill
  if (isArr(x)) {
    switch(TI(x,elType)) { default: UD;
      case el_i8: case el_i16: case el_i32: case el_f64: case el_bit: return m_i32(0);
      case el_c8: case el_c16: case el_c32: return m_c32(' ');
      case el_B:;
        u8 t = TY(x);
        if (t==t_fillarr  ) return c(FillArr,  x)->fill;
        if (t==t_fillslice) return c(FillSlice,x)->fill;
        return bi_noFill;
    }
  }
  if (isNum(x)) return m_i32(0);
  if (isC32(x)) return m_c32(' ');
  return bi_noFill;
}
static B getFillR(B x) { // doesn't consume; can return bi_noFill
  if (isArr(x)) {
    switch(TI(x,elType)) { default: UD;
      case el_i8: case el_i16: case el_i32: case el_f64: case el_bit: return m_i32(0);
      case el_c8: case el_c16: case el_c32: return m_c32(' ');
      case el_B:;
        u8 t = TY(x);
        if (t==t_fillarr  ) return inc(c(FillArr,  x)->fill);
        if (t==t_fillslice) return inc(c(FillSlice,x)->fill);
        return bi_noFill;
    }
  }
  if (isNum(x)) return m_i32(0);
  if (isC32(x)) return m_c32(' ');
  return bi_noFill;
}
static B getFillQ(B x) { // doesn't consume; returns 0 if !SEMANTIC_CATCH
  B r = getFillR(x);
  #if SEMANTIC_CATCH
    return r;
  #endif
  return noFill(r)? m_f64(0) : r;
}
static B getFillE(B x) { // errors if there's no fill
  B xf = getFillQ(x);
  if (noFill(xf)) {
    if (PROPER_FILLS) thrM("No fill found");
    else return m_f64(0);
  }
  return xf;
}


static Arr* m_fillarrp(usz ia) { // needs a NOGC_E after fill & all elements are initialized
  CHECK_IA(ia, sizeof(B));
  Arr* r = m_arr(fsizeof(FillArr,a,B,ia), t_fillarr, ia);
  NOGC_S;
  return r;
}
static void fillarr_setFill(Arr* x, B fill) { assert(PTY(x)==t_fillarr); ((FillArr*)x)->fill = fill; } // consumes fill
static B* fillarrv_ptr  (Arr* x) { assert(PTY(x)==t_fillarr);   return ((FillArr*)x)->a; }
static B* fillslicev_ptr(Arr* x) { assert(PTY(x)==t_fillslice); return ((FillSlice*)x)->a; }
static Arr* m_fillarrpEmpty(B fill) {
  Arr* r = m_fillarrp(0);
  fillarr_setFill(r, fill);
  NOGC_E;
  return r;
}
static Arr* m_fillarr0p(usz ia) { // zero-initialized fillarr, with both fill & elements set to m_f64(0)
  Arr* r = arr_shVec(m_fillarrp(ia));
  fillarr_setFill(r, m_f64(0));
  FILL_TO(fillarrv_ptr(r), el_B, 0, m_f64(0), ia);
  NOGC_E;
  return r;
}

B m_funit(B x); // consumes
B m_unit(B x); // consumes

static bool fillEqualsGetFill(B fill, B obj) { // returns whether `fill` equals the fill of `obj`
  return fillEqual(fill, getFillN(obj));
}

static B fill_both(B w, B x) { // doesn't consume
  B wf = getFillN(w);
  if (noFill(wf)) return bi_noFill;
  B xf = getFillQ(x);
  if (fillEqual(wf, xf)) return xf;
  dec(xf);
  return bi_noFill;
}
