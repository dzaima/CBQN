⟨LV⇐ListVariations, V⇐Variation, ClearRefs⇐ClearRefs, I⇐Info, TY⇐Type, Refc⇐Refc, EEQ⇐EEqual⟩ ← •internal
u ← ⌊100×(•UnixTime+1|100×•MonoTime)@

rand ← •MakeRand •Show u
⟨RandVals⟩ ← ⟨rand⟩ •Import "utils.bqn"
R ← rand.Range
RB ← "Ab" V R⟜2 # uniformly random boolean array
RB2 ← {¬⍟(R 2) "Ab" V 𝕩 R 1+R 2⋆R 10} # random boolean array with random uniform probability

rByteVals ← "Ai8"V {𝕩-256×𝕩>127} ∾{{2⊸×⊸+˜´ 8↑/⁼ 𝕩 •rand.Subset 8}¨ ↕9}¨↕20
RByte ← (R (≠rByteVals)˙)⊸(⊑⟜rByteVals)
RH ← "Ai8"⊸V¨ R⥊¨RByte # store to a variable to randomize heap by 𝕨 values, each up to 𝕩 bytes
CH ← { # do "var CH↩" to a variable assigned to a result of RH
  "heap corruption" ! ∧´2=Refc¨ 𝕩
  "heap corruption" ! 1 ≡ Refc 𝕩
  0
}

OrScanBitarr ← {𝕊:
 n ← 1 + R 2⋆R 13
 ! (∨` ≡ ⊢∘∨`) "Ab" V {𝕊: 1⌾((R n)⊸⊑)𝕩}⍟2 "Ai8" V 0⥊˜ n
}¨ ↕

PlusScanBitarr ← {𝕊:
  a ← RB2 R 2⋆R 13
  ! (+` ≡ ⊢∘+`) a
}¨ ↕

BitSlash ← {ty‿dy‿max𝕊n:
  f ← dy ⊑ ⟨/∘⊢ ⋄ {𝕩/ty V 𝕨 R 100}⟩
  {𝕊:
    # 𝕩⊸{•Show 𝕨}⍟⊢ 0=10000|𝕩
    t1←20 RH 100
    n←R max
    x←n F n↑RB2 128+n
    t2←20 RH 100
    x↩0 ⋄ t1 CH↩ ⋄ t2 CH↩
  }¨↕n
}

_testConsistent ← { w0 F _𝕣 x0:
  ∧´ (⊏ EEQ¨1↓⊢) ⥊𝕨 {(𝕨 V w0) F (𝕩 V x0)}⌜○LV 𝕩? ClearRefs@;
  
  w0 F _𝕣 x0:
  •Out "Fail for:"
  "𝕨: "‿"𝕩: " •Out∘(∾⟜•Repr)¨ 𝕨⋈𝕩
  tbl ← 𝕨 {(𝕨 V w0) F (𝕩 V x0)}⌜○LV 𝕩
  •Show (<∘⊑≡¨⊢) tbl
  •Exit 1
}

Group ← {𝕊:
  t ← R 8
  l ← R 10
  x ← l RandVals t
  w ← ¯1+l R 1+l
  w ⊔_testConsistent x
  1
}¨ ↕

•Show "∨`bitarr" ⋄   OrScanBitarr 100000
•Show "+`bitarr" ⋄ PlusScanBitarr 100000

•Show "heap corruption test of /bit"    ⋄      @‿0‿200 BitSlash 1000000
•Show "heap corruption test of bit/i8"  ⋄  "Ai8"‿1‿50  BitSlash 1000000
•Show "heap corruption test of bit/i16" ⋄ "Ai16"‿1‿50  BitSlash 1000000
•Show "heap corruption test of bit/i32" ⋄ "Ai32"‿1‿50  BitSlash 1000000

{
  ¬0 EEQ -0? •Out "Skipping tests requiring EEQUAL_NEGZERO";
  •Show "⊔ consistency" ⋄ Group 10000
}
