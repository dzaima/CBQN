#pragma once

/* Usage:

Start with MAKE_MUT(name, itemAmount) or MAKE_MUT_INIT(name, itemAmount, elType).
Those allocate the Mut instance on the stack, so it must be finished before the scope ends.
MAKE_MUT doesn't allocate any backing array, and will do so during mut_(set|fill|copy) (one of which must be the next operation on the Mut).
MAKE_MUT_INIT makes a backing array with the specified element type; mut_(set|fill|copy) can still widen it, but the primary use-case is for the G-postfixed functions.

End with mut_f(v|c|cd|p);.

There must be no allocations while a mut object is being built so GC doesn't read the partially-initialized object.
mut_pfree must be used to free a partially filled `mut` instance safely (e.g. before throwing an error).

*/
typedef struct Mut Mut;
typedef struct MutFns MutFns;
typedef void (*M_SetF)(void*, usz, B);
typedef B (*M_GetF)(void*, usz);
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
  void* a;
};

void mut_to(Mut* m, u8 n);
#if __clang__ && __has_attribute(noescape) // workaround for clang not realizing that stack-returned structs aren't captured
  void make_mut_init(__attribute__((noescape)) Mut* rp, ux ia, u8 el);
  #define MAKE_MUT_INIT(N, IA, EL) Mut N##_val; Mut* N = &N##_val; make_mut_init(N, IA, EL);
#else
  Mut make_mut_init(ux ia, u8 el);
  #define MAKE_MUT_INIT(N, IA, EL) Mut N##_val = make_mut_init(IA, EL); Mut* N = &N##_val;
#endif
#define MAKE_MUT(N, IA) Mut N##_val; N##_val.fns = &mutFns[el_MAX]; N##_val.ia = (IA); Mut* N = &N##_val;

static B mut_fv(Mut* m) { assert(m->fns->elType!=el_MAX);
  NOGC_E; assert(m->ia == m->val->ia);
  Arr* a = m->val;
  a->sh = &a->ia;
  SPRNK(a, 1);
  return taga(a);
}
static B mut_fc(Mut* m, B x) { assert(m->fns->elType!=el_MAX); // doesn't consume x
  NOGC_E; assert(m->ia == m->val->ia);
  Arr* a = m->val;
  arr_shCopy(a, x);
  return taga(a);
}
static B mut_fcd(Mut* m, B x) { assert(m->fns->elType!=el_MAX); // consumes x
  NOGC_E; assert(m->ia == m->val->ia);
  Arr* a = m->val;
  arr_shCopy(a, x);
  decG(x);
  return taga(a);
}
static Arr* mut_fp(Mut* m) { assert(m->fns->elType!=el_MAX);
  NOGC_E; assert(m->ia == m->val->ia);
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
static void mut_rm(Mut* m, usz ms) { if (m->fns->elType == el_B) dec(((B*)m->a)[ms]); }

// gets object at position ms, without increasing refcount
static B mut_getU(Mut* m, usz ms) { return m->fns->m_getU(m->a, ms); }

// doesn't consume; fills m[ms…ms+l] with x
static void mut_fill(Mut* m, usz ms, B x, usz l) { m->fns->m_fill(m, ms, x, l); }

// doesn't consume; expects x to be an array, each position must be written to precisely once
static void mut_copy(Mut* m, usz ms, B x, usz xs, usz l) { assert(isArr(x)); m->fns->m_copy(m, ms, x, xs, l); }

// MUT_APPEND_INIT must be called immediately after MAKE_MUT or MAKE_MUT_INIT
// after that, the only valid operation on the Mut will be MUT_APPEND
// using this append system will no longer prevent allocations from being done during the lifetime of the Mut
#define MUT_APPEND_INIT(N) ux N##_ci = 0; NOGC_E;
#define MUT_APPEND(N, X, XS, L) ({ ux l_ = (L); NOGC_CHECK; \
  mut_copy(N, N##_ci, X, XS, l_);  \
  N##_ci+= l_;                     \
  if (PTY(N->val) == t_harr) { NOGC_E; N->val->ia = N##_ci; } \
})

#define MUTG_INIT(N) MutFns N##_mutfns = *N->fns; void* N##_mutarr = N->a
// these methods function as the non-G-postfixed ones, except that
// the MAKE_MUT_INIT must have been used, MUTG_INIT called, and x must fit into the array type initialized to
#define mut_setG(N, ms, x) N##_mutfns.m_setG(N##_mutarr, ms, x)
#define mut_fillG(N, ms, x, l) N##_mutfns.m_fillG(N##_mutarr, ms, x, l)
#define mut_copyG(N, ms, x, xs, l) N##_mutfns.m_copyG(N##_mutarr, ms, x, xs, l)


// Companion to bit_cpy when uniform syntax is wanted
#define MEM_CPY(R,RI,X,XI,L) memcpy((u8*)(R)+(RI), (u8*)(X)+(XI), (L))

void bit_cpyN(u64* r, usz rs, u64* x, usz xs, usz l);
SHOULD_INLINE void bit_cpy(u64* r, usz rs, u64* x, usz xs, usz l) {
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



// if `consume==true`, consumes w,x and expects both args to be vectors
// else, doesn't consume x, and decrements refcount of w iif *reusedW (won't free because the result will be w)
FORCE_INLINE B arr_join_inline(B w, B x, bool consume, bool* reusedW) {
  usz wia = IA(w);
  usz xia = IA(x);
  u64 ria = wia+xia;
  if (!reusable(w)) goto no;
  u64 wsz = mm_sizeUsable(v(w));
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
  MAKE_MUT_INIT(r, ria, el_or(TI(w,elType), TI(x,elType)));
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
    u64 wsz = mm_sizeUsable(v(w));
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
static B vec_add(B w, B x) { // consumes both; fills may be wrong
  if (inplace_add(w, x)) return w;
  return vec_addF(w, x);
}
