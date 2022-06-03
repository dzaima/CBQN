#! /usr/bin/env dbqn
"Usage: ./test.bqn path/to/mlochbaum/BQN [-qsmevw] name | ./BQN"!2≤≠•args
# q - no "finished" at end
# s - only non-erroring
# e - only erroring
# v - verify that the result is the expected one
# w - wrap each expression in {}s so it can be a single compilation unit
path←•wdpath•file.At ⊑•args
args←1↓•args
opt←""
{𝕊: opt↩𝕩 ⋄ args↓˜↩1}⍟('-'≡⊑) ⊑args
verify  ← ⊑'v'∊opt
wrap    ← ⊑'w'∊opt
onlyErr ← ⊑'e'∊opt
onlyReg ← ⊑'s'∊opt

"Missing argument!"!0<≠args
{
  case ← 𝕩∾".bqn"
  tests ← •FLines path∾"/test/cases/"∾case
  tests/˜↩ (("#"≢1⊸↑)∧0<≠)¨tests
  {𝕊: tests/˜↩'!'≠⊑¨tests}⍟⊢ onlyReg
  {𝕊: tests/˜↩'!'=⊑¨tests}⍟⊢ onlyErr
  RMC ← ⊐⟜'#'⊑⊸↑⊢ # doesn't work for token.bqn but whatever
  Q ← {
    i←⊑𝕩⊐'%'
    ((1+'!'≠⊑𝕩)×verify)◶⟨(1+i)⊸↓ ⋄ {∾ ⟨"""should've errored!""∘•BQN⎊1 """ ⋄ {𝕩/˜1+𝕩='"'}𝕩↓˜1+⊑𝕩⊐'%' ⋄ """"⟩} ⋄ {∾⟨"""incorrect!""‿1⊑˜(" ⋄ i↑𝕩 ⋄ ") ≡ {" ⋄ (1+i)↓𝕩 ⋄ " }"⟩}⟩ RMC 𝕩
  }
  Out ← {•Out(⊑'%'⊸∊)◶⟨{∾⟨"""incorrect!""‿1⊑˜1≡{"⋄RMC𝕩⋄" }"⟩}⍟verify {∾⟨"{"⋄RMC𝕩⋄"}"⟩}⍟wrap ⋄ Q⟩ 𝕩}

  ((⊑'m'∊opt)⊑Out‿{•Show𝕩⋄Out𝕩})¨tests
  # ('%'⊸∊∨ '#'= ·⊑ ∾⟜"#")◶•Out‿@¨tests
}¨args
•Out⍟(¬⊑'q'∊opt) """finished"""
•Out⍟(¬⊑'q'∊opt) ""