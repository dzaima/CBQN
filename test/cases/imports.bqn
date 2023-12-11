%DEF file (
  path0 ← •path•file.At"../imports/"
  File ← {path0 •file.At 𝕩}
  CleanPath ← ! {r←•file.RealPath path0 ⋄ ≠i←/r ⍷ 𝕩? (⋈⁼i)(↑∾"..."∾(≠r)⊸+⊸↓)𝕩; 𝕩}∘•CurrentError∘@
)
(
  %USE file
  i1 ← •Import File "show.bqn" ⋄ ! "show.bqn"‿⟨⟩ ≡ 1↓i1
  i2 ← •Import File "show.bqn" ⋄ ! "show.bqn"‿⟨⟩ ≡ 1↓i2
  ! i1 ≡ i2
)

!"•Import: cyclic import of "".../cyclic.bqn""" % %USE file ⋄ •Import⎊CleanPath File "cyclic.bqn"
!"•Import: cyclic import of "".../cyclic.bqn""" % %USE file ⋄ •Import⎊CleanPath File "cyclic.bqn"
!"•Import: cyclic import of "".../cyclic.bqn""" % %USE file ⋄ ⟨⟩•Import⎊CleanPath File "cyclic.bqn"
!"•Import: cyclic import of "".../cyclic_A.bqn""" % %USE file ⋄ •Import⎊CleanPath File "cyclic_A.bqn"
!"•Import: cyclic import of "".../cyclic_B.bqn""" % %USE file ⋄ •Import⎊CleanPath File "cyclic_B.bqn"
!"•Import: cyclic import of "".../cyclic_A.bqn""" % %USE file ⋄ ⟨⟩•Import⎊CleanPath File "cyclic_B.bqn"
!"•Import: cyclic import of "".../cyclic_B.bqn""" % %USE file ⋄ ⟨⟩•Import⎊CleanPath File "cyclic_A.bqn"
%USE file ⋄ 3 •Import File "notCyclic.bqn" %% 30