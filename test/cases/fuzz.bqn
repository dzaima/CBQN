(
  # character 𝕨∊𝕩
  ⟨V⇐Variation,LV⇐ListVariations,CL⇐ClearRefs,EE⇐EEqual⟩←•internal
  R ← •rand.Range
  _t←{w F _𝕣 x: ∧´EE¨⟜⊏⥊(LV 𝕨){(𝕨 V w) F 𝕩 V x}⌜LV𝕩?CL@; •Show¨𝕗‿𝕨‿𝕩⋄!0}
  GS←{𝕊: @+0⌈(¯1+2⋆R∘≠⊸⊑7‿8‿15‿16‿20) - R˜R 200}
  {𝕊:(GS@) ∊_t GS@}¨↕1000
)

(
  # ∾𝕩 not getting garbage
  ≠{𝕊: ! ∧´1=⥊∾⎊1 ⥊⟜1¨ (4•rand.Range 3) •rand.Range¨ 4}¨↕10000
)
