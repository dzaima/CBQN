#! /usr/bin/env dbqn
"Usage: ./test.bqn path/to/mlochbaum/BQN [-sme] name | ./BQN"!2≤≠•args
path←⊑•args
args←1↓•args
opt←""
{𝕊: opt↩𝕩 ⋄ args↓˜↩1}⍟('-'≡⊑) ⊑args

"Missing argument!"!1=≠args
case←⊑args
case∾↩".bqn"
tests ← •FLines path∾"/test/cases/"∾case
tests/˜↩ (('#'≢⊑)∧0<≠)¨tests
{𝕊: tests/˜↩{"! %"≢3↑𝕩}¨tests}⍟⊑ 's'∊opt
{𝕊: tests/˜↩{"! %"≡3↑𝕩}¨tests}⍟⊑ 'e'∊opt
Out ← {•Out'%'⊸∊◶⊢‿{𝕩↓˜1+⊑𝕩⊐'%'}𝕩}

((⊑'m'∊opt)⊑Out‿{•Show𝕩⋄Out𝕩})¨tests
# ('%'⊸∊∨ '#'= ·⊑ ∾⟜"#")◶•Out‿@¨tests
•Out """finished"""
•Out ""