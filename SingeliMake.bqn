# •Show •args

# resolve arguments & files
siDirR‿srcfileR‿resfileR‿buildDir ← •args
siDir ← •file.At siDirR
tmpfile ← •file.At "obj/singeliDeps"
srcfile ← •file.At srcfileR
resfile ← •file.At resfileR
depfile ← (•file.At buildDir) •file.At ".d"∾˜ (⊐⟜'/'↑⊢)⌾⌽ srcfile

# invoke singeli, which creates the result file
(⟨"-d" ⋄ tmpfile ⋄ "-o" ⋄ resfile ⋄ srcfile⟩) •Import siDir •file.At "singeli"

# output dependency file
deps ← siDir⊸•file.At¨ •file.Lines tmpfile
lnln ← @+10‿10
depfile •file.Chars ∾⟨resfileR ⋄ ": " ⋄ srcfile ⋄ ∾' '⊸∾¨deps ⋄ lnln ⋄ ∾ ∾⟜(":"∾lnln)¨ deps⟩

