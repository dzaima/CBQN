#pragma once

typedef void (*M_CopyF)(void*, usz, B, usz, usz);
typedef void (*M_FillF)(void*, usz, B, usz);
extern M_CopyF copyFns[el_MAX];
extern M_FillF fillFns[el_MAX];
#define COPY_TO(WHERE, ELT, MS, X, XS, LEN) copyFns[ELT](WHERE, MS, X, XS, LEN)
#define FILL_TO(WHERE, ELT, MS, X, LEN) fillFns[ELT](WHERE, MS, X, LEN)


typedef void (*CmpAAFn)(u64*, void*, void*, u64);
typedef void (*CmpASFn)(u64*, void*, u64, u64);
#define CMP_DEF(F, S) extern const Cmp##S##Fn cmp_fns_##F##S[];
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
