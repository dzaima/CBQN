# •Show •args

# resolve arguments & files
siDirR‿srcfileR‿resfileR‿buildDir ← •args
filename ← (⊐⟜'/'↑⊢)⌾⌽ srcfileR
At ← •wdpath⊸•file.At
siDir ← At siDirR

srcfile ← At srcfileR
resfile ← At resfileR
tmpfile ← (At buildDir) •file.At filename∾".d.orig"
depfile ← (At buildDir) •file.At filename∾".d"
prefix  ← "si_"∾ (∧`'.'⊸≠)⊸/ •file.Name srcfile

# invoke singeli, which creates the result file
(⟨"-d" ⋄ tmpfile ⋄ "-o" ⋄ resfile ⋄ "-n" ⋄ prefix ⋄ srcfile⟩) •Import siDir •file.At "singeli"

# output dependency file
deps ← siDir⊸•file.At¨ •file.Lines tmpfile
lnln ← @+10‿10
depfile •file.Chars ∾⟨resfileR ⋄ ": " ⋄ srcfile ⋄ ∾' '⊸∾¨deps ⋄ lnln ⋄ ∾ ∾⟜(":"∾lnln)¨ deps⟩

