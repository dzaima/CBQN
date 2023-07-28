#pragma once


#define M1C1(M,F,  X) m1c1_unsafe(M##_c1, bi_##F,    X)
#define M1C2(M,F,W,X) m1c2_unsafe(M##_c2, bi_##F, W, X)
static inline B m1c1_unsafe(D1C1 m, B f,      B x) { Md1D d; d.f=f; return m(&d,    x); }
static inline B m1c2_unsafe(D1C2 m, B f, B w, B x) { Md1D d; d.f=f; return m(&d, w, x); }

typedef void (*CmpAAFn)(u64*, void*, void*, u64);
typedef void (*CmpASFn)(u64*, void*, u64, u64);
#define CMP_DEF(F, S) extern Cmp##S##Fn cmp_fns_##F##S[];
CMP_DEF(eq, AS); CMP_DEF(eq, AA);
CMP_DEF(ne, AS); CMP_DEF(ne, AA);
CMP_DEF(gt, AS); CMP_DEF(gt, AA);
CMP_DEF(ge, AS); CMP_DEF(ge, AA);
CMP_DEF(lt, AS);
CMP_DEF(le, AS);

// will write up to 8×⌈len÷64 bytes to WHERE, i.e. whole u64-s
#define CMP_AA_FN(FN, ELT) cmp_fns_##FN##AA[ELT]
#define CMP_AS_FN(FN, ELT) cmp_fns_##FN##AS[ELT]

#define CMP_AA_CALL(FN, WHERE, WP, XP, LEN) FN(WHERE, WP, XP, LEN)
#define CMP_AS_CALL(FN, WHERE, WP, X,  LEN) FN(WHERE, WP, (X).u, LEN)

#define CMP_AA_IMM(FN, ELT, WHERE, WP, XP, LEN) CMP_AA_CALL(CMP_AA_FN(FN, ELT), WHERE, WP, XP, LEN)
#define CMP_AS_IMM(FN, ELT, WHERE, WP, X,  LEN) CMP_AS_CALL(CMP_AS_FN(FN, ELT), WHERE, WP, X, LEN)

// Check if the l elements starting at a and b match
typedef bool (*EqFn)(void* a, void* b, u64 l, u64 data);
extern EqFn eqFns[];
extern u8 eqFnData[];
#define EQFN_INDEX(W_ELT, X_ELT) ((W_ELT)*8 + (X_ELT))
typedef struct { EqFn fn; u8 data; } EqFnObj;
#define EQFN_GET(W_ELT, X_ELT) ({ u8 eqfn_i_ = EQFN_INDEX(W_ELT, X_ELT); (EqFnObj){.fn=eqFns[eqfn_i_], .data=eqFnData[eqfn_i_]}; })
#define EQFN_CALL(FN, W, X, L) (FN).fn(W, X, L, (FN).data)

typedef bool (*RangeFn)(void* xp, i64* res, u64 len); // writes min,max in res, assumes len≥1; returns 0 and leaves res undefined if either any (floor(x)≠x or abs>2⋆53), or (x≠(i64)x)
extern RangeFn getRange_fns[el_f64+1]; // limited to ≤el_f64
