u ← ⌊100×(•UnixTime+1|100×•MonoTime)@
Range ← (•MakeRand •Show u).Range

Do ← {𝕊:
  orig ← (1000⌊𝕩) Range 2
  s ← Range 1+≠orig
  mut ← "Ab"•internal.Variation orig
  l ← Range (1+≠orig)-s
  v ← Range 2
  mut •internal.Temp ⟨0,s,0,l⟩
  exp ← 0¨⌾((s+↕l)⊸⊏) orig
  mut ≢ exp?
    •Show '0'+orig
    •Show '0'+mut
    •Show '0'+exp
    •Show s‿l‿v
    •Exit 1;
  𝕩
}

Do¨ ↕100000