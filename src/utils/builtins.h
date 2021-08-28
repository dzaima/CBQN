#pragma once

#define FOR_PFN(A,M,D) \
/*   arith.c*/A(add,"+") A(sub,"-") A(mul,"×") A(div,"÷") A(pow,"⋆") A(root,"√") A(floor,"⌊") A(ceil,"⌈") A(stile,"|") A(eq,"=") \
/*   arith.c*/A(ne,"≠") D(le,"≤") D(ge,"≥") A(lt,"<") A(gt,">") A(and,"∧") A(or,"∨") A(not,"¬") A(log,"⋆⁼") \
/*     fns.c*/A(ud,"↕") A(fne,"≢") A(feq,"≡") A(ltack,"⊣") A(rtack,"⊢") A(indexOf,"⊐") A(memberOf,"∊") A(find,"⍷") A(count,"⊒") \
/*    sfns.c*/A(shape,"⥊") A(pick,"⊑") A(pair,"{𝕨‿𝕩}") A(select,"⊏") A(slash,"/") A(join,"∾") A(couple,"≍") A(shiftb,"»") A(shifta,"«") A(take,"↑") A(drop,"↓") A(group,"⊔") A(reverse,"⌽") \
/*    sort.c*/A(gradeUp,"⍋") A(gradeDown,"⍒") \
/* everything before the definition of •Type is defined to be pure, and everything after is not */ \
/*   sysfn.c*/M(type,"•Type") M(decp,"•Decompose") M(primInd,"•PrimInd") M(glyph,"•Glyph") A(fill,"•FillFn") M(sys,"•getsys") A(grLen,"•GroupLen") D(grOrd,"•groupOrd") \
/*   sysfn.c*/M(repr,"•Repr") M(fmt,"•Fmt") A(asrt,"!") A(casrt,"!") M(out,"•Out") M(show,"•Show") M(bqn,"•BQN") M(sh,"•SH") M(fromUtf8,"•FromUTF8") \
/*   sysfn.c*/D(cmp,"•Cmp") A(hash,"•Hash") M(delay,"•Delay") M(makeRand,"•MakeRand") M(reBQN,"•ReBQN") M(exit,"•Exit") M(getLine,"•GetLine") \
/*internal.c*/M(itype,"•internal.Type") M(refc,"•internal.Refc") M(squeeze,"•internal.Squeeze") M(isPure,"•internal.IsPure") A(info,"•internal.Info") \
/*internal.c*/D(variation,"•internal.Variation") A(listVariations,"•internal.ListVariations") M(clearRefs,"•internal.ClearRefs") M(unshare,"•internal.Unshare")

#define FOR_PM1(A,M,D) \
  /*md1.c*/ A(tbl,"⌜") A(each,"¨") A(fold,"´") A(scan,"`") A(const,"˙") A(swap,"˜") A(cell,"˘") \
/* everything before the definition of •_timed is defined to be pure, and everything after is not */ \
  /*md1.c*/ A(timed,"•_timed")

#define FOR_PM2(A,M,D) \
  /*md2.c*/ A(val,"⊘") A(repeat,"⍟") A(fillBy,"•_fillBy_") A(catch,"⎊") \
  /*md2.c*/ A(atop,"∘") A(over,"○") A(before,"⊸") A(after,"⟜") A(cond,"◶") A(under,"⌾")

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
static const i32 firstImpurePM2 = I32_MAX;

static inline bool isImpureBuiltin(B x) {
  if (isFun(x)) return !v(x)->extra || v(x)->extra>=firstImpurePFN;
  if (isMd1(x)) return !v(x)->extra || v(x)->extra>=firstImpurePM1;
  if (isMd2(x)) return !v(x)->extra || v(x)->extra>=firstImpurePM2;
  return false;
}


#define F(N,X) extern B bi_##N;
FOR_PFN(F,F,F)
FOR_PM1(F,F,F)
FOR_PM2(F,F,F)
#undef F
