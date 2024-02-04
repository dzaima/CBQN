%DEF file (
  path0 ← •path•file.At"../imports/"
  File ← {path0 •file.At 𝕩}
  CleanPath ← ! {r←•file.RealPath path0 ⋄ ≠i←/r ⍷ 𝕩? (⋈⁼i)(↑∾"..."∾(≠r)⊸+⊸↓)𝕩; 𝕩}∘•CurrentError∘@
)

(
  %USE file
  f ← File "show.bqn"
  r1 ← •Import f ⋄ ! +‿"show.bqn"‿⟨⟩ ≡ 1↓r1
  r2 ← •Import f ⋄ ! +‿"show.bqn"‿⟨⟩ ≡ 1↓r2
  ! r1 ≡○⊑ r2
  re ← •ReBQN{primitives⇐⟨'+'‿-⟩}
  import2 ← RE "•Import"
  r3 ← Import2 f ⋄ ! -‿"show.bqn"‿⟨⟩ ≡ 1↓r3
  r4 ← Import2 f ⋄ ! -‿"show.bqn"‿⟨⟩ ≡ 1↓r4
  ! (⊑¨r1‿r3) ≡ ⍷⊑¨r1‿r2‿r3‿r4
)

!"•Import: cyclic import of "".../cyclic.bqn""" % %USE file ⋄ •Import⎊CleanPath File "cyclic.bqn"
!"•Import: cyclic import of "".../cyclic.bqn""" % %USE file ⋄ •Import⎊CleanPath File "cyclic.bqn"
!"•Import: cyclic import of "".../cyclic.bqn""" % %USE file ⋄ ⟨⟩•Import⎊CleanPath File "cyclic.bqn"
!"•Import: cyclic import of "".../cyclic_A.bqn""" % %USE file ⋄ •Import⎊CleanPath File "cyclic_A.bqn"
!"•Import: cyclic import of "".../cyclic_B.bqn""" % %USE file ⋄ •Import⎊CleanPath File "cyclic_B.bqn"
!"•Import: cyclic import of "".../cyclic_A.bqn""" % %USE file ⋄ ⟨⟩•Import⎊CleanPath File "cyclic_B.bqn"
!"•Import: cyclic import of "".../cyclic_B.bqn""" % %USE file ⋄ ⟨⟩•Import⎊CleanPath File "cyclic_A.bqn"
%USE file ⋄ 3 •Import File "notCyclic.bqn" %% 30
