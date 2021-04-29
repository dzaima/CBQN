typedef struct Mut {
  u8 type;
  usz ia;
  Arr* val;
} Mut;
#define MAKE_MUT(N, IA) Mut N##_val; N##_val.type = el_MAX; N##_val.ia = (IA); Mut* N = &N##_val;

void mut_to(Mut* m, u8 n) {
  u8 o = m->type;
  m->type = n;
  if (o==el_MAX) {
    switch(n) { default: UD;
      case el_i32: m->val =    a(m_i32arrp(m->ia)) ; return;
      case el_f64: m->val =    a(m_f64arrp(m->ia)) ; return;
      case el_c32: m->val =    a(m_c32arrp(m->ia)) ; return;
      case el_B  : m->val = (Arr*)m_harrUp(m->ia).c; return;
    }
  } else {
    sprnk(m->val, 1);
    m->val->sh = &m->val->ia;
    switch(n) { default: UD;
      case el_i32: m->val = (Arr*)toI32Arr(tag(m->val, ARR_TAG)); return;
      case el_f64: m->val = (Arr*)toF64Arr(tag(m->val, ARR_TAG)); return;
      case el_c32: m->val = (Arr*)toC32Arr(tag(m->val, ARR_TAG)); return;
      case el_B  : m->val = (Arr*)toHArr  (tag(m->val, ARR_TAG)); return;
    }
  }
}

B mut_fv(Mut* m) {
  assert(m->type!=el_MAX);
  m->val->sh = &m->val->ia;
  B r = tag(m->val, ARR_TAG);
  srnk(r, 1);
  return r;
}
B mut_fp(Mut* m) {
  assert(m->type!=el_MAX);
  return tag(m->val, ARR_TAG);
}

u8 el_or(u8 a, u8 b) {
  #define M(X) if(b==X) return b>X?b:X;
  switch (a) {
    case el_c32: M(el_c32);            return el_B;
    case el_i32: M(el_i32); M(el_f64); return el_B;
    case el_f64: M(el_i32); M(el_f64); return el_B;
    case el_B:                         return el_B;
    case el_MAX: return b;
    default: UD;
  }
  #undef M
}

void mut_pfree(Mut* m, usz n) { // free the first n elements
  if (m->type==el_B) harr_pfree(tag(m->val,ARR_TAG), n);
  else mm_free((Value*) m->val);
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
  switch(m->type) {
    case el_MAX: AGAIN;
    
    case el_i32: {
      i32* xp;
      if (xt==t_i32arr) xp = i32arr_ptr(x);
      else if (xt==t_i32slice) xp = c(I32Slice,x)->a;
      else AGAIN;
      memcpy(((I32Arr*)m->val)->a+ms, xp+xs, l*4);
      return;
    }
    case el_c32: {
      u32* xp;
      if (xt==t_c32arr) xp = c32arr_ptr(x);
      else if (xt==t_c32slice) xp = c(C32Slice,x)->a;
      else AGAIN;
      memcpy(((C32Arr*)m->val)->a+ms, xp+xs, l*4);
      return;
    }
    case el_f64: {
      f64* xp;
      if (xt==t_f64arr) xp = f64arr_ptr(x);
      else if (xt==t_f64slice) xp = c(F64Slice,x)->a;
      else AGAIN;
      memcpy(((F64Arr*)m->val)->a+ms, xp+xs, l*8);
      return;
    }
    case el_B: {
      B* mpo = ((HArr*)m->val)->a+ms;
      B* xp;
      if (xt==t_harr) xp = harr_ptr(x);
      else if (xt==t_hslice) xp = c(HSlice,x)->a;
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