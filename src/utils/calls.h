#pragma once


#define M1C1(M,F,  X) m1c1_unsafe(M##_c1, bi_##F,    X)
#define M1C2(M,F,W,X) m1c2_unsafe(M##_c2, bi_##F, W, X)
static inline B m1c1_unsafe(D1C1 m, B f,      B x) { Md1D d; d.f=f; return m(&d,    x); }
static inline B m1c2_unsafe(D1C2 m, B f, B w, B x) { Md1D d; d.f=f; return m(&d, w, x); }

typedef void (*CmpAAFn)(u64*, void*, void*, u64);
typedef void (*CmpASFn)(u64*, void*, u64, u64);
#define CMP_DEF(F, S) extern INIT_GLOBAL Cmp##S##Fn cmp_fns_##F##S[];
CMP_DEF(eq, AS); CMP_DEF(eq, AA);
CMP_DEF(ne, AS); CMP_DEF(ne, AA);
CMP_DEF(gt, AS); CMP_DEF(gt, AA);
CMP_DEF(ge, AS); CMP_DEF(ge, AA);
CMP_DEF(lt, AS);
CMP_DEF(le, AS);

// will write up to 8×⌈len÷64 bytes to WHERE, i.e. whole u64-s; LEN must not be 0
#define CMP_AA_FN(FN, ELT) cmp_fns_##FN##AA[ELT]
#define CMP_AS_FN(FN, ELT) cmp_fns_##FN##AS[ELT]

#define CMP_AA_CALL(FN, WHERE, WP, XP, LEN) FN(WHERE, WP, XP, LEN)
#define CMP_AS_CALL(FN, WHERE, WP, X,  LEN) FN(WHERE, WP, (X).u, LEN)

#define CMP_AA_IMM(FN, ELT, WHERE, WP, XP, LEN) CMP_AA_CALL(CMP_AA_FN(FN, ELT), WHERE, WP, XP, LEN)
#define CMP_AS_IMM(FN, ELT, WHERE, WP, X,  LEN) CMP_AS_CALL(CMP_AS_FN(FN, ELT), WHERE, WP, X, LEN)

typedef bool (*MatchFn)(void* a, void* b, ux l, u64 data);
extern INIT_GLOBAL MatchFn matchFns[];
extern INIT_GLOBAL MatchFn matchFnsR[];
extern u8 const matchFnData[];
typedef struct { MatchFn fn; u8 data; } MatchFnObj;
#define MATCH_GET( W_ELT, X_ELT) ({ u8 mfn_i_ = ((W_ELT)*8 + (X_ELT)); (MatchFnObj){.fn=matchFns [mfn_i_], .data=matchFnData[mfn_i_]}; })
#define MATCHR_GET(W_ELT, X_ELT) ({ u8 mfn_i_ = ((W_ELT)*8 + (X_ELT)); (MatchFnObj){.fn=matchFnsR[mfn_i_], .data=matchFnData[mfn_i_]}; })
#define MATCH_CALL(FN, W, X, L) (FN).fn(W, X, L, (FN).data) // check if L elements starting at a and b match; assumes L≥1

typedef bool (*RangeFn)(void* xp, i64* res, u64 len); // assumes len≥1; if x has non-integers or values with absolute value >2⋆53, will return 0 or report min<-2⋆53 or max>2⋆53; else, writes min,max in res and returns 1
extern INIT_GLOBAL RangeFn getRange_fns[el_f64+1]; // limited to ≤el_f64

typedef void (*BitSelFn)(void*,u64*,u64,u64,u64);
extern INIT_GLOBAL BitSelFn* bitselFns;
