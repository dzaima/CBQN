### WHOLE-FILE-TEST
# only tests erroring cases
helpers ← ⊑•args
r ← helpers.GetRand@
⟨ListVariations, ClearRefs, Variation, ObjFlags⟩ ← •internal
CF ← 0⊸ObjFlags

Rn ← r.Range
nan ← 0÷0

{𝕊:
  Rl ← {𝕊: 0⌈¯2+⌊2⋆1+(2⋆⁼𝕩-2) × ×˜𝕨 Rn 0}
  arr ← {(Rn≠𝕩)◶𝕩 @} ⟨
    {𝕊: # bitarr
      ⟨Rl 500, 1+Rl 500, 1+Rl 500, Rl 500⟩ / 0‿1‿0‿1
    }
    {𝕊: # integer & float & char
      m ← 10
      n ← 2+Rl m
      type ← Rn 3
      float ← type=2
      char ← type=1
      fnan ← float ∧ 1=Rn 3
      
      ds ← {float? n Rn 0; n Rn 2+Rn 10}
      
      ds (-1+Rl 1000)⌾((1+Rn n-1)⊸⊑)⍟(¬fnan)↩
      off ← 1+Rl 1e9
      r ← off × (+`ds) - Rl m
      r nan⌾((Rn ¯1+≠r)⊸⊑)⍟fnan↩
      r {float? 𝕩∾nan⥊˜Rl 20; 𝕩}↩
    }
  ⟩
  ex ← ⊑arr
  _test ← {(𝔽 Variation⟜𝕩)¨ ListVariations 𝕩}
  arr ! ∧´ {𝕩 0∘⍋⎊1 ex} _test CF arr
  arr ! ∧´ {𝕩 0∘⍒⎊1 ex} _test CF ⌽arr
  ClearRefs@
  @
}¨ ↕5000
