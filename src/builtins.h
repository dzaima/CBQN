#pragma once

#define FOR_PFN(A,M,D) \
/* arith    */A(add,"+") A(sub,"-") A(mul,"×") A(div,"÷") A(pow,"⋆") A(root,"√") A(floor,"⌊") A(ceil,"⌈") A(stile,"|") A(eq,"=") \
/* arith    */A(ne,"≠") D(le,"≤") D(ge,"≥") A(lt,"<") A(gt,">") A(and,"∧") A(or,"∨") A(not,"¬") A(log,"⋆⁼") \
/*     fns.c*/A(ud,"↕") A(fne,"≢") A(feq,"≡") A(ltack,"⊣") A(rtack,"⊢") A(indexOf,"⊐") A(memberOf,"∊") A(find,"⍷") A(count,"⊒") \
/*    sfns.c*/A(shape,"⥊") A(pick,"⊑") A(pair,"⋈") A(select,"⊏") A(slash,"/") A(join,"∾") A(couple,"≍") A(shiftb,"»") \
/*    sfns.c*/A(shifta,"«") A(take,"↑") A(drop,"↓") A(group,"⊔") A(reverse,"⌽") A(transp,"⍉") \
/*    sort.c*/A(gradeUp,"⍋") A(gradeDown,"⍒") \
/* everything before the definition of •Type is defined to be pure, and everything after is not */ \
/*   sysfn.c*/M(type,"•Type") M(decp,"•Decompose") M(primInd,"•PrimInd") M(glyph,"•Glyph") A(fill,"•FillFn") M(sys,"•getsys") A(grLen,"•GroupLen") D(grOrd,"•GroupOrd") \
/*   sysfn.c*/M(repr,"•Repr") M(fmt,"•Fmt") A(asrt,"!") A(casrt,"!") M(out,"•Out") M(show,"•Show") A(bqn,"•BQN") A(sh,"•SH") M(fromUtf8,"•FromUTF8") \
/*   sysfn.c*/D(cmp,"•Cmp") A(hash,"•Hash") M(unixTime,"•UnixTime") M(monoTime,"•MonoTime") M(delay,"•Delay") M(makeRand,"•MakeRand") M(reBQN,"•ReBQN") M(exit,"•Exit") M(getLine,"•GetLine") \
/* inverse.c*/M(setInvReg, "(SetInvReg)") M(setInvSwap, "(SetInvSwap)") M(nativeInvReg, "(NativeInvReg)") M(nativeInvSwap, "(NativeInvSwap)") \
/*internal.c*/M(itype,"•internal.Type") M(elType,"•internal.ElType") M(refc,"•internal.Refc") M(isPure,"•internal.IsPure") A(info,"•internal.Info") \
/*internal.c*/M(squeeze,"•internal.Squeeze") M(deepSqueeze,"•internal.DeepSqueeze") \
/*internal.c*/D(variation,"•internal.Variation") A(listVariations,"•internal.ListVariations") M(clearRefs,"•internal.ClearRefs") M(unshare,"•internal.Unshare") \
/*  arithm.c*/M(sin,"•math.Sin") M(cos,"•math.Cos") M(tan,"•math.Tan") M(asin,"•math.Asin") M(acos,"•math.Acos") M(atan,"•math.Atan")

#define FOR_PM1(A,M,D) \
    /*md1.c*/A(tbl,"⌜") A(each,"¨") A(fold,"´") A(scan,"`") A(const,"˙") A(swap,"˜") A(cell,"˘") A(insert,"˝") \
/*inverse.c*/A(undo,"⁼") \
/* everything before the definition of •_timed is defined to be pure, and everything after is not */ \
    /*md1.c*/A(timed,"•_timed")

#define FOR_PM2(A,M,D) \
  /*md2.c*/A(val,"⊘") A(repeat,"⍟") A(rank,"⎉") A(depth,"⚇") A(fillBy,"•_fillBy_") A(catch,"⎊") \
  /*md2.c*/A(atop,"∘") A(over,"○") A(before,"⊸") A(after,"⟜") A(cond,"◶") A(under,"⌾") \
/* everything before the definition of •_while_ is defined to be pure, and everything after is not */ \
  /*md2.c*/A(while,"•_while_")

enum PrimNumbers {
    /* +-×÷⋆√⌊⌈|¬  */ n_add     , n_sub    , n_mul   , n_div  , n_pow    , n_root     , n_floor , n_ceil , n_stile  , n_not,
    /* ∧∨<>≠=≤≥≡≢  */ n_and     , n_or     , n_lt    , n_gt   , n_ne     , n_eq       , n_le    , n_ge   , n_feq    , n_fne,
    /* ⊣⊢⥊∾≍⋈↑↓↕«  */ n_ltack   , n_rtack  , n_shape , n_join , n_couple , n_pair     , n_take  , n_drop , n_ud     , n_shifta,
    /* »⌽⍉/⍋⍒⊏⊑⊐⊒  */ n_shiftb  , n_reverse, n_transp, n_slash, n_gradeUp, n_gradeDown, n_select, n_pick , n_indexOf, n_count,
    /* ∊⍷⊔!˙˜˘¨⌜⁼  */ n_memberOf, n_find   , n_group , n_asrt , n_const  , n_swap     , n_cell  , n_each , n_tbl    , n_undo,
    /* ´˝`∘○⊸⟜⌾⊘◶  */ n_fold    , n_insert , n_scan  , n_atop , n_over   , n_before   , n_after , n_under, n_val    , n_cond,
    /* ⎉⚇⍟⎊        */ n_rank    , n_depth  , n_repeat, n_catch
};
extern BB2B rt_invFnRegFn;
extern BB2B rt_invFnSwapFn;


#ifdef RT_WRAP
#define Q_BI(X, T) ({ B x_ = (X); isFun(x_) && v(x_)->flags-1 == n_##T; })
#else
#define Q_BI(X, T) ((X).u == bi_##T.u)
#endif

enum PrimFns { pf_none,
  #define F(N,X) pf_##N,
  FOR_PFN(F,F,F)
  #undef F
};
enum PrimMd1 { pm1_none,
  #define F(N,X) pm1_##N,
  FOR_PM1(F,F,F)
  #undef F
};
enum PrimMd2 { pm2_none,
  #define F(N,X) pm2_##N,
  FOR_PM2(F,F,F)
  #undef F
};
static const i32 firstImpurePFN = pf_type;
static const i32 firstImpurePM1 = pm1_timed;
static const i32 firstImpurePM2 = pm2_while;

static inline bool isImpureBuiltin(B x) {
  if (isFun(x)) return !v(x)->extra || v(x)->extra>=firstImpurePFN;
  if (isMd1(x)) return !v(x)->extra || v(x)->extra>=firstImpurePM1;
  if (isMd2(x)) return !v(x)->extra || v(x)->extra>=firstImpurePM2;
  return false;
}

extern B
#define F(N,X) bi_##N,
FOR_PFN(F,F,F)
FOR_PM1(F,F,F)
FOR_PM2(F,F,F)
#undef F
rt_invFnReg, rt_invFnSwap;