#! /usr/bin/env dbqn
"usage: ./primSafe.bqn path/to/mlochbaum/BQN | ./BQN (while compiled for REPL mode)"!1≤≠•args
path←⊑•args

('%'⊸∊∨ '#'= ·⊑ ∾⟜"#")◶•Out‿@¨•FLines path∾"/test/cases/prim.bqn"
•Out """finished"""
•Out ""