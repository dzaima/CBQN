typedef struct FillArr {
  struct Arr;
  B fill;
  B a[];
} FillArr;
typedef struct FillSlice {
  struct Slice;
  B* a;
} FillSlice;

B asFill(B x); // consumes
void validateFill(B x);
bool fillEqual(B w, B x);
B withFill(B x, B fill); // consumes both
static B qWithFill(B x, B fill) { // consumes both
  if (noFill(fill)) return x;
  return withFill(x, fill);
}


static B getFillR(B x) { // doesn't consume; can return bi_noFill
  if (isArr(x)) {
    u8 t = v(x)->type;
    if (t==t_fillarr  ) { B r = inc(c(FillArr,x            )->fill); return r; }
    if (t==t_fillslice) { B r = inc(c(FillArr,c(Slice,x)->p)->fill); return r; }
    if (t==t_c32arr || t==t_c32slice) return m_c32(' ');
    if (t==t_i32arr || t==t_i32slice) return m_f64(0  );
    if (t==t_f64arr || t==t_f64slice) return m_f64(0  );
    return bi_noFill;
  }
  if (isF64(x)|isI32(x)) return m_i32(0);
  if (isC32(x)) return m_c32(' ');
  return bi_noFill;
}
static B getFillQ(B x) { // doesn't consume; can return bi_noFill if CATCH_ERRORS
  B r = getFillR(x);
  #ifdef CATCH_ERRORS
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


static B m_fillarrp(usz ia) { // doesn't set ia
  return m_arr(fsizeof(FillArr,a,B,ia), t_fillarr);
}
static void fillarr_setFill(B x, B fill) { // consumes fill
  c(FillArr, x)->fill = fill;
}

static B m_fillslice(B p, B* ptr) {
  FillSlice* r = mm_allocN(sizeof(FillSlice), t_fillslice);
  r->p = p;
  r->a = ptr;
  return tag(r, ARR_TAG);
}

static B* fillarr_ptr(B x) { VTY(x,t_fillarr); return c(FillArr,x)->a; }


static B m_unit(B x) {
  B xf = asFill(inc(x));
  if (noFill(xf)) {
    HArr_p r = m_harrUp(1);
    arr_shAllocR(r.b, 0);
    r.a[0] = x;
    return r.b;
  }
  B r = m_arr(fsizeof(FillArr,a,B,1), t_fillarr);
  arr_shAllocI(r, 1, 0);
  c(FillArr,r)->fill = xf;
  c(FillArr,r)->a[0] = x;
  return r;
}

static B m_atomUnit(B x) {
  if (isNum(x)) {
    B r;
    if (q_i32(x)) { i32* rp; r=m_i32arrp(&rp, 1); rp[0] = o2iu(x); }
    else          { f64* rp; r=m_f64arrp(&rp, 1); rp[0] = o2fu(x); }
    arr_shAllocR(r,0);
    return r;
  }
  if (isC32(x)) {
    u32* rp; B r = m_c32arrp(&rp, 1);
    rp[0] = o2cu(x);
    arr_shAllocR(r,0);
    return r;
  }
  return m_unit(x);
}

static B fill_or(B wf, B xf) { // consumes
  if (fillEqual(wf, xf)) {
    dec(wf);
    return xf;
  }
  dec(wf); dec(xf);
  return bi_noFill;
}

static B fill_both(B w, B x) { // doesn't consume
  B wf = getFillQ(w);
  if (noFill(wf)) return bi_noFill;
  B xf = getFillQ(x);
  return fill_or(wf, xf);
}
