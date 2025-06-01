### WHOLE-FILE-TEST
# tests full array copying via •internal.Variation
helpers ← ⊑•args
R ← (helpers.GetRand@).Range
⟨Variation, ListVariations, ClearRefs⟩ ⇐ •internal

vn ← "Ab"‿"Ai8"‿"Ai16"‿"Ai32"‿"Af64"‿"Ac8"‿"Ac16"‿"Ac32"‿"Ah"‿"Af"
vi ←⟨0,   0,    0,     0,     1,     2,    2,     2,     1,   1⟩

vf ← ∾⟨
  ↓⟜(5↑  vn)¨ ↕5
  ↓⟜(3↑5↓vn)¨ ↕3
  ⟨⟩‿⟨⟩
⟩
vf ↩ ∾⟜(8↓vn)¨ vf
vf ↩ (↕10) {𝕩 ∾ 'S'∾¨ 1↓¨ (𝕨=0)↓𝕩}¨ vf
vf ↩ {𝕩 ∾ ∾⟜"Inc"¨ 𝕩}¨ vf

# •Show¨ vf

count ← 10000
eqlen ← 0

{𝕊:
  n ← R 2⋆5+R 9
  shl ← 1⊸+•_while_{(R 2)∧0=1|n÷2⋆𝕩+1} 0
  sh ← (n÷2⋆shl)∾shl⥊2
  {𝕊: sh↩⟨⟩}⍟⊢ (n=1) ∧ 0=R 2
  
  ! n ≡ ×´sh
  v ← R ≠vn
  l ← sh ⥊ ⊢◶⟨
    {𝕊: (v⊑0∾2⋆6‿14‿30) -˜ n R 2⋆v⊑1‿7‿15‿31}
    {𝕊: ÷n R 0}
    {𝕊: @+n R (v-5)⊑256‿65536‿1114112}
  ⟩ v⊑vi
  
  vs ← v⊑vf
  vsLV ← ListVariations l
  
  { 𝕊 v0:
    a0 ← v0 Variation l
    ! a0 ≡ l
    { 𝕊:
      a1 ← 𝕩 Variation a0
      a0 ≡ a1?1;
      •Out "fail:"
      •Show v0‿"→"‿𝕩
      !0
    }¨ vs
  }¨ vs
  ClearRefs@
  eqlen+↩ (≠vs) ≡ +´⥊vs≡⌜vsLV
}¨ ↕count

•Out "Fraction of matching variation count to ListVariations: "∾(•Repr eqlen÷count)∾" (expected: ~0.999)"