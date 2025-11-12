%DEF rand r ← (⊑•args).GetRand@

(
  %USE rand
  # ∾𝕩 not getting garbage
  ≠{𝕊: ! ∧´1=⥊∾⎊1 ⥊⟜1¨ (4 r.Range 3) r.Range¨ 4}¨↕10000
)
