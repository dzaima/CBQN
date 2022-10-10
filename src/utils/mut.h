#pragma once

/* Usage:

Start with MAKE_MUT(name, itemAmount);
MAKE_MUT allocates the object on the stack, so everything must happen within the scope of it.
Optionally, call mut_init(name, el_something) with an appropriate ElType.
End with mut_f(v|c|cd|p);

There must be no allocations while a mut object is being built so GC doesn't do bad things.
mut_pfree must be used to free a partially finished `mut` instance safely (e.g. before throwing an error).
Methods ending with G expect that mut_init has been called with a type that can fit the elements that it'll set, and MUTG_INIT afterwards in the same scope.

*/
typedef struct Mut Mut;
typedef struct MutFns MutFns;
typedef void (*M_CopyF)(void*, usz, B, usz, usz);
typedef void (*M_FillF)(void*, usz, B, usz);
typedef void (*M_SetF)(void*, usz, B);
typedef B (*M_GetF)(Mut*, usz);
struct MutFns {
  u8 elType;
  u8 valType;
  M_CopyF m_copy, m_copyG;
  M_FillF m_fill, m_fillG;
  M_SetF  m_set,  m_setG;
  M_GetF m_getU;
};
extern MutFns mutFns[el_MAX+1];

struct Mut {
  MutFns* fns;
  usz ia;
  Arr* val;
  union {
    void* a; B* aB;
    i8* ai8; i16* ai16; i32* ai32;
    u8* ac8; u16* ac16; u32* ac32;
    f64* af64; u64* abit;
  };
};
#define MAKE_MUT(N, IA) Mut N##_val; N##_val.fns = &mutFns[el_MAX]; N##_val.ia = (IA); Mut* N = &N##_val;

static void mut_init(Mut* m, u8 n) {
  m->fns = &mutFns[n];
  usz ia = PIA(m);
  usz sz;
  // hack around inlining of the allocator too many times
  switch(n) { default: UD;
    case el_bit:              sz = BITARR_SZ(   ia); break;
    case el_i8:  case el_c8:  sz = TYARR_SZ(I8, ia); break;
    case el_i16: case el_c16: sz = TYARR_SZ(I16,ia); break;
    case el_i32: case el_c32: sz = TYARR_SZ(I32,ia); break;
    case el_f64:              sz = TYARR_SZ(F64,ia); break;
    case el_B:;
      HArr_p t = m_harrUp(ia);
      m->val = (Arr*)t.c;
      m->aB = t.c->a;
      return;
  }
  Arr* a = m_arr(sz, m->fns->valType, ia);
  m->val = a;
  m->a = ((TyArr*)a)->a;
}
void mut_to(Mut* m, u8 n);

static B mut_fv(Mut* m) { assert(m->fns->elType!=el_MAX);
  Arr* a = m->val;
  a->sh = &a->ia;
  SPRNK(a, 1);
  return taga(a);
}
static B mut_fc(Mut* m, B x) { assert(m->fns->elType!=el_MAX);
  Arr* a = m->val;
  arr_shCopy(a, x);
  return taga(a);
}
static B mut_fcd(Mut* m, B x) { assert(m->fns->elType!=el_MAX);
  Arr* a = m->val;
  arr_shCopy(a, x);
  decG(x);
  return taga(a);
}
static Arr* mut_fp(Mut* m) { assert(m->fns->elType!=el_MAX);
  return m->val;
}

extern u8 el_orArr[];
static u8 el_or(u8 a, u8 b) {
  return el_orArr[a*16 + b];
}

void mut_pfree(Mut* m, usz n);

// consumes x; sets m[ms] to x
static void mut_set(Mut* m, usz ms, B x) { m->fns->m_set(m, ms, x); }


// clears the object (decrements its refcount) at position ms
static void mut_rm(Mut* m, usz ms) { if (m->fns->elType == el_B) dec(m->aB[ms]); }

// gets object at position ms, without increasing refcount
static B mut_getU(Mut* m, usz ms) { return m->fns->m_getU(m, ms); }

// doesn't consume; fills m[ms…ms+l] with x
static void mut_fill(Mut* m, usz ms, B x, usz l) { m->fns->m_fill(m, ms, x, l); }

// expects x to be an array, each position must be written to precisely once
// doesn't consume
static void mut_copy(Mut* m, usz ms, B x, usz xs, usz l) { assert(isArr(x)); m->fns->m_copy(m, ms, x, xs, l); }


#define MUTG_INIT(N) MutFns N##_mutfns = *N->fns; void* N##_mutarr = N->a
// // mut_set but assumes the type of x already fits in m
#define mut_setG(N, ms, x) N##_mutfns.m_setG(N##_mutarr, ms, x)
// // mut_fill but assumes the type of x already fits in m
#define mut_fillG(N, ms, x, l) N##_mutfns.m_fillG(N##_mutarr, ms, x, l)
// // mut_copy but assumes the type of x already fits in m
#define mut_copyG(N, ms, x, xs, l) N##_mutfns.m_copyG(N##_mutarr, ms, x, xs, l)



static void bit_cpy(u64* r, usz rs, u64* x, usz xs, usz l) {
  u64 re = rs+(u64)l;
  i64 d = (i64)xs-(i64)rs;
  
  u64 ti = rs>>6;
  u64 ei = re>>6;
  
  u64 dp = (u64)(d>>6);
  u64 df = ((u64)d)&63u;
  #define RDFo(N) *(x + (i64)(ti+dp+N))
  #define RDFp ((RDFo(0) >> df) | (RDFo(1) << (64-df)))
  #define READ (df==0? RDFo(0) : RDFp)
  if (ti!=ei) {
    if (rs&63) {
      u64 m = (1ULL << (rs&63))-1;
      r[ti] = (READ & ~m)  |  (r[ti] & m);
      ti++;
    }
    
    if (df==0) for (; ti<ei; ti++) r[ti] = RDFo(0);
    else       for (; ti<ei; ti++) r[ti] = RDFp;
    
    if (re&63) {
      u64 m = (1ULL << (re&63))-1;
      r[ti] = (READ & m)  |  (r[ti] & ~m);
    }
  } else if (rs!=re) {
    assert(re!=0); // otherwise rs and re would be in different items, hitting the earlier ti!=ei; re!=0 is required for the mask to work
    u64 m = ((1ULL << (rs&63))-1) ^ ((1ULL << (re&63))-1);
    r[ti] = (READ & m)  |  (r[ti] & ~m);
  }
  #undef READ
  #undef RDFp
  #undef RDFo
}

B vec_join(B w, B x); // consumes both


extern M_CopyF copyFns[el_MAX];
#define COPY_TO(WHERE, ELT, MS, X, XS, LEN) copyFns[ELT](WHERE, MS, X, XS, LEN)

extern M_FillF fillFns[el_MAX];
#define FILL_TO(WHERE, ELT, MS, X, LEN) fillFns[ELT](WHERE, MS, X, LEN)

// if `consume==true`, consumes w,x and expects both args to be vectors
// else, doesn't consume x, and decrements refcount of w iif *reusedW (won't free because the result will be w)
FORCE_INLINE B arr_join_inline(B w, B x, bool consume, bool* reusedW) {
  usz wia = IA(w);
  usz xia = IA(x);
  u64 ria = wia+xia;
  if (!reusable(w)) goto no;
  u64 wsz = mm_size(v(w));
  u8 wt = TY(w);
  u8 we = TI(w, elType);
  // TODO f64∾i32, i32∾i8, c32∾c8 etc
  void* rp = tyany_ptr(w);
  switch (wt) {
    case t_bitarr: if (BITARR_SZ(   ria)<wsz && TI(x,elType)==el_bit) goto yes; break;
    case t_i8arr:  if (TYARR_SZ(I8, ria)<wsz && TI(x,elType)<=el_i8 ) goto yes; break;
    case t_i16arr: if (TYARR_SZ(I16,ria)<wsz && TI(x,elType)<=el_i16) goto yes; break;
    case t_i32arr: if (TYARR_SZ(I32,ria)<wsz && TI(x,elType)<=el_i32) goto yes; break;
    case t_f64arr: if (TYARR_SZ(F64,ria)<wsz && TI(x,elType)<=el_f64) goto yes; break;
    case t_c8arr:  if (TYARR_SZ(C8, ria)<wsz && TI(x,elType)==el_c8 ) goto yes; break;
    case t_c16arr: if (TYARR_SZ(C16,ria)<wsz && TI(x,elType)<=el_c16 && TI(x,elType)>=el_c8) goto yes; break;
    case t_c32arr: if (TYARR_SZ(C32,ria)<wsz && TI(x,elType)<=el_c32 && TI(x,elType)>=el_c8) goto yes; break;
    case t_harr: if (fsizeof(HArr,a,B,ria)<wsz) { rp = harr_ptr(w); goto yes; } break;
  }
  no:; // failed to reuse
  MAKE_MUT(r, ria); mut_init(r, el_or(TI(w,elType), TI(x,elType)));
  MUTG_INIT(r);
  mut_copyG(r, 0,   w, 0, wia);
  mut_copyG(r, wia, x, 0, xia);
  if (consume) { decG(x); decG(w); }
  *reusedW = false;
  return mut_fv(r);
  
  yes:
  COPY_TO(rp, we, wia, x, 0, xia);
  if (consume) decG(x);
  *reusedW = true;
  a(w)->ia = ria;
  return FL_KEEP(w,fl_squoze); // keeping fl_squoze as appending items can't make the largest item smaller
}

static inline bool inplace_add(B w, B x) { // consumes x if returns true; fails if fills wouldn't be correct
  usz wia = IA(w);
  usz ria = wia+1;
  if (reusable(w)) {
    u64 wsz = mm_size(v(w));
    u8 wt = TY(w);
    switch (wt) {
      case t_bitarr: if (BITARR_SZ(   ria)<wsz && q_bit(x)) { bitp_set(bitarr_ptr(w),wia,o2bG(x)); goto ok; } break;
      case t_i8arr:  if (TYARR_SZ(I8, ria)<wsz && q_i8 (x)) { i8arr_ptr (w)[wia]=o2iG(x); goto ok; } break;
      case t_i16arr: if (TYARR_SZ(I16,ria)<wsz && q_i16(x)) { i16arr_ptr(w)[wia]=o2iG(x); goto ok; } break;
      case t_i32arr: if (TYARR_SZ(I32,ria)<wsz && q_i32(x)) { i32arr_ptr(w)[wia]=o2iG(x); goto ok; } break;
      case t_c8arr:  if (TYARR_SZ(C8, ria)<wsz && q_c8 (x)) { c8arr_ptr (w)[wia]=o2cG(x); goto ok; } break;
      case t_c16arr: if (TYARR_SZ(C16,ria)<wsz && q_c16(x)) { c16arr_ptr(w)[wia]=o2cG(x); goto ok; } break;
      case t_c32arr: if (TYARR_SZ(C32,ria)<wsz && q_c32(x)) { c32arr_ptr(w)[wia]=o2cG(x); goto ok; } break;
      case t_f64arr: if (TYARR_SZ(F64,ria)<wsz && q_f64(x)) { f64arr_ptr(w)[wia]=o2fG(x); goto ok; } break;
      case t_harr: if (fsizeof(HArr,a,B,ria)<wsz) {
        harr_ptr(w)[wia] = x;
        goto ok;
      } break;
    }
  }
  return false;
  ok:
  a(FL_KEEP(w,fl_squoze))->ia = ria;
  return true;
}
B vec_addF(B w, B x);
B vec_addN(B w, B x); // vec_add but not inlined
static B vec_add(B w, B x) { // consumes both; fills may be wrong
  if (inplace_add(w, x)) return w;
  return vec_addF(w, x);
}
