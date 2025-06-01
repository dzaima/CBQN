%DEF eqvar ⟨_eqvar⟩←⊑•args

(
  %USE eqvar
  helpers ← ⊑•args
  rand ← helpers.GetRand@
  R ← rand.Range
  ⟨RandVals⟩ ← ⟨rand⟩ helpers.Import "utils.bqn"
  {𝕊:
    t ← R 8
    l ← R 10
    x ← l RandVals t
    w ← ¯1+l R 1+l
    w ⊔_eqvar x
  }¨ ↕10000
)