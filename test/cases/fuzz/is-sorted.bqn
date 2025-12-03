### WHOLE-FILE-TEST
helpers ← ⊑•args
r ← helpers.GetRand@
⟨ListVariations, ClearRefs, Variation, ObjFlags⟩ ← •internal
CF ← 0⊸ObjFlags

Rn ← r.Range
nan ← 0÷0

{𝕊:
  Rl ← {𝕊: 0⌈¯2+⌊2⋆1+(¯1+2⋆⁼𝕩) × ×˜𝕨 Rn 0}
  ok ← Rn 2
  bad ← ¬ok
  m ← 500
  bm ← m×8
  
  arr ← {(Rn≠𝕩)◶𝕩 @} ⟨
    {𝕊: # bitarr
      ok? ⟨1+Rl bm, 1+Rl bm⟩ / 0‿1
      ;   ⟨Rl bm, 1+Rl bm, 1+Rl bm, Rl bm⟩ / 0‿1‿0‿1
    }
    {𝕊: # integer & float
      n ← 2+Rl m
      float ← Rn 2
      fnan ← float ∧ 1=Rn 3
      
      ds ← {float? n Rn 0; n Rn 2+Rn 10}
      
      ds (-1+Rl 1000)⌾((1+Rn n-1)⊸⊑)⍟(bad∧¬fnan)↩
      mul ← 1+Rl 1e9
      r ← mul × (+`ds) - Rl m
      r nan⌾((Rn ¯1+≠r)⊸⊑)⍟(bad∧fnan)↩
      r {𝕩∾nan⥊˜Rl 20}⍟float↩
    }
    {𝕊: # char
      n ← 2+Rl m
      ds ← n Rn 2+Rn 10
      e ← 1114111
      r ← e⌊ (Rl e÷≠n)×+`ds
      r {a‿·: a {𝕊: e⌊1+Rl e×1.1}⍟(0=⊢)↩ ⋄ ⟨a, a-1+Rl a⟩}⌾((0‿1+Rn n-1)⊸⊏)⍟bad↩
      r {𝕩 + 0⌈(⌈´𝕩)-˜ Rn∘≠⊸⊑ 0‿0‿255‿65535‿e}↩
      @+r
    }
  ⟩
  arr •internal.Squeeze↩
  
  ex ← ⊑arr
  _test ← {{𝕊: c←2=•Type ex ⋄ ⟨ok,c,-⟜@⍟c arr⟩}⍟¬⊸! ok{𝕗?¬∨´;∧´} (𝔽 Variation⟜𝕩)¨ ListVariations 𝕩}
  {𝕩 0∘⍋⎊1 ex} _test CF arr
  {𝕩 0∘⍒⎊1 ex} _test CF ⌽arr
  ClearRefs@
  @
}¨ ↕5000
