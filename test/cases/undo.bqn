<⁼ <"foo" %% "foo"
!"<⁼: Argument wasn't a rank 0 array" % <⁼ 4
!"<⁼: Argument wasn't a rank 0 array" % <⁼ ↕0
!"<⁼: Argument wasn't a rank 0 array" % <⁼ "f"

⋈⁼ ⋈"foo" %% "foo"
⋈⁼ "f" %% 'f'
!"⋈⁼: Argument wasn't a length-1 list" % ⋈⁼ 4
!"⋈⁼: Argument wasn't a length-1 list" % ⋈⁼ ↕0
!"⋈⁼: Argument wasn't a length-1 list" % ⋈⁼ ≍"f"

!"⌽: Argument cannot be a unit" % ⌽⁼ 0
⌽⁼ ↕10 %% 9-↕10

!"⍉⁼: 𝕩 must not be an atom" % ⍉⁼ 3
⍉⁼ <3 %% <3
