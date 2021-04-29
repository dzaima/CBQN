#! /usr/bin/env dbqn
"Usage: ./test.bqn path/to/mlochbaum/BQN [-qsmev] name | ./BQN"!2≤≠•args
path←⊑•args
args←1↓•args
opt←""
{𝕊: opt↩𝕩 ⋄ args↓˜↩1}⍟('-'≡⊑) ⊑args

"Missing argument!"!1=≠args
case←⊑args
case∾↩".bqn"
tests ← •FLines path∾"/test/cases/"∾case
tests/˜↩ (('#'≢⊑)∧0<≠)¨tests
{𝕊: tests/˜↩'!'≠⊑¨tests}⍟⊑ 's'∊opt
{𝕊: tests/˜↩'!'=⊑¨tests}⍟⊑ 'e'∊opt
Q ← {
  i←⊑𝕩⊐'%'
  (('!'≠⊑𝕩)∧⊑'v'∊opt)◶⟨(1+i)⊸↓ ⋄ {∾⟨"(" ⋄ i↑𝕩 ⋄ ") ≡ {" ⋄ (⊐⟜'#'⊑⊸↑⊢) (1+i)↓𝕩 ⋄ " }"⟩}⟩ 𝕩
}
Out ← {•Out'%'⊸∊◶⊢‿Q𝕩}

((⊑'m'∊opt)⊑Out‿{•Show𝕩⋄Out𝕩})¨tests
# ('%'⊸∊∨ '#'= ·⊑ ∾⟜"#")◶•Out‿@¨tests
•Out⍟(¬⊑'q'∊opt) """finished"""
•Out⍟(¬⊑'q'∊opt) ""