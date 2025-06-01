# tests scan on a matrix checking for overflow in tail elements
8‿16‿32 {e𝕊n‿w: m←2⋆e-2 ⋄ ! (m⊸+⌾(⟨¯1,w|n⟩⊸⊑) ⌽‿w⥊(n+w+1)⥊m) ≡ +` •internal.Squeeze ↑‿w⥊ (w⥊m)∾(n⥊0)∾m}⌜ 1+↕200‿65

%DEF rand R ← ((⊑•args).GetRand@).Range ⋄ ⟨V⇐Variation⟩ ← •internal

# ∨`bitarr
(
  %USE rand
  {𝕊:
   n ← 1 + R 2⋆R 13
   ! (∨` ≡ ⊢∘∨`) "Ab" V {𝕊: 1⌾((R n)⊸⊑)𝕩}⍟2 "Ai8" V 0⥊˜ n
  }¨ ↕100000
)

# +`bitarr
(
  %USE rand
  RB2 ← {¬⍟(R 2) "Ab" V 𝕩 R 1+R 2⋆R 10} # random boolean array with random uniform probability
  {𝕊:
    a ← RB2 R 2⋆R 13
    ! (+` ≡ ⊢∘+`) a
  }¨ ↕100000
)
