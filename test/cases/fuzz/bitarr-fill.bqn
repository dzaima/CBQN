### WHOLE-FILE-TEST
helpers ← ⊑•args
Range ← (helpers.GetRand@).Range

Do ← {𝕊:
  orig ← (1000⌊𝕩) Range 2
  s ← Range 1+≠orig
  mut ← "Ab"•internal.Variation orig
  l ← Range (1+≠orig)-s
  v ← Range 2
  201 •internal.Temp ⟨0,s,0,l,mut⟩
  exp ← 0¨⌾((s+↕l)⊸⊏) orig
  mut ≢ exp?
    •Show '0'+orig
    •Show '0'+mut
    •Show '0'+exp
    •Show s‿l‿v
    !0;
  𝕩
}

Do¨ ↕100000