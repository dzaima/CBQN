#pragma once

/* Usage:

Start with MAKE_MUT(name, itemAmount);
MAKE_MUT allocates the object on the stack, so everything must happen within the scope of it.
Optionally, call mut_init(name, el_something) with an appropriate ElType
End with mut_f(v|c|cd|p);

There must be no allocations while a mut object is being built so GC doesn't do bad things.
mut_pfree must be used to free a partially finished `mut` instance safely (e.g. before throwing an error)
methods ending with G expect that mut_init has been called with a type that can fit the elements that it'll set

*/

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

static void mut_init(Mut* m, u8 n) {
  m->type = n;
  usz ia = m->ia;
  u8 ty;
  usz sz;
  // hack around inlining of the allocator too many times
  switch(n) { default: UD;
    case el_i32: ty = t_i32arr; sz = I32A_SZ(ia); break;
    case el_f64: ty = t_f64arr; sz = F64A_SZ(ia); break;
    case el_c32: ty = t_c32arr; sz = C32A_SZ(ia); break;
    case el_B:;
      HArr_p t = m_harrUp(ia);
      m->val = (Arr*)t.c;
      m->aB = t.c->a;
      return;
  }
  Arr* a = m_arr(sz, ty, ia);
  m->val = a;
  switch(n) { default: UD; // gcc generates horrible code for this (which should just be two instructions), but that's what gcc does
    case el_i32: m->ai32 = ((I32Arr*)a)->a; break;
    case el_f64: m->af64 = ((F64Arr*)a)->a; break;
    case el_c32: m->ac32 = ((C32Arr*)a)->a; break;
  }
}
void mut_to(Mut* m, u8 n);

static B mut_fv(Mut* m) { assert(m->type!=el_MAX);
  m->val->sh = &m->val->ia;
  B r = taga(m->val);
  srnk(r, 1);
  return r;
}
static B mut_fc(Mut* m, B x) { assert(m->type!=el_MAX);
  Arr* a = m->val;
  arr_shCopy(a, x);
  return taga(a);
}
static B mut_fcd(Mut* m, B x) { assert(m->type!=el_MAX);
  Arr* a = m->val;
  arr_shCopy(a, x);
  dec(x);
  return taga(a);
}
static Arr* mut_fp(Mut* m) { assert(m->type!=el_MAX);
  return m->val;
}

static u8 el_or(u8 a, u8 b) {
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

void mut_pfree(Mut* m, usz n);

static void mut_set(Mut* m, usz ms, B x) { // consumes x; sets m[ms] to x
  again:;
  u8 nty;
  switch(m->type) { default: UD;
    case el_MAX:
      nty = isF64(x)? (q_i32(x)? el_i32 : el_f64) : (isC32(x)? el_c32 : el_B);
      goto change;
    
    case el_i32: {
      if (!q_i32(x)) { nty = isF64(x)? el_f64 : el_B; goto change; }
      m->ai32[ms] = o2iu(x);
      return;
    }
    case el_c32: {
      if (!isC32(x)) { nty = el_B; goto change; }
      m->ac32[ms] = o2cu(x);
      return;
    }
    case el_f64: {
      if (!isF64(x)) { nty = el_B; goto change; }
      m->af64[ms] = o2fu(x);
      return;
    }
    case el_B: {
      m->aB[ms] = x;
      return;
    }
  }
  change:
  mut_to(m, nty);
  goto again;
}
static void mut_setG(Mut* m, usz ms, B x) { // consumes; sets m[ms] to x, assumes the current type can store it
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
static void mut_rm(Mut* m, usz ms) { // clears the object at position ms
  if (m->type == el_B) dec(m->aB[ms]);
}
static B mut_getU(Mut* m, usz ms) {
  switch(m->type) { default: UD;
    case el_i32: return m_i32(m->ai32[ms]);
    case el_c32: return m_c32(m->ac32[ms]);
    case el_f64: return m_f64(m->af64[ms]);
    case el_B:   return m->aB[ms];
  }
}

// doesn't consume; fills m[ms…ms+l] with x
static void mut_fill(Mut* m, usz ms, B x, usz l) {
  again:;
  u8 nty;
  switch(m->type) { default: UD;
    case el_MAX:
      nty = isF64(x)? (q_i32(x)? el_i32 : el_f64) : (isC32(x)? el_c32 : el_B);
      goto change;
    
    case el_i32: {
      if (RARE(!q_i32(x))) { nty = isF64(x)? el_f64 : el_B; goto change; }
      i32* p = m->ai32+ms;
      i32 v = o2iu(x);
      for (usz i = 0; i < l; i++) p[i] = v;
      return;
    }
    case el_c32: {
      if (RARE(!isC32(x))) { nty = el_B; goto change; }
      u32* p = m->ac32+ms;
      u32 v = o2cu(x);
      for (usz i = 0; i < l; i++) p[i] = v;
      return;
    }
    case el_f64: {
      if (RARE(!isF64(x))) { nty = el_B; goto change; }
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
  change:
  mut_to(m, nty);
  goto again;
}
static void mut_fillG(Mut* m, usz ms, B x, usz l) {
  switch(m->type) { default: UD;
    case el_i32: {
      assert(q_i32(x));
      i32* p = m->ai32+ms;
      i32 v = o2iu(x);
      for (usz i = 0; i < l; i++) p[i] = v;
      return;
    }
    case el_c32: {
      assert(isC32(x));
      u32* p = m->ac32+ms;
      u32 v = o2cu(x);
      for (usz i = 0; i < l; i++) p[i] = v;
      return;
    }
    case el_f64: {
      assert(isF64(x));
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
}

// expects x to be an array, each position must be written to precisely once
// doesn't consume x
static void mut_copy(Mut* m, usz ms, B x, usz xs, usz l) { // TODO try harder to not bump type
  assert(isArr(x));
  u8 xt = v(x)->type;
  u8 xe = TIi(xt,elType);
  // printf("mut_%d[%d…%d] ← %s[%d…%d]\n", m->type, ms, ms+l, format_type(xt), xs, xs+l); fflush(stdout);
  again:
  switch(m->type) { default: UD;
    case el_MAX: goto change;
    
    case el_i32: {
      if (RARE(xt!=t_i32arr & xt!=t_i32slice)) goto change;
      i32* xp = i32any_ptr(x);
      memcpy(m->ai32+ms, xp+xs, l*4);
      return;
    }
    case el_c32: {
      if (RARE(xt!=t_c32arr & xt!=t_c32slice)) goto change;
      u32* xp = c32any_ptr(x);
      memcpy(m->ac32+ms, xp+xs, l*4);
      return;
    }
    case el_f64: {
      f64* xp;
      if (xt==t_f64arr) xp = f64arr_ptr(x);
      else if (xt==t_f64slice) xp = c(F64Slice,x)->a;
      else if (LIKELY(xt==t_i32arr|xt==t_i32slice)) {
        i32* xp = i32any_ptr(x);
        f64* rp = m->af64+ms;
        for (usz i = 0; i < l; i++) rp[i] = xp[i+xs];
        return;
      }
      else goto change;
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
        BS2B xget = TIi(xt,get);
        for (usz i = 0; i < l; i++) mpo[i] = xget(x,i+xs);
        return;
      }
      memcpy(mpo, xp+xs, l*sizeof(B*));
      for (usz i = 0; i < l; i++) inc(mpo[i]);
      return;
    }
  }
  change:
  mut_to(m, el_or(m->type, xe));
  goto again;
}
static void mut_copyG(Mut* m, usz ms, B x, usz xs, usz l) { // mut_copy but x is guaranteed to be a subtype of m
  assert(isArr(x));
  u8 xt = v(x)->type;
  switch(m->type) { default: UD;
    case el_i32: {
      i32* xp = i32any_ptr(x);
      memcpy(m->ai32+ms, xp+xs, l*4);
      return;
    }
    case el_c32: {
      u32* xp = c32any_ptr(x);
      memcpy(m->ac32+ms, xp+xs, l*4);
      return;
    }
    case el_f64: {
      f64* xp;
      if (xt==t_f64arr) xp = f64arr_ptr(x);
      else if (xt==t_f64slice) xp = c(F64Slice,x)->a;
      else {
        assert(TIi(xt,elType)==el_i32);
        i32* xp = i32any_ptr(x);
        f64* rp = m->af64+ms;
        for (usz i = 0; i < l; i++) rp[i] = xp[i+xs];
        return;
      }
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
        BS2B xget = TIi(xt,get);
        for (usz i = 0; i < l; i++) mpo[i] = xget(x,i+xs);
        return;
      }
      memcpy(mpo, xp+xs, l*sizeof(B*));
      for (usz i = 0; i < l; i++) inc(mpo[i]);
      return;
    }
  }
}


static B vec_join(B w, B x) { // consumes both
  usz wia = a(w)->ia;
  usz xia = a(x)->ia;
  usz ria = wia+xia;
  if (v(w)->refc==1) {
    u64 wsz = mm_size(v(w));
    u8 wt = v(w)->type;
    if (wt==t_i32arr && fsizeof(I32Arr,a,i32,ria)<wsz && TI(x,elType)==el_i32) {
      a(w)->ia = ria;
      memcpy(i32arr_ptr(w)+wia, i32any_ptr(x), xia*4);
      dec(x);
      return w;
    }
    if (wt==t_c32arr && fsizeof(C32Arr,a,u32,ria)<wsz && TI(x,elType)==el_c32) {
      a(w)->ia = ria;
      memcpy(c32arr_ptr(w)+wia, c32any_ptr(x), xia*4);
      dec(x);
      return w;
    }
    if (wt==t_f64arr && fsizeof(F64Arr,a,f64,ria)<wsz && TI(x,elType)==el_f64) { // TODO handle f64∾i32
      a(w)->ia = ria;
      memcpy(f64arr_ptr(w)+wia, f64any_ptr(x), xia*8);
      dec(x);
      return w;
    }
    if (wt==t_harr && fsizeof(HArr,a,B,ria)<wsz) {
      a(w)->ia = ria;
      B* rp = harr_ptr(w)+wia;
      u8 xt = v(x)->type;
      u8 xe = TI(x,elType);
      if (xt==t_harr | xt==t_hslice | xt==t_fillarr) {
        B* xp = xt==t_harr? harr_ptr(x) : xt==t_hslice? c(HSlice, x)->a : fillarr_ptr(a(x));
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
        BS2B xget = TI(x,get);
        for (usz i = 0; i < xia; i++) rp[i] = xget(x, i);
      }
      dec(x);
      return w;
    }
  }
  MAKE_MUT(r, ria); mut_init(r, el_or(TI(w,elType), TI(x,elType)));
  mut_copyG(r, 0,   w, 0, wia);
  mut_copyG(r, wia, x, 0, xia);
  dec(w); dec(x);
  return mut_fv(r);
}
static inline bool inplace_add(B w, B x) { // fails if fills wouldn't be correct
  usz wia = a(w)->ia;
  usz ria = wia+1;
  if (v(w)->refc==1) {
    u64 wsz = mm_size(v(w));
    u8 wt = v(w)->type;
    if (wt==t_i32arr && fsizeof(I32Arr,a,i32,ria)<wsz && q_i32(x)) {
      a(w)->ia = ria;
      i32arr_ptr(w)[wia] = o2iu(x);
      return true;
    }
    if (wt==t_c32arr && fsizeof(C32Arr,a,u32,ria)<wsz && isC32(x)) {
      a(w)->ia = ria;
      c32arr_ptr(w)[wia] = o2cu(x);
      return true;
    }
    if (wt==t_f64arr && fsizeof(F64Arr,a,f64,ria)<wsz && isNum(x)) {
      a(w)->ia = ria;
      f64arr_ptr(w)[wia] = o2fu(x);
      return true;
    }
    if (wt==t_harr && fsizeof(HArr,a,B,ria)<wsz) {
      a(w)->ia = ria;
      harr_ptr(w)[wia] = x;
      return true;
    }
  }
  return false;
}
B vec_addR(B w, B x);
static B vec_add(B w, B x) { // consumes both; fills may be wrong
  if (inplace_add(w, x)) return w;
  return vec_addR(w, x);
}
