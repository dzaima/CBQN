### WHOLE-FILE-TEST
# generally unnecessary now that debug builds include heap corruption testing
⟨LV⇐ListVariations, V⇐Variation, ClearRefs⇐ClearRefs, I⇐Info, TY⇐Type, Refc⇐Refc, EEQ⇐EEqual⟩ ← •internal
helpers ← ⊑•args
R ← (helpers.GetRand@).Range
RB2 ← {¬⍟(R 2) "Ab" V 𝕩 R 1+R 2⋆R 10} # random boolean array with random uniform probability

rByteVals ← "Ai8"V {𝕩-256×𝕩>127} ∾{{2⊸×⊸+˜´ 8↑/⁼ 𝕩 •rand.Subset 8}¨ ↕9}¨↕20
RByte ← (R (≠rByteVals)˙)⊸(⊑⟜rByteVals)

RH ← "Ai8"⊸V¨ R⥊¨RByte # store to a variable to randomize heap by 𝕨 values, each up to 𝕩 bytes
CH ← { # do "var CH↩" to a variable assigned to a result of RH
  "heap corruption" ! ∧´2=Refc¨ 𝕩
  "heap corruption" ! 1 ≡ Refc 𝕩
  0
}
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
•Show "heap corruption test of /bit"    ⋄      @‿0‿200 BitSlash 1000000
•Show "heap corruption test of bit/i8"  ⋄  "Ai8"‿1‿50  BitSlash 1000000
•Show "heap corruption test of bit/i16" ⋄ "Ai16"‿1‿50  BitSlash 1000000
•Show "heap corruption test of bit/i32" ⋄ "Ai32"‿1‿50  BitSlash 1000000
