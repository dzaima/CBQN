⟨ListVariations, ClearRefs, Variation, Squeeze, Info⟩ ← •internal
u ← •UnixTime@
# u ↩ 1648896733

r ← •MakeRand •Show u

TestVars ← { w𝕊a‿b:
  n ← ¬w
  (ListVariations a) {
    ¬⍟n (𝕨 Variation a) ≡ (𝕩 Variation b)? @;
    •Out ∾"fail for "‿𝕨‿"/"‿𝕩‿", expected "‿(w⊑"0"‿"1")‿":"
    •Out •Repr a
    •Out •Repr b
    •Out •Repr a≠b
    •Exit 1
  }⌜ ListVariations b
  ClearRefs@
}

(1 TestVars ⋈˜)¨ ⟨⟨⟩ ⋄ "" ⋄ ↕0⟩

# types: u1,i8,i16,i32,f64,c8,c16,c32
szs ← 2∾(2⋆8‿16‿32)∾0∾(2⋆8‿16)∾1114111
off ← 0∾(2⋆7‿15‿31)∾¯0.5∾0‿0‿0
chr ← 0‿0‿0‿0‿0‿1‿1‿1
specF64 ← ∾⟜- ÷⟜0⊸∾ 1‿0
RandVal ← {
  𝕩≡4? {specF64⊏˜𝕩 r.Range ≠specF64}∘≠⌾((0=𝕨 r.Range 2⋆r.Range 10)⊸/) 𝕨 r.Range 0;
  Squeeze @+⍟(𝕩⊑chr) (𝕨 r.Range 𝕩⊑szs) - 𝕩⊑off
}

Do ← { 𝕊:
  at ← r.Range 8
  l ← 1 + r.Range (at=0)⊑100‿700
  a ← l RandVal at
  (⊢◶1‿{𝕊:∧´a=a} at=4) TestVars a‿a
  { 𝕊:
    p ← r.Range l
    v ← ⊑1 RandVal ⊢◶⟨r.Range∘5 ⋄ 5+r.Range∘3⟩ at≥5
    exp ← v = p⊑a
    b ← Squeeze v⌾(p⊸⊑) a
    (⊢◶exp‿{𝕊:∧´a=b} at=4) TestVars a‿b
  }⍟100 @
}

@∘Do⍟1000 @