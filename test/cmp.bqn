noerr←≠•args
•Out⍟noerr "Not executing tests that would error"
fns ← =‿≠‿<‿≤‿>‿≥
n ← 0÷0
nn ← -n
arrs ← •internal.Squeeze¨ ⟨
  "hello","{⍉⍵}","{⍉𝕩}"
  @+0‿1‿2‿126‿127
  @+0‿1‿2‿126‿127‿128‿129
  @+0‿1‿2‿65533‿65534‿65535
  "",↕0
  1‿2‿3 ⋄ ¯1‿0‿1 ⋄ ↕10 ⋄ ¯5+↕10 ⋄ 1‿1.5‿0‿0‿4‿π ⋄ -1‿1.5‿0‿0‿4‿π ⋄ n‿nn‿n‿n‿nn∾(-0‿2‿π)∾1‿2‿5‿0‿0÷0‿0‿3‿5‿0
  ¯128‿¯1‿0‿1‿127
  ¯32768‿¯1‿0‿1‿32767
  ¯2147483648‿¯1‿0‿1‿2147483647
  ¯2147483648‿¯1‿0‿1‿2147483648
  63 (•MakeRand 2).Range 2
  64 (•MakeRand 2).Range 2
  65 (•MakeRand 2).Range 2
  90 (•MakeRand 2).Range 2
⟩
atms ← 1‿¯1‿0‿¯0‿∞‿¯∞‿n‿nn‿@‿'l'‿'⍉'‿'𝕩'‿{a⇐1}‿+‿{𝔽}
atms∾↩ -⊸∾ 10⋆↕20
atms∾↩ -⊸∾ ⥊(⥊¯0.9‿¯0.1‿0‿0.1‿0.9+⌜2-↕5) +⌜ 2⋆↕35
atms∾↩ @+⥊128‿65536+⌜2-↕5

LV ← •internal.ListVariations
V ← •internal.Variation
C ← •internal.ClearRefs
ElType ← •internal.ElType

NEas ← ((⊑¨∊=‿≠˙) ∨ 8≠·ElType 2⊑¨⊢)⊸/

•Out "AS"
{
fn‿arr‿atm:
  C@ ⋄ ∧´(⊏≡¨⊢) {(𝕩 V arr) Fn⎊"err" atm}¨ LV arr?0;
fn‿arr‿atm:
  •Out "Fail:"
  •Show arr
  •Show fn
  •Show atm
  •Show ⍉(LV arr)≍{(𝕩 V arr) Fn⎊"err" atm}¨ LV arr
  •Exit 1
}¨ NEas⍟noerr ⥊(<⟨⟩)<⊸∾⌜´ fns‿arrs‿atms

•Out "AA"
{
fn‿l‿r:
  C@
  lr←(⌈´≠¨l‿r)↑¨l‿r
  ∧´(⊏≡¨⊢) {fn⎊"err"˝𝕩 V¨ lr}¨ ⥊≍○<⌜´ LV¨lr?0;
  𝕩!0
}¨ (⥊(<⟨⟩)<⊸∾⌜´ ⟨fns⟩ ∾ 2⥊< (0<≠¨)⊸/arrs) ∾ ⥊(<⟨⟩)<⊸∾⌜´ ⟨fns⟩ ∾ 2⥊<⟨"" ⋄ ↕0 ⋄ ⟨⟩⟩