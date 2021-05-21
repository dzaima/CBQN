typedef struct Mut {
  u8 type;
  usz ia;
  Arr* val;
  union {
    i32* ai32;
    f64* af64;
    u32* ac32;
    B* aB;
  };
} Mut;
#define MAKE_MUT(N, IA) Mut N##_val; N##_val.type = el_MAX; N##_val.ia = (IA); Mut* N = &N##_val;

void mut_to(Mut* m, u8 n) {
  u8 o = m->type;
  assert(o!=el_B);
  m->type = n;
  if (o==el_MAX) {
    switch(n) { default: UD;
      case el_i32: m->val = a(m_i32arrp(&m->ai32, m->ia)); return;
      case el_f64: m->val = a(m_f64arrp(&m->af64, m->ia)); return;
      case el_c32: m->val = a(m_c32arrp(&m->ac32, m->ia)); return;
      case el_B  :; HArr_p t = m_harrUp(          m->ia); m->val = (Arr*)t.c; m->aB = t.c->a; return;
    }
  } else {
    sprnk(m->val, 1);
    m->val->sh = &m->val->ia;
    #ifdef USE_VALGRIND
      VALGRIND_MAKE_MEM_DEFINED(m->val, mm_size((Value*)m->val)); // it's incomplete, but it's a typed array so garbage is acceptable
    #endif
    #ifdef DEBUG
      if (n==el_B && o==el_f64) { // hack to make toHArr calling f64arr_get not cry about possible sNaN floats
        usz ia = m->val->ia;
        f64* p = f64arr_ptr(tag(m->val,ARR_TAG));
        for (usz i = 0; i < ia; i++) if (!isF64(b(p[i]))) p[i] = 1.2217638442043777e161; // 0x6161616161616161 
      }
    #endif
    switch(n) { default: UD;
      case el_i32: { I32Arr* t= toI32Arr(tag(m->val, ARR_TAG)); m->val = (Arr*)t; m->ai32 = t->a; return; }
      case el_f64: { F64Arr* t= toF64Arr(tag(m->val, ARR_TAG)); m->val = (Arr*)t; m->af64 = t->a; return; }
      case el_c32: { C32Arr* t= toC32Arr(tag(m->val, ARR_TAG)); m->val = (Arr*)t; m->ac32 = t->a; return; }
      case el_B  : { HArr*   t= toHArr  (tag(m->val, ARR_TAG)); m->val = (Arr*)t; m->aB   = t->a; return; }
    }
  }
}

B mut_fv(Mut* m) { assert(m->type!=el_MAX);
  m->val->sh = &m->val->ia;
  B r = tag(m->val, ARR_TAG);
  srnk(r, 1);
  return r;
}
B mut_fp(Mut* m) { assert(m->type!=el_MAX); // has ia set
  return tag(m->val, ARR_TAG);
}
B mut_fc(Mut* m, B x) { assert(m->type!=el_MAX);
  B r = tag(m->val, ARR_TAG);
  arr_shCopy(r, x);
  return r;
}
B mut_fcd(Mut* m, B x) { assert(m->type!=el_MAX);
  B r = tag(m->val, ARR_TAG);
  arr_shCopy(r, x);
  dec(x);
  return r;
}

u8 el_or(u8 a, u8 b) {
  #define M(X) if(b==X) return a>X?a:X;
  switch (a) { default: UD;
    case el_c32: M(el_c32);            return el_B;
    case el_i32: M(el_i32); M(el_f64); return el_B;
    case el_f64: M(el_i32); M(el_f64); return el_B;
    case el_B:                         return el_B;
    case el_MAX: return b;
  }
  #undef M
}

void mut_pfree(Mut* m, usz n) { // free the first n elements
  if (m->type==el_B) harr_pfree(tag(m->val,ARR_TAG), n);
  else mm_free((Value*) m->val);
}

void mut_set(Mut* m, usz ms, B x) { // consumes x; sets m[ms] to x
  again:
  #define AGAIN(T) { mut_to(m, T); goto again; }
  switch(m->type) { default: UD;
    case el_MAX: AGAIN(isF64(x)? (q_i32(x)? el_i32 : el_f64) : (isC32(x)? el_c32 : el_B));
    
    case el_i32: {
      if (!q_i32(x)) AGAIN(isF64(x)? el_f64 : el_B);
      m->ai32[ms] = o2iu(x);
      return;
    }
    case el_c32: {
      if (!isC32(x)) AGAIN(el_B);
      m->ac32[ms] = o2cu(x);
      return;
    }
    case el_f64: {
      if (!isF64(x)) AGAIN(el_B);
      m->af64[ms] = o2fu(x);
      return;
    }
    case el_B: {
      m->aB[ms] = x;
      return;
    }
  }
  #undef AGAIN
}
void mut_setS(Mut* m, usz ms, B x) { // consumes; sets m[ms] to x, assumes the current type can store it
  switch(m->type) { default: UD;
    case el_i32: { assert(q_i32(x));
      m->ai32[ms] = o2iu(x);
      return;
    }
    case el_c32: { assert(isC32(x));
      m->ac32[ms] = o2cu(x);
      return;
    }
    case el_f64: { assert(isF64(x));
      m->af64[ms] = o2fu(x);
      return;
    }
    case el_B: {
      m->aB[ms] = x;
      return;
    }
  }
}
void mut_rm(Mut* m, usz ms) { // clears the object at position ms
  if (m->type == el_B) dec(m->aB[ms]);
}
B mut_getU(Mut* m, usz ms) {
  switch(m->type) { default: UD;
    case el_i32: return m_i32(m->ai32[ms]);
    case el_c32: return m_c32(m->ac32[ms]);
    case el_f64: return m_c32(m->af64[ms]);
    case el_B:   return m->aB[ms];
  }
}

// doesn't consume; fills m[ms…ms+l] with x
void mut_fill(Mut* m, usz ms, B x, usz l) {
  again:
  #define AGAIN(T) { mut_to(m, T); goto again; }
  switch(m->type) { default: UD;
    case el_MAX: AGAIN(isF64(x)? (q_i32(x)? el_i32 : el_f64) : (isC32(x)? el_c32 : el_B));
    
    case el_i32: {
      if (!q_i32(x)) AGAIN(isF64(x)? el_f64 : el_B);
      i32* p = m->ai32+ms;
      i32 v = o2iu(x);
      for (usz i = 0; i < l; i++) p[i] = v;
      return;
    }
    case el_c32: {
      if (!isC32(x)) AGAIN(el_B);
      u32* p = m->ac32+ms;
      u32 v = o2cu(x);
      for (usz i = 0; i < l; i++) p[i] = v;
      return;
    }
    case el_f64: {
      if (!isF64(x)) AGAIN(el_B);
      f64* p = m->af64+ms;
      f64 v = o2fu(x);
      for (usz i = 0; i < l; i++) p[i] = v;
      return;
    }
    case el_B: {
      B* p = m->aB+ms;
      for (usz i = 0; i < l; i++) p[i] = x;
      if (isVal(x)) for (usz i = 0; i < l; i++) inc(x);
      return;
    }
  }
  #undef AGAIN
}

// expects x to be an array, each position must be written to precisely once
// doesn't consume x
void mut_copy(Mut* m, usz ms, B x, usz xs, usz l) {
  assert(isArr(x));
  u8 xt = v(x)->type;
  u8 xe = ti[xt].elType;
  // printf("mut_%d[%d…%d] ← %s[%d…%d]\n", m->type, ms, ms+l, format_type(xt), xs, xs+l); fflush(stdout);
  again:
  #define AGAIN { mut_to(m, el_or(m->type, xe)); goto again; }
  // TODO try harder to not bump type
  switch(m->type) { default: UD;
    case el_MAX: AGAIN;
    
    case el_i32: {
      if (xt!=t_i32arr & xt!=t_i32slice) AGAIN;
      i32* xp = i32any_ptr(x);
      memcpy(m->ai32+ms, xp+xs, l*4);
      return;
    }
    case el_c32: {
      if (xt!=t_c32arr & xt!=t_c32slice) AGAIN;
      u32* xp = c32any_ptr(x);
      memcpy(m->ac32+ms, xp+xs, l*4);
      return;
    }
    case el_f64: {
      f64* xp;
      if (xt==t_f64arr) xp = f64arr_ptr(x);
      else if (xt==t_f64slice) xp = c(F64Slice,x)->a;
      else if (xt==t_i32arr|xt==t_i32slice) {
        i32* xp = i32any_ptr(x);
        f64* rp = m->af64+ms;
        for (usz i = 0; i < l; i++) rp[i] = xp[i+xs];
        return;
      }
      else AGAIN;
      memcpy(m->af64+ms, xp+xs, l*8);
      return;
    }
    case el_B: {
      B* mpo = m->aB+ms;
      B* xp;
      if (xt==t_harr) xp = harr_ptr(x);
      else if (xt==t_hslice) xp = c(HSlice,x)->a;
      else if (xt==t_fillarr) xp = c(FillArr,x)->a;
      else {
        BS2B xget = ti[xt].get;
        for (usz i = 0; i < l; i++) mpo[i] = xget(x,i+xs);
        return;
      }
      memcpy(mpo, xp+xs, l*sizeof(B*));
      for (usz i = 0; i < l; i++) inc(mpo[i]);
      return;
    }
  }
  #undef AGAIN
}


B vec_join(B w, B x) { // consumes both
  usz wia = a(w)->ia;
  usz xia = a(x)->ia;
  usz ria = wia+xia;
  if (v(w)->refc==1) {
    u64 wsz = mm_size(v(w));
    u8 wt = v(w)->type;
    if (wt==t_i32arr && fsizeof(I32Arr,a,i32,ria)<wsz && TI(x).elType==el_i32) {
      a(w)->ia = ria;
      memcpy(i32arr_ptr(w)+wia, i32any_ptr(x), xia*4);
      dec(x);
      return w;
    }
    if (wt==t_c32arr && fsizeof(C32Arr,a,u32,ria)<wsz && TI(x).elType==el_c32) {
      a(w)->ia = ria;
      memcpy(c32arr_ptr(w)+wia, c32any_ptr(x), xia*4);
      dec(x);
      return w;
    }
    if (wt==t_f64arr && fsizeof(F64Arr,a,f64,ria)<wsz && TI(x).elType==el_f64) { // TODO handle f64∾i32
      a(w)->ia = ria;
      memcpy(f64arr_ptr(w)+wia, f64any_ptr(x), xia*8);
      dec(x);
      return w;
    }
    if (wt==t_harr && fsizeof(HArr,a,B,ria)<wsz) {
      a(w)->ia = ria;
      B* rp = harr_ptr(w)+wia;
      u8 xt = v(x)->type;
      u8 xe = TI(x).elType;
      if (xt==t_harr | xt==t_hslice | xt==t_fillarr) {
        B* xp = xt==t_harr? harr_ptr(x) : xt==t_hslice? c(HSlice, x)->a : fillarr_ptr(x);
        memcpy(rp, xp, xia*sizeof(B));
        for (usz i = 0; i < xia; i++) inc(rp[i]);
      } else if (xe==el_i32) {
        i32* xp = i32any_ptr(x);
        for (usz i = 0; i < xia; i++) rp[i] = m_i32(xp[i]);
      } else if (xe==el_c32) {
        u32* xp = c32any_ptr(x);
        for (usz i = 0; i < xia; i++) rp[i] = m_c32(xp[i]);
      } else if (xe==el_f64) {
        f64* xp = f64any_ptr(x);
        for (usz i = 0; i < xia; i++) rp[i] = m_f64(xp[i]);
      } else {
        BS2B xget = TI(x).get;
        for (usz i = 0; i < xia; i++) rp[i] = xget(x, i);
      }
      dec(x);
      return w;
    }
  }
  MAKE_MUT(r, ria); mut_to(r, el_or(TI(w).elType, TI(x).elType));
  mut_copy(r, 0,   w, 0, wia);
  mut_copy(r, wia, x, 0, xia);
  dec(w); dec(x);
  return mut_fv(r);
}
B vec_add(B w, B x) { // consumes both
  usz wia = a(w)->ia;
  usz ria = wia+1;
  if (v(w)->refc==1) {
    u64 wsz = mm_size(v(w));
    u8 wt = v(w)->type;
    if (wt==t_i32arr && fsizeof(I32Arr,a,i32,ria)<wsz && q_i32(x)) {
      a(w)->ia = ria;
      i32arr_ptr(w)[wia] = o2iu(x);
      return w;
    }
    if (wt==t_c32arr && fsizeof(C32Arr,a,u32,ria)<wsz && isC32(x)) {
      a(w)->ia = ria;
      c32arr_ptr(w)[wia] = o2cu(x);
      return w;
    }
    if (wt==t_f64arr && fsizeof(F64Arr,a,f64,ria)<wsz && isNum(x)) {
      a(w)->ia = ria;
      f64arr_ptr(w)[wia] = o2fu(x);
      return w;
    }
    if (wt==t_harr && fsizeof(HArr,a,B,ria)<wsz) {
      a(w)->ia = ria;
      harr_ptr(w)[wia] = x;
      return w;
    }
  }
  MAKE_MUT(r, ria); mut_to(r, el_or(TI(w).elType, selfElType(x)));
  mut_copy(r, 0, w, 0, wia);
  mut_set(r, wia, x);
  dec(w);
  return mut_fv(r);
}
