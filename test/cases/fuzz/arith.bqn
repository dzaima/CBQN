(
  helpers ← ⊑•args
  R ← (helpers.GetRand@).Range
  ⟨EEqual, Variation⟩ ⇐ •internal

  Mod ← {𝕊:
    modOptions ← ⟨∞,¯∞,0÷0⟩ ∾ ∾⟜- 2⋆↕34
    {𝕊:
      n ← R 2⋆4+R 6
      w ← R∘≠⊸⊑ modOptions
      x ← -⍟(R 2) (2⋆R 34) + (↕n)-R n
      got ← w|x
      exp ← w|"Ah" Variation x
      ¬exp EEqual got?
      •Out "𝕨: "∾•Repr w
      •Out "𝕩: "∾(•Repr x)∾" / "∾•internal.Info x
      •Out "exp: "∾•Repr exp
      •Out "got: "∾•Repr got
      !0
      ;@
    }¨ ↕𝕩
  }

  •Show "powerOfTwo | arr" ⋄ Mod 100000
)