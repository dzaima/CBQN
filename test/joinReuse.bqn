⟨V⇐Variation ⋄ LV⇐ListVariations⟩←•internal
AllEq ← !¨ (<∘⊑≡¨⊢)
# all that's important in the output is that there are at least some "reuse:1"s and "reuse:0"s for each item; quoted errors the output are expected

{ w𝕊x:
  •Out "next"
  AllEq t ← {(⥊ ⋈ ≢) (𝕨 V w)∾𝕩 V x}⌜´ LV¨ 𝕨‿𝕩
  •Out "" ⋄ •Show ⊑t
}´¨ ⟨
  "ab"‿["01","23"]
  ["ab","cd"]‿"01"
  ["ab","cd","ef"]‿["01","23"]
  [["ab","cd"]⋄["ef","gh"]⋄["ij","kl"]]‿["01","23"]
⟩

{𝕊:

  { w𝕊x:
    •Out "next"
    AllEq t ← {(𝕨 V w)∾𝕩 V x ⋄ •Out "Expected error, didn't get one; fail" ⋄ •Exit 1}⎊{𝕊:•CurrentError@}⌜´ LV¨ 𝕨‿𝕩
    •Out "" ⋄ •Show ⊑t
  }´¨ ⟨
    "abc"‿["01","23"]
    ["ab","cd"]‿"012"
    ["ab","cd","ef"]‿["012","345"]
    [["ab","cd"]⋄["ef","gh"]⋄["ij","kl"]]‿["012","345"]
  ⟩
}⍟⊢ 0=≠•args