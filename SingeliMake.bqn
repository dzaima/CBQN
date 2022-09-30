# •Show •args

# resolve arguments & files
siDirR‿srcfileR‿resfileR‿buildDir ← •args
filename ← (⊐⟜'/'↑⊢)⌾⌽ srcfileR
siDir ← •file.At siDirR

srcfile ← •file.At srcfileR
resfile ← •file.At resfileR
tmpfile ← (•file.At buildDir) •file.At filename∾".d.orig"
depfile ← (•file.At buildDir) •file.At filename∾".d"
prefix  ← "si_"∾ (∧`'.'⊸≠)⊸/ •file.Name srcfile

# invoke singeli, which creates the result file
(⟨"-d" ⋄ tmpfile ⋄ "-o" ⋄ resfile ⋄ "-n" ⋄ prefix ⋄ srcfile⟩) •Import siDir •file.At "singeli"

# output dependency file
deps ← siDir⊸•file.At¨ •file.Lines tmpfile
lnln ← @+10‿10
depfile •file.Chars ∾⟨resfileR ⋄ ": " ⋄ srcfile ⋄ ∾' '⊸∾¨deps ⋄ lnln ⋄ ∾ ∾⟜(":"∾lnln)¨ deps⟩

