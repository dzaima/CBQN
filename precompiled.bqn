#! /usr/bin/env dbqn
"Usage: ./precompiled.bqn path/to/mlochbaum/BQN ""$PATH"" [expressions, else prim tests]"!2≤≠•args
path←0⊑•args
envP←1⊑•args

tests ← •FLines path∾"/test/cases/prim.bqn"
# tests ← •FLines path∾"/test/cases/identity.bqn"
# tests ← •FLines path∾"/test/cases/undo.bqn"
# tests ← •FLines path∾"/test/cases/under.bqn"
# tests ← •FLines path∾"/test/cases/fill.bqn"
{tests↩𝕩}⍟(×≠) 2↓•args

('#'≠ ·⊑ ∾⟜"#")◶@‿{
  (⊑'%'⊸∊)◶{𝕤
    •Out 𝕩
    "src/gen/interp" •FChars ⟨1,path,𝕩⟩ •Import "cc.bqn"
    # make ← "make"‿"singeli=1"‿"f=-DPRECOMP -march=native"‿"c"
    # make ← "make"‿"f=-DPRECOMP -m32"‿"lf=-m32"‿"c"
    make ← "make"‿"f=-DPRECOMP"‿"c"
    (×⊑)◶@‿{𝕤⋄•Out "############ Failed to compile! ############" ⋄ •Out¨1↓𝕩}{env⇐<"PATH="∾envP}•SH make
    code‿out‿err←•SH⟨"./BQN"⟩
    •Out out
    {𝕤⋄•Out"exit code "∾(•Repr code) ⋄ •Out err}⍟(×code) err
  }‿⊢
}¨tests