#pragma once

#define FOR_PFN(A,M,D) \
/*   arith.c*/A(add,"+") A(sub,"-") A(mul,"×") A(div,"÷") A(pow,"⋆") A(root,"√") A(floor,"⌊") A(ceil,"⌈") A(stile,"|") A(eq,"=") \
/*   arith.c*/A(ne,"≠") D(le,"≤") D(ge,"≥") A(lt,"<") A(gt,">") A(and,"∧") A(or,"∨") A(not,"¬") A(log,"⋆⁼") \
/*     fns.c*/A(ud,"↕") A(fne,"≢") A(feq,"≡") A(ltack,"⊣") A(rtack,"⊢") M(fmtF,"•FmtF") A(indexOf,"⊐") A(memberOf,"∊") A(find,"⍷") A(count,"⊒") \
/*    sfns.c*/A(shape,"⥊") A(pick,"⊑") A(pair,"{𝕨‿𝕩}") A(select,"⊏") A(slash,"/") A(join,"∾") A(couple,"≍") A(shiftb,"»") A(shifta,"«") A(take,"↑") A(drop,"↓") A(group,"⊔") A(reverse,"⌽") \
/*    sort.c*/A(gradeUp,"⍋") A(gradeDown,"⍒") \
/*   sysfn.c*/M(type,"•Type") M(decp,"•Decompose") M(primInd,"•PrimInd") M(glyph,"•Glyph") A(fill,"•FillFn") M(sys,"•getsys") A(grLen,"•GroupLen") D(grOrd,"•groupOrd") \
/*   sysfn.c*/M(repr,"•Repr") A(asrt,"!") M(out,"•Out") M(show,"•Show") M(bqn,"•BQN") D(cmp,"•Cmp") A(hash,"•Hash") M(delay,"•Delay") M(makeRand,"•MakeRand") M(exit,"•Exit") \
/*internal.c*/M(itype,"•internal.Type") M(refc,"•internal.Refc") M(squeeze,"•internal.Squeeze") M(isPure,"•internal.IsPure") A(info,"•internal.Info") \
/*internal.c*/D(variation,"•internal.Variation") A(listVariations,"•internal.ListVariations") M(clearRefs,"•internal.ClearRefs") M(unshare,"•internal.Unshare")

#define FOR_PM1(A,M,D) \
  /*md1.c*/ A(tbl,"⌜") A(each,"¨") A(fold,"´") A(scan,"`") A(const,"˙") A(swap,"˜") A(cell,"˘") \
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


#define F(N,X) extern B bi_##N;
FOR_PFN(F,F,F)
FOR_PM1(F,F,F)
FOR_PM2(F,F,F)
#undef F
