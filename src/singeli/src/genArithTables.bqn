debug ← 0=≠•args
•term.OutRaw⍟debug ∾⟨@+27 ⋄ "[H" ⋄ @+27 ⋄ "[2J" ⋄ @+27 ⋄ "[3J"⟩
•Out⍟(3×debug) 200⥊'-'
impls‿tables ← 2↑•args

T ← =⊸{<⎉𝕨⍉⍟𝕨>𝕩}

bit←0
i8 ←1 ⋄ c8 ←5
i16←2 ⋄ c16←6
i32←3 ⋄ c32←7
f64←4
u‿c‿e‿rsq‿xsq ← ↕5
tmNoNext ← 1‿0‿1‿1‿1
tmCheckedFn ← 0‿1‿1‿0‿0
tmLit ← "uceue"

tyWidth ← 99‿0‿1‿2‿3‿0‿1‿2 # in log2(bytes)
tyName ← "bit"‿"i8"‿"i16"‿"i32"‿"f64"‿"c8"‿"c16"‿"c32"
tySiType ← "u1"‿"i8"‿"i16"‿"i32"‿"f64"‿"u8"‿"u16"‿"u32"

ShowTbl ← {
  # 𝕩 ↩ {∧´⥊𝕩≡¨@? @; 𝕩/˜𝕩≢¨@}¨ 𝕩
  𝕩 ≍˘⍟=¨ ↩
  •Show 𝕩
  #•Out •Repr 𝕩
}⍟debug

mdNone ← 0
mdExact ← 1
mdCast ← 2
mdBit2i8 ← 3
mdF64Chr ← 4
mdSqF64 ← 5

singeliFns ← ⟨⟩
GetSingeli ← { tm‿w‿x‿r‿id‿si:
  {
    r≡@? @;
    name ← ∾⟨id, "AA", tm⊑tmLit, "_", w⊑tyName, "_", x⊑tyName, "_", r⊑tyName⟩
    singeliFns∾↩ <name‿w‿x‿r‿tm‿si
    name
  }
}

nl←@+10
cSrc ← ""
Make ← {𝕊ns:
  cbqn‿repr‿id‿si‿commutative ← ns
  
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
        {𝕊: ⟨GetSingeli e, t, Type@, call∾(bit=3⊑n⊑map)⊑"rbyte"‿"bit", s, @⟩} # mdExact
        {𝕊: ⟨GetSingeli e, t, Type@, call∾(bit=3⊑n⊑map)⊑"rbyte"‿"bit", s, ((s≠m)/"wx")∾tyName⊑˜⊑m/n⟩} # mdCast
        {𝕊: ⟨GetSingeli e, t, Type@, call∾"wxi8", s, @⟩} # mdBit2i8
        {𝕊: ⟨@,            t, @,     "e_call_sqx", s, @⟩} # mdF64Chr
        {𝕊: ⟨GetSingeli e, t, Type@, call∾"wxf64sq", s, @⟩} # mdSqF64
      ⟩ 𝕩
    }¨ res
  }¨ aaA‿aaB
  
  # ShowTbl ∾´¨ instrs
  
  cSrc∾↩ ∾⟨
    "B ", cbqn, "_c2(B,B,B);", nl
    "static DyTable ", cbqn,"DyTable = {", nl
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
              @⊸≢◶""‿{∾⟨",.uFn"‿",.cFn"⊑˜t⊑tmCheckedFn,"=",𝕩⟩} fn
            ⟩
        }¨ instrs
        ∾⟨"    [",•Repr we,"*8+",•Repr xe,"] = {{", i0, "}, {", i1, "}},", nl⟩
      }
    }¨ ⥊T ⟨instrs ⋄ ↕8‿8⟩
    "  },", nl
    "  .mainFn = ", cbqn, "_c2,", nl
    "  .repr = """, repr, """", nl
    "};", nl
  ⟩
}



# TODO unify sub u‿c32‿c32‿i32 and c‿i32‿i32‿i32?
# TODO when there's singeli div/sqrt, use scalar for like 1-2 tail elements

{ # definitions
  # ⟨mode, w, x, r⟩
  c_numDiag ← ⟨c‿i8‿i8‿i8,  c‿i16‿i16‿i16, c‿i32‿i32‿i32, u‿f64‿f64‿f64⟩
  u_numDiag ← ⟨u‿i8‿i8‿i8,  u‿i16‿i16‿i16, u‿i32‿i32‿i32, u‿f64‿f64‿f64⟩
  w_numDiag ← ⟨u‿i8‿i8‿i16, u‿i16‿i16‿i32, u‿i32‿i32‿f64⟩
  bitand ← ⟨u‿bit‿bit‿bit‿"and"‿"__and"⟩
  bitor  ← ⟨u‿bit‿bit‿bit‿"or"‿"__or"⟩
  Make {
    commutative⇐1 ⋄ repr⇐"+" ⋄ cbqn⇐"add" ⋄ si⇐"__add" ⋄ id⇐"add"
    aa_a ⇐ c_numDiag∾⟨c‿i8‿c8‿c8,  c‿i16‿c16‿c16, e‿i32‿c32‿c32, xsq‿c8‿f64‿@, xsq‿c16‿f64‿@, xsq‿c32‿f64‿@⟩∾⟨c‿i8‿i16‿i16, c‿i8‿i32‿i32, c‿i16‿i32‿i32⟩
    aa_b ⇐ w_numDiag∾⟨e‿i8‿c8‿c16, e‿i16‿c16‿c32⟩
  }
  Make {
    commutative⇐0 ⋄ repr⇐"-" ⋄ cbqn⇐"sub" ⋄ si⇐"__sub" ⋄ id⇐"sub"
    aa_a ⇐ c_numDiag∾⟨c‿c8‿c8‿i8,  c‿c16‿c16‿i16, u‿c32‿c32‿i32⟩∾⟨c‿c8‿i8‿c8,  c‿c16‿i16‿c16, e‿c32‿i32‿c32⟩∾⟨xsq‿c8‿f64‿@, xsq‿c16‿f64‿@, xsq‿c32‿f64‿@⟩
    aa_b ⇐ w_numDiag∾⟨u‿c8‿c8‿i16, u‿c16‿c16‿i32⟩               ∾⟨e‿c8‿i8‿c16, e‿c16‿i16‿c32⟩
  }
  Make¨ { repr‿cbqn:
    commutative⇐1 ⋄ repr‿cbqn⇐ ⋄ si⇐"__mul" ⋄ id⇐"mul"
    aa_a ⇐ bitand∾c_numDiag # ∾⟨u‿bit‿i8‿i8, u‿bit‿i16‿i16, u‿bit‿i32‿i32, u‿bit‿f64‿f64⟩
    aa_b ⇐ w_numDiag        # TODO readd ↑
  }¨ ⟨"×"‿"mul", "∧"‿"and"⟩
  Make {
    commutative⇐1 ⋄ repr⇐"∨" ⋄ cbqn⇐"or" ⋄ si⇐"bqn_or" ⋄ id⇐"or"
    aa_a ⇐ bitor∾⟨rsq‿f64‿f64‿f64⟩ # TODO maybe new instr to squeeze arguments & try to go bitarr-bitarr
    aa_b ⇐ ⟨⟩                      # and maybe use that for ∧ but not ×
  }
  Make {
    commutative⇐1 ⋄ repr⇐"⌊" ⋄ cbqn⇐"floor" ⋄ si⇐"min" ⋄ id⇐"min"
    aa_a ⇐ bitand∾u_numDiag ⋄ aa_b ⇐ ⟨⟩
  }
  Make {
    commutative⇐1 ⋄ repr⇐"⌈" ⋄ cbqn⇐"ceil" ⋄ si⇐"max" ⋄ id⇐"max"
    aa_a ⇐ bitor∾u_numDiag ⋄ aa_b ⇐ ⟨⟩
  }
  # Make¨ { repr‿cbqn‿si:
  #   commutative⇐0 ⋄ repr‿cbqn‿si⇐ ⋄ id⇐cbqn
  #   aa_a ⇐ ⟨rsq‿f64‿f64‿f64⟩ ⋄ aa_b ⇐ ⟨⟩
  # }¨ ⟨"÷"‿"div"‿"__div", "√"‿"root"‿@, "|"‿"stile"‿@, "⋆"‿"pow"‿@, "⋆⁼"‿"log"‿@⟩
}

siSrc ← ""
GenSingeli ← { 𝕊name‿w‿x‿r‿tm‿si:
  si≢@?
    siSrc ∾↩ ∾⟨"'", name, "'=arithAA{", '0'+tm, ',', si, ',', w⊑tySiType, ',', x⊑tySiType, ',', r⊑tySiType, '}', nl⟩;
  # else
    @
}

GenSingeli¨ ⍷singeliFns

⊢◶{𝕊:
  impls •FChars siSrc
  tables •FChars cSrc
}‿{𝕊:
  # •Out cSrc
  •Out siSrc
  1
} debug

