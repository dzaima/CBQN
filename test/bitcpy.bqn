# make f='-O3 -march=native -DTEST_BITCPY' singeli=1 c && ./BQN test/bitcpy.bqn
⟨Temp, Type, Unshare, Squeeze, Info, Variation⟩ ← •internal
u ← ⌊100×(•UnixTime+1|100×•MonoTime)@
# u ↩ 123

r ← •MakeRand •Show u


Do ← { 𝕊:
  al ← 1+r.Range 500
  a ← al r.Range 2
  aT ← "Ai32" Variation a
  ! a ≡ aT
  ! "bitarr" ≡ Type a
  b ← al r.Range 2
  ! "bitarr" ≡ Type b
  { 𝕊:
    bl ← r.Range 1+al
    as ← r.Range 1+al-bl
    bs ← r.Range 1+al-bl
    i ← ↕bl
    exp ← ((bs+i)⊏b)⌾((as+i)⊸⊏) a
    aC ← Unshare a
    Temp ⟨aC, as, b, bs, bl⟩
    aC ≢ exp?
      •Out "Fail:"
      F ← {(¬ ·∧`⌾⌽ ' '⊸=)⊸/ ⥊' '∾˘˜ ↑‿64⥊ '0'+𝕩}
      •Out "Exp: "∾F exp
      •Out "Got: "∾F aC
      •Out "Inputs:"
      •Out "r:   "∾F a
      •Out "rs:  "∾•Repr as
      •Out "x:   "∾F b
      •Out "xs:  "∾•Repr bs
      •Out "l:   "∾•Repr bl
      •Exit 1
    ;0
  }∘@⍟500 @
  ! a ≡ aT
}

@∘Do⍟(⊑(•BQN¨•args)∾2000) @