u ← ⌊100×(•UnixTime+1|100×•MonoTime)@

Range ← (•MakeRand •Show u).Range

ExpectedRangeStep ← { n‿len𝕊depth‿max:
  ls ← {𝕊:len Range max}¨↕n
  maxGot ← ⌈´⌈´¨ls
  ! maxGot<max⌈1
  {𝕊: ! 0<⌊´⌊´¨ls}⍟⊢ max=0
  {(max=0) ∨ (max>n×len÷20) ∨ maxGot=max-1? @; •Out ∾⟨"Maximum ", •Repr max-1, " likely should have been encountered, but max was ", •Repr maxGot, "; n=", •Repr n," len=", •Repr len, " range=", •Repr max⟩}
  
  # some very very crappy statistics
  sumExp ← n×len÷2
  sumGot ← +´ +´¨ ls
  {𝕊: sumGot ↩ max÷˜ (2÷˜n×len) + sumGot}⍟⊢ max≠0
  diff ← sumGot-sumExp
  dev ← diff ÷ √12÷˜ n×len
  {𝕊:
    depth≥4? •Out ∾⟨"Unlikely sum (dev=", •Repr dev, ": expected ", •Repr sumExp, ", got ", •Repr sumGot, "; n=", •Repr n," len=", •Repr len, " range=", •Repr max⟩;
    n‿len ExpectedRangeStep ⟨depth+1, max⟩
  }⍟⊢ 2 < |dev
  dev
}

ExpectedRange ← ExpectedRangeStep⟜(0⊸⋈)

# for whatever reason, the following don't all evaluate to the same number; but they're close enough so ¯\_(ツ)_/¯
# •Show (+´÷≠) |{𝕊: 100‿1000 ExpectedRange 2  }¨↕10000
# •Show (+´÷≠) |{𝕊:  10‿1000 ExpectedRange 3  }¨↕10000
# •Show (+´÷≠) |{𝕊:  10‿1000 ExpectedRange 1e3}¨↕10000
# •Show (+´÷≠) |{𝕊:  10‿1000 ExpectedRange 1e6}¨↕10000

am ← 10000
•Out "general"
(100‿128∾˜↕20) {(⌊(10×am)÷𝕨⌈1)‿𝕨 ExpectedRange 𝕩}⌜ 3‿10‿100‿1000‿10000‿100000‿1e8‿1e15∾2⋆2↓↕34
(↕10) {! ∧´0=𝕩 Range 1}⌜ ↕200

•Out "bit boolean"
{⟨5×am,𝕩⟩ ExpectedRange 2}¨ ⥊31‿32‿33×⌜↕100
•Out "float"
{⟨2×am,𝕩⟩ ExpectedRange 0}¨ ↕100
