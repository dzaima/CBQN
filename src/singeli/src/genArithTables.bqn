debug ← 0=≠•args
impls‿tables ← 2↑•args

T ← =⊸{<⎉𝕨⍉⍟𝕨>𝕩}

bit←0
i8 ←1 ⋄ c8 ←5
i16←2 ⋄ c16←6
i32←3 ⋄ c32←7
f64←4
u‿c‿e‿rsq‿xsq ← ↕5
tmNoNext ← 1‿0‿1‿1‿1
tmChkFnAA ← 0‿1‿1‿0‿0
tmLit ← "uceue"

tyWidth ← 99‿0‿1‿2‿3‿0‿1‿2 # in log2(bytes)
tyName ← "bit"‿"i8"‿"i16"‿"i32"‿"f64"‿"c8"‿"c16"‿"c32"
tySiName ← "u1"‿"i8"‿"i16"‿"i32"‿"f64"‿"u8"‿"u16"‿"u32"

mdNone ← 0
mdExact ← 1
mdCast ← 2
mdBit2i8 ← 3
mdF64Chr ← 4
mdSqF64 ← 5

singeliAAFns ← ⟨⟩
GetSingeliAA ← { tm‿w‿x‿r‿id‿si:
  {
    r≡@? @;
    name ← ∾⟨id, "AA", tm⊑tmLit, "_", w⊑tyName, "_", x⊑tyName, "_", r⊑tyName⟩
    singeliAAFns∾↩ <name‿w‿x‿r‿tm‿si
    name
  }
}

singeliASFns ← ⟨⟩

nl←@+10
cSrc ← ""
siSrc ← ""

Make ← {𝕊ns:
  cbqn‿repr‿id‿si‿commutative ← ns
  
  # array-array
  aaA‿aaB ← {4≡≠𝕩?𝕩∾id‿si; 𝕩}¨¨ ns.aa_a‿ns.aa_b # add ID and singeli fn to entries that don't have it explicit
  
  currTypes ← ↕8‿8
  swap ← 8‿8⥊0
  {𝕊:
    aaA‿aaB {⍷𝕩 ∾ ⌽⌾(1‿2⊸⊏)¨ 𝕩}¨↩ # allow definitions from either side of the diagonal
    f64chr ← (≥⟜5 ∧○(∨´¨) =⟜4) currTypes
    swap ↩ <´¨ ↕8‿8 # map of which entries to swap
    currTypes ↩ swap ⌽¨ currTypes
  }⍟⊢ commutative
  
  instrs ← ⋈¨´ {
    map ← 𝕩⌾(({1‿2⊏𝕩}¨ 𝕩)⊸⊑)⍟(×≠𝕩) 8‿8⥊@
    has ← map≢¨@
    newTypes‿res ← T { 𝕊 si:
      ti ← (𝕩⊸-+⌈´) {(5-𝕩≥5)|𝕩} 𝕩 # upcast index
      {
        si≡¯1‿¯1?
          si⋈mdNone;
        ∨´ ti=8? si⊑has? # f64 F chr
          ¯1‿¯1⋈mdF64Chr;
        si⊑has? # already has an impl
          si⋈mdExact;
        ∧´ ti≠8? ∨´m←(is←⥊si<⊸+↕ti¬si)⊑has? # cast one argument up in size
          (⊑m/is)⋈mdCast;
        si≡0‿0? 1‿1⊑has? # c_call_wxi8
          1‿1⋈mdBit2i8;
        ∧´ si<5‿5? rsq≡⊑4‿4⊑map? # cast both to f64 & squeeze afterwards
          4‿4⋈mdSqF64;
        # else
          ¯1‿¯1⋈mdNone
      }
    }¨ prevTypes←currTypes
    updMap ← {𝕩≡@? 𝕨; 𝕩}´¨ (prevTypes⊑map) ⋈¨ newTypes⊑map
    typeMap ← {u¨⌾((⥊@=𝕩)/⥊)𝕩} ⊑¨updMap
    uncheckedMap ← typeMap⊏tmNoNext
    currTypes ↩ ¯1‿¯1¨⌾(uncheckedMap/○⥊⊢) newTypes # remove unchecked operations from next pass
    
    swap ↩ 0¨ prevSwap←swap
    
    (T prevTypes‿newTypes‿typeMap‿updMap‿prevSwap) { p‿n‿t‿e‿s𝕊𝕩:
      call ← (t⊑tmLit)∾"_call_"
      m←≠´p‿n
      Type ← {𝕊: 3⊑n⊑map}
      ⊢◶⟨
        @ # mdNone
        {𝕊: ⟨GetSingeliAA e, t, Type@, call∾(bit=3⊑n⊑map)⊑"rbyte"‿"bit", s, @⟩} # mdExact
        {𝕊: ⟨GetSingeliAA e, t, Type@, call∾(bit=3⊑n⊑map)⊑"rbyte"‿"bit", s, ((s≠m)/"wx")∾tyName⊑˜⊑m/n⟩} # mdCast
        {𝕊: ⟨GetSingeliAA e, t, Type@, call∾"wxi8", s, @⟩} # mdBit2i8
        {𝕊: ⟨@,            t, @,     "e_call_sqx", s, @⟩} # mdF64Chr
        {𝕊: ⟨GetSingeliAA e, t, Type@, call∾"wxf64sq", s, @⟩} # mdSqF64
      ⟩ 𝕩
    }¨ res
  }¨ aaA‿aaB
  
  {•Show ≍˘¨𝕩}⍟debug instrs
  
  cSrc∾↩ ∾⟨
    "B ", cbqn, "_c2(B,B,B);", nl
    "INIT_GLOBAL DyTableAA ", cbqn,"DyTableAA = {", nl
    "  .entsAA = {", nl
    ∾{ ⟨instrs, we‿xe⟩:
      {
        instrs≡@‿@? "";
        i0‿i1 ← 1‿0 {
          𝕨 𝕊 @: "";
          i0 𝕊 fn‿t‿rt‿ex‿swap‿cast:
            ! i0 ∨ ¬swap
            two ← swap ∨ @≢cast
            ∾⟨⊢◶""‿{𝕊:∾⟨
              ".ex1=",@⊸≢◶⟨"",∾⟜"_"⟩ cast,swap⊑"reg,"‿"swap,"⟩} two,
              ".ex", '1'+two, "=", ex,
              @⊸≢◶""‿{∾⟨",.type=t_",𝕩⊑tyName,"arr,.width=",•Repr 𝕩⊑tyWidth⟩} rt,
              @⊸≢◶""‿{∾⟨",.uFn"‿",.cFn"⊑˜t⊑tmChkFnAA,"=",𝕩⟩} fn
            ⟩
        }¨ instrs
        ∾⟨"    [",•Repr we,"*8+",•Repr xe,"] = {{{", i0, "}, {", i1, "}}},", nl⟩
      }
    }¨ ⥊T ⟨instrs ⋄ ↕8‿8⟩
    "  },", nl
    "  .mainFn = ", cbqn, "_c2,", nl
    "  .repr = """, repr, """", nl
    "};", nl
  ⟩
  
  # array-scalar / scalar-array
  tyi ← ⟨@,  "i8","i16","i32","f64", "i8","i16","i32"⟩
  tyc ← ⟨@,  "c8","c16","c32",@,     "c8","c16","c32"⟩
  isc ← ⟨0, 0,    0,    0,    0,     1,   1,    1⟩
  { 𝕊 swap:
    choices ← ⌽⍟swap⌾(2⊸↑)¨ ns.sa
    hadChrArr ← 0
    _tyMap ← {(𝔽 𝕩)⌾((2↑¨𝕩='c')⊸⊑) 2‿2⥊@}
    ResMap ← ('c'=2⊑¨⊢) _tyMap
    {
      0≡≠𝕩? @;
      charAtm ← 'c' ≡ ⊑⊑𝕩
      hasNumArr  ← ⊑'i' ∊ 1⊑¨𝕩
      hasChrArr ← ⊑'c' ∊ 1⊑¨𝕩
      resChr ← ResMap 𝕩
      uncheckedMap ← (⊑¨'u'∊¨⊢) _tyMap 𝕩
      argarr ← ⌽⍟swap "SA"
      canOverflow ← "f64"⊸≢¨ tyName
      P ← {∾⟨"{",𝕨,",",𝕩,"}"⟩}´
      
      cSrc∾↩ ∾⟨
        "INIT_GLOBAL DyTableSA ", cbqn,"DyTable", ⌽⍟swap (charAtm⊑"NC")∾"A", " = {", nl
        "  .ents = {", nl
        "    {.bitsel=", hasNumArr◶⟨"bad", {𝕊:id∾swap/"R"}⟩ @, "_forBitsel", charAtm⊑"NC", "N_SA},", nl
        ∾{ 𝕊 num:
          onlyFirst ← ns.overflowEq⊑ ⟨1, ¬num⊑canOverflow⟩
          f1‿f2 ← 1‿2 {
            𝕩≢@?
            charArr ← num⊑isc
            ¬ (charAtm∧¬num⊑canOverflow) ∨ (onlyFirst ∧ 𝕨=2) ∨ (¬hasChrArr)∧charArr?
            currRes ← charAtm‿charArr⊑resChr
            unchFn2 ← charAtm‿charArr⊑uncheckedMap
            currRes≢@?
            tw←num⊑charAtm⊑tyi‿tyc ⋄ tx←num⊑tyName ⋄ tr←𝕩⊑currRes⊑tyi‿tyc
            tr≢@?
            siName ← ∾⟨id, argarr, 𝕨⊑"?ce", ∾'_'⊸∾¨ tw‿tx‿tr⟩
            𝕨 { 𝕊:
              siSrc∾↩ ∾⟨"export{'", siName, "', arith_sa{", '0'+ 𝕨⊑⟨@, onlyFirst⊑0‿2, unchFn2⊑1‿3⟩, ',', si, ',', '0'+swap, ∾(',' ∾ 'u'⌾⊑⍟('c'≡⊑))¨ tw‿tx‿tr, "}}", nl⟩
              singeliASFns∾↩ <siName
            }⍟⊢ ¬⊑(<siName)∊singeliASFns
            siName;
            "failAtomArr"∾'0'+𝕨
          }¨ ⟨num⊑@‿1‿2‿3‿4‿5‿6‿7,
              num⊑@‿2‿3‿4‿@‿6‿7‿@⟩
          ∾⟨"    {.f1=",f1,", .f2=",f2,"},", nl⟩
        }¨ 1+↕7
        "  },", nl
        "  .chrAtom = ", hadChrArr◶"NULL"‿{𝕊: ∾⟨"&",cbqn,"DyTable", ⌽⍟swap "CA"⟩} @, ",", nl
        "  .mainFn = ", cbqn, "_c2", swap/"R", ",", nl
        "  .fill = ", P P¨ <˘ '0'+@‿0‿1⊐ResMap choices, ",", nl
        "  .repr = """, repr, """", nl
        "};", nl
      ⟩
      
      hadChrArr∨↩ charAtm
    }¨ 2↑(("ci"⊐⊑¨)⊔⊢) choices
  }¨ ↕2-commutative
  
  siSrc∾↩ nl
}




# definitions
{
  # TODO when there's singeli div/sqrt, use scalar for like 1-2 tail elements
  # aa_a/aa_b: list of ⟨mode, w, x, r⟩
  # sa: list of "wxr", with an appended "u" if widened case can't error
  c_numDiag ← ⟨c‿i8‿i8‿i8,  c‿i16‿i16‿i16, c‿i32‿i32‿i32, u‿f64‿f64‿f64⟩
  u_numDiag ← ⟨u‿i8‿i8‿i8,  u‿i16‿i16‿i16, u‿i32‿i32‿i32, u‿f64‿f64‿f64⟩
  w_numDiag ← ⟨u‿i8‿i8‿i16, u‿i16‿i16‿i32, u‿i32‿i32‿f64⟩
  bitand ← ⟨u‿bit‿bit‿bit‿"and"‿"__and"⟩
  bitor  ← ⟨u‿bit‿bit‿bit‿"or"‿"__or"⟩
  Make {
    commutative⇐1 ⋄ repr⇐"+" ⋄ cbqn⇐"add" ⋄ si⇐"__add" ⋄ id⇐"add"
    aa_a ⇐ c_numDiag∾⟨c‿i8‿c8‿c8,  c‿i16‿c16‿c16, e‿i32‿c32‿c32, xsq‿c8‿f64‿@, xsq‿c16‿f64‿@, xsq‿c32‿f64‿@⟩∾⟨c‿i8‿i16‿i16, c‿i8‿i32‿i32, c‿i16‿i32‿i32⟩
    aa_b ⇐ w_numDiag∾⟨e‿i8‿c8‿c16, e‿i16‿c16‿c32⟩
    sa⇐"iiiu"‿"icc"‿"cic" ⋄ overflowEq⇐1
  }
  Make {
    commutative⇐0 ⋄ repr⇐"-" ⋄ cbqn⇐"sub" ⋄ si⇐"__sub" ⋄ id⇐"sub"
    aa_a ⇐ c_numDiag∾⟨c‿c8‿c8‿i8,  c‿c16‿c16‿i16, u‿c32‿c32‿i32⟩∾⟨c‿c8‿i8‿c8,  c‿c16‿i16‿c16, e‿c32‿i32‿c32⟩∾⟨xsq‿c8‿f64‿@, xsq‿c16‿f64‿@, xsq‿c32‿f64‿@⟩
    aa_b ⇐ w_numDiag∾⟨u‿c8‿c8‿i16, u‿c16‿c16‿i32⟩               ∾⟨e‿c8‿i8‿c16, e‿c16‿i16‿c32⟩
    sa⇐"iiiu"‿"cic"‿"cciu" ⋄ overflowEq⇐1
  }
  Make¨ { repr‿cbqn:
    commutative⇐1 ⋄ repr‿cbqn⇐ ⋄ si⇐"__mul" ⋄ id⇐"mul"
    aa_a ⇐ bitand∾c_numDiag∾⟨u‿bit‿i8‿i8, u‿bit‿i16‿i16, u‿bit‿i32‿i32⟩ # no u‿bit‿f64‿f64 because 0×∞ etc
    aa_b ⇐ w_numDiag
    sa⇐⟨"iiiu"⟩ ⋄ overflowEq⇐1
  }¨ ⟨"×"‿"mul", "∧"‿"and"⟩
  Make {
    commutative⇐1 ⋄ repr⇐"∨" ⋄ cbqn⇐"or" ⋄ si⇐"bqn_or" ⋄ id⇐"or"
    aa_a ⇐ bitor∾⟨rsq‿f64‿f64‿f64⟩ # TODO maybe new instr to squeeze arguments & try to go bitarr-bitarr
    aa_b ⇐ ⟨⟩                      # and maybe use that for ∧ but not ×
    sa⇐⟨⟩ ⋄ overflowEq⇐1
  }
  Make {
    commutative⇐1 ⋄ repr⇐"⌊" ⋄ cbqn⇐"floor" ⋄ si⇐"__min" ⋄ id⇐"min"
    aa_a ⇐ bitand∾u_numDiag ⋄ aa_b ⇐ ⟨⟩
    sa⇐⟨"iiiu"⟩ ⋄ overflowEq⇐0
  }
  Make {
    commutative⇐1 ⋄ repr⇐"⌈" ⋄ cbqn⇐"ceil" ⋄ si⇐"__max" ⋄ id⇐"max"
    aa_a ⇐ bitor∾u_numDiag ⋄ aa_b ⇐ ⟨⟩
    sa⇐⟨"iiiu"⟩ ⋄ overflowEq⇐0
  }
  # Make¨ { repr‿cbqn‿si:
  #   commutative⇐0 ⋄ repr‿cbqn‿si⇐ ⋄ id⇐cbqn
  #   aa_a ⇐ ⟨rsq‿f64‿f64‿f64⟩ ⋄ aa_b ⇐ ⟨⟩
  #   sa⇐"i" ⋄ as⇐"i" ⋄ overflowEq⇐1
  # }¨ ⟨"÷"‿"div"‿"__div", "√"‿"root"‿@, "|"‿"stile"‿@, "⋆"‿"pow"‿@, "⋆⁼"‿"log"‿@⟩
}

GenSingeli ← { 𝕊name‿w‿x‿r‿tm‿si:
  si≢@?
    siSrc ∾↩ ∾⟨"export{'", name, "', arith_aa{", '0'+tm, ',', si, ',', w⊑tySiName, ',', x⊑tySiName, ',', r⊑tySiName, "}}", nl⟩;
  # else
    @
}

GenSingeli¨ ⍷singeliAAFns

⊢◶{𝕊:
  impls •FChars siSrc
  tables •FChars cSrc
}‿{𝕊:
  •Out cSrc
  •Out siSrc
  1
} debug