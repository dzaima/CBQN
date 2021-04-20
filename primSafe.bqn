#! /usr/bin/env dbqn
"Usage: ./primSafe.bqn path/to/mlochbaum/BQN | ./BQN"!1≤≠•args
path←⊑•args
case←"prim"
{case↩⊑𝕩}⍟(×≠) 1↓•args
case∾↩".bqn"


('%'⊸∊∨ '#'= ·⊑ ∾⟜"#")◶•Out‿@¨•FLines path∾"/test/cases/"∾case
•Out """finished"""
•Out ""