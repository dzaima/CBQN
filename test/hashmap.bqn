u ← ⌊100×(•UnixTime+1|100×•MonoTime)@
ro← •MakeRand •Show u
R ← ro.Range

err ← 1  # Run operations that should error

Test ← {iter 𝕊 n:
  # Global set of keys that might be used
  keys ← ⊒⊸(-⊸∾⍟(0<⊣)¨) (R⟜15 R¨ 1-˜1e6⌊∘⋆R⟜0) n
  # Which keys are currently set, and count
  c ← +´ mask ← 0 = n R 20+R 10
  # Value for key if set
  vals ← ((R⟜≠⊏⊢)⟜⊢‿÷‿⥊‿↕ {𝕎𝕩}¨ R⟜10) n
  
  map ← keys •HashMap○(mask⊸/) vals
  
  # Single operations and checks
  _er ← { err ? ! 0∘𝔽⎊1 𝕩⊑keys ; 1 }
  Cnt ← {𝕊: ! c ≡ map.Count@ }
  Key ← {𝕊: ! (mask/keys) ≡○∧ map.Keys@ }
  Val ← {𝕊: ! (mask/vals) ≡○∧ map.Values@ }
  Has ← { ! (𝕩⊑mask) ≡ map.Has 𝕩⊑keys }
  Get ← { 𝕩⊑mask ? ! (𝕩⊑vals) ≡ map.Get 𝕩⊑keys ; map.Get _er 𝕩 }
  Set ← { 𝕨 map.Set˜ 𝕩⊑keys ⋄ vals 𝕨˙⌾(𝕩⊸⊑)↩ ⋄ mask {c+↩¬𝕩⋄1}⌾(𝕩⊸⊑)↩ ⋄ @ }
  Del ← { 𝕩⊑mask ? map.Delete 𝕩⊑keys ⋄ mask 0⌾(𝕩⊸⊑)↩ ⋄ c-↩1 ; map.Delete _er 𝕩 }
  
  Selection ← { (𝕨≤1)◶⟨R·⌊1.5⊸×,𝕨⟩⊸⌊⊸ro.Deal∘≠⊸⊏ 𝕩 }
  RandVal ← R∘100
  ops ← ⟨Cnt, Has, Get, Del, (R∘n⊑vals˙)Set⊢, RandVal⊸Set⟩
  
  # Larger sets: argument is number of ops (sometimes overridden)
  FullCheck ← Key∘Val∘Cnt
  Deletes  ← { Del¨          𝕩 Selection  mask }
  Restores ← { ⊏⟜vals⊸(Set¨) 𝕩 Selection ¬mask }
  RandomOps ← (R⟜≠⊏⊢)⟜ops {𝕎𝕩}¨ R⟜n
  RandomSame ← R∘≠⊸⊑∘ops {𝕎𝕩}¨ R⟜n
  opSets ← FullCheck‿Deletes‿Restores‿RandomOps‿RandomSame
  ((R⟜≠⊏⊢)⟜opSets {𝕎𝕩}¨ R⟜100) iter
}

1e5 Test 1e2
1e4 Test 1e4
