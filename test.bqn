#! /usr/bin/env dbqn
"Usage: ./test.bqn path/to/mlochbaum/BQN [-qsmev] name | ./BQN"!2≤≠•args
path←⊑•args
args←1↓•args
opt←""
{𝕊: opt↩𝕩 ⋄ args↓˜↩1}⍟('-'≡⊑) ⊑args

"Missing argument!"!0<≠args
{
  case ← 𝕩∾".bqn"
  tests ← •FLines path∾"/test/cases/"∾case
  tests/˜↩ (('#'≢⊑)∧0<≠)¨tests
  {𝕊: tests/˜↩'!'≠⊑¨tests}⍟⊑ 's'∊opt
  {𝕊: tests/˜↩'!'=⊑¨tests}⍟⊑ 'e'∊opt
  verify ← ⊑'v'∊opt
  RMC ← ⊐⟜'#'⊑⊸↑⊢ # doesn't work for token.bqn but whatever
  Q ← {
    i←⊑𝕩⊐'%'
    ((1+'!'≠⊑𝕩)×verify)◶⟨(1+i)⊸↓ ⋄ {∾ ⟨"""should've errored!""∘•BQN⎊1 """ ⋄ {𝕩/˜1+𝕩='"'}𝕩↓˜1+⊑𝕩⊐'%' ⋄ """"⟩} ⋄ {∾⟨"""incorrect!""‿1⊑˜(" ⋄ i↑𝕩 ⋄ ") ≡ {" ⋄ (1+i)↓𝕩 ⋄ " }"⟩}⟩ RMC 𝕩
  }
  Out ← {•Out'%'⊸∊◶(verify◶⊢‿{∾⟨"""incorrect!""‿1⊑˜1≡{" ⋄ RMC 𝕩 ⋄ " }"⟩})‿Q𝕩}

  ((⊑'m'∊opt)⊑Out‿{•Show𝕩⋄Out𝕩})¨tests
  # ('%'⊸∊∨ '#'= ·⊑ ∾⟜"#")◶•Out‿@¨tests
}¨args
•Out⍟(¬⊑'q'∊opt) """finished"""
•Out⍟(¬⊑'q'∊opt) ""