%DEF file (
  ⟨AtRoot, Import⟩ ← ⊑•args
  File ← AtRoot
  CleanPath ← ! {r←•file.RealPath AtRoot"" ⋄ ≠i←/r ⍷ 𝕩? (⋈⁼i)(↑∾"..."∾(≠r)⊸+⊸↓)𝕩; 𝕩}∘•CurrentError∘@
)

(
  %USE file
  f ← File "imports/show.bqn"
  r1 ← •Import f ⋄ ! +‿"show.bqn"‿⟨⟩ ≡ 1↓r1
  r2 ← •Import f ⋄ ! +‿"show.bqn"‿⟨⟩ ≡ 1↓r2
  ! r1 ≡○⊑ r2
  re ← •ReBQN{primitives⇐⟨'+'‿-⟩}
  import2 ← RE "•Import"
  r3 ← Import2 f ⋄ ! -‿"show.bqn"‿⟨⟩ ≡ 1↓r3
  r4 ← Import2 f ⋄ ! -‿"show.bqn"‿⟨⟩ ≡ 1↓r4
  ! (⊑¨r1‿r3) ≡ ⍷⊑¨r1‿r2‿r3‿r4
)

!"•Import 𝕩: cyclic import of "".../imports/cyclic.bqn""" % %USE file ⋄ Import⎊CleanPath "imports/cyclic.bqn"
!"•Import 𝕩: cyclic import of "".../imports/cyclic.bqn""" % %USE file ⋄ Import⎊CleanPath "imports/cyclic.bqn"
!"•Import 𝕩: cyclic import of "".../imports/cyclic.bqn""" % %USE file ⋄ ⟨⟩Import⎊CleanPath "imports/cyclic.bqn"
!"•Import 𝕩: cyclic import of "".../imports/cyclic_A.bqn""" % %USE file ⋄ Import⎊CleanPath "imports/cyclic_A.bqn"
!"•Import 𝕩: cyclic import of "".../imports/cyclic_B.bqn""" % %USE file ⋄ Import⎊CleanPath "imports/cyclic_B.bqn"
!"•Import 𝕩: cyclic import of "".../imports/cyclic_A.bqn""" % %USE file ⋄ ⟨⟩Import⎊CleanPath "imports/cyclic_B.bqn"
!"•Import 𝕩: cyclic import of "".../imports/cyclic_B.bqn""" % %USE file ⋄ ⟨⟩Import⎊CleanPath "imports/cyclic_A.bqn"
%USE file ⋄ 3 Import "imports/notCyclic.bqn" %% 30
