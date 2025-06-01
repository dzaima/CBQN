### WHOLE-FILE-TEST
# tests if squeeze produces valid results
helpers ← ⊑•args
r ← helpers.GetRand@
⟨EEqual, ListVariations, ClearRefs, Variation, Unshare, Squeeze, Info⟩ ← •internal
⟨RandVals, vars, spec, specF64⟩ ← ⟨r⟩ helpers.Import "utils.bqn"

TestSq0 ← {a ← Squeeze 𝕨 Variation 𝕩 ⋄ ! 1==a ⋄ ! 0=≠a ⋄ a}
{! ⟨0⟩≡1↑𝕩 TestSq0 ↕0}¨ 5↑vars
{! " "≡1↑𝕩 TestSq0 ""}¨ 5↓vars
{𝕩 TestSq0 ⟨⟩}¨ ListVariations ⟨⟩ # {𝕩{•Out "Failed fill for "∾𝕨}⍟⊢ 8 ≠ 1↑⎊8 𝕩 TestSq0 ⟨⟩}¨ ListVariations ⟨⟩ # %ALLOW_CATCH

specSpl ← 5(↑⋈↓)spec
allSpec ← specF64 ∾ (@+0‿10‿32‿127‿128‿255‿256‿32767‿32768‿65535‿65536‿1114110‿1114111) ∾ {⇐}‿+
Do ← { 𝕊:
  at ← r.Range 8
  l ← 1 + r.Range (at=0)⊑1000‿300
  a ← l RandVals at
  {! a EEqual Squeeze 𝕩 Variation a}¨ ListVariations a
  
  F ← { 𝕊:
    p ← {r.Range 2? r.Range l; (¯1+l-⊢)⍟(r.Range 2) r.Range (2⋆r.Range 10)⌊l}
    b ← 𝕩˙⌾(p⊸⊑) a
    b0 ← Unshare b
    ¬b0 EEqual Squeeze b?
      •Out "Fail:"
      •Out "Array: "∾•Repr b
      •Out "Squeezed: "∾•Repr Squeeze b
      !0
    ;@
  }
  
  F¨ (at≥5)⊑specSpl
  F¨ allSpec
  {𝕊: F¨ @⊸+⍟(at≥5) r.Range 2⋆r.Range (at≥5)⊑33‿20}¨ ↕10
  ClearRefs@
}

Do⍟40000 @
