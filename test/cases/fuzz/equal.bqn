### WHOLE-FILE-TEST
# tests 𝕨 ≡ 𝕩
helpers ← ⊑•args
r ← helpers.GetRand@
⟨ListVariations, ClearRefs, Variation, Squeeze, Info⟩ ← •internal

TestVars ← { w𝕊a‿b:
  n ← ¬w
  (ListVariations a) {
    ¬⍟n (𝕨 Variation a) ≡ (𝕩 Variation b)? @;
    •Out ∾"fail for "‿𝕨‿"/"‿𝕩‿", expected "‿(w⊑"0"‿"1")‿":"
    •Out •Repr a
    •Out •Repr b
    •Out •Repr a≠b
    !0
  }⌜ ListVariations b
  ClearRefs@
}

(1 TestVars ⋈˜)¨ ⟨⟨⟩ ⋄ "" ⋄ ↕0⟩

⟨RandVals⟩ ← ⟨r⟩ helpers.Import "utils.bqn"

Do ← { 𝕊:
  at ← r.Range 8
  l ← 1 + r.Range (at=0)⊑100‿700
  a ← l RandVals at
  (⊢◶1‿{𝕊:∧´a=a} at=4) TestVars a‿a
  { 𝕊:
    p ← r.Range l
    v ← ⊑1 RandVals ⊢◶⟨r.Range∘5 ⋄ 5+r.Range∘3⟩ at≥5
    exp ← v = p⊑a
    b ← Squeeze v⌾(p⊸⊑) a
    (⊢◶exp‿{𝕊:∧´a=b} at=4) TestVars a‿b
  }∘@⍟100 @
}

@∘Do⍟1000 @
