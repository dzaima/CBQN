# note: tests only bit/i8/i16/i32 arrays; no f64/c8/c16/c32/generic array tests
u ← ⌊100×(•UnixTime+1|100×•MonoTime)@
R ← (•MakeRand •Show u).Range

M ← (2⋆32)⊸×⊸+´
Test ← { 𝕊:
  width ← 1‿8‿16‿32⊑˜R 4
  count ← 1+R (width=1)⊑32‿256
  data ← (width×count) R 2
  n ← 1+R count
  cast ← {width=1? ⊢; ⟨1⋄width‿'i'⟩ •bit._cast}
  iota ← ↕≠data
  shifts ← ↕1+count-n
  exp ← >(-shifts)(width/⌽)¨<n>↕count
  got ← >1↓¨∊¨<˘⍉>{
    a ← Cast data≠𝕩≠iota
    shifts (M∘•Hash n↑↓)¨ <a
  }¨¯1∾iota
  +´∘⥊¨ exp(>⋈<)got
}

fails ← 0‿0
{𝕊: fails+↩ Test@}¨↕1000
•Out ∾⟨"Collisions: " ⋄ •Repr 0⊑fails ⋄ " (expected 0, assuming no actual hash collisions)"⟩
•Out ∾⟨"Mismatches: " ⋄ •Repr 1⊑fails ⋄ " (expected 0)"⟩
