#! /usr/bin/env dbqn
"usage: test.bqn path/to/mlochbaum/BQN ""$PATH"" [optional expressions to evaluate]"!2≤≠•args
path←0⊑•args
envP←1⊑•args

tests ← •FLines path∾"/test/cases/prim.bqn"
# tests ← •FLines path∾"/test/cases/identity.bqn"
# tests ← •FLines path∾"/test/cases/under.bqn"
# tests ← •FLines path∾"/test/cases/fill.bqn"
# tests ← •FLines "primLeft.bqn"
{tests↩𝕩}⍟(×≠) 2↓•args

('#'≠ ·⊑ ∾⟜"#")◶@‿{
  '%'⊸∊◶{𝕤
    •Out 𝕩
    "src/interp" •FChars ⟨1,path,𝕩⟩ •Import "cc.bqn"
    (×⊑)◶@‿{𝕤⋄•Out "############ Failed to compile! ############" ⋄ •Out¨1↓𝕩}{env⇐<"PATH="∾envP}•SH"./debugBuild"
    code‿out‿err←•SH"./BQN"
    •Out out
    {𝕤⋄•Out"exit code "∾(⍕code) ⋄ •Out err}⍟(×code) err
  }‿{𝕤
    # •Out 𝕩
    # "src/interp" •FChars ⟨1,path,3↓𝕩⟩ •Import "cc.bqn"
    # (×⊑)◶@‿{𝕤⋄•Out "############ Failed to compile! ############" ⋄ •Out¨1↓𝕩}{env⇐<"PATH="∾envP}•SH"./debugBuild"
    # code‿out‿err←•SH"./BQN"
    # •Out out
    # {𝕤⋄•Out"exit code "∾(⍕code) ⋄ •Out err}⍟(×code) err
  }
}¨tests