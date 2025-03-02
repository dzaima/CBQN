<⁼ <"foo" %% "foo"
!"<⁼𝕩: Argument wasn't a rank 0 array" % <⁼ 4
!"<⁼𝕩: Argument wasn't a rank 0 array" % <⁼ ↕0
!"<⁼𝕩: Argument wasn't a rank 0 array" % <⁼ "f"

⋈⁼ ⋈"foo" %% "foo"
⋈⁼ "f" %% 'f'
!"⋈⁼𝕩: Argument wasn't a length-1 list" % ⋈⁼ 4
!"⋈⁼𝕩: Argument wasn't a length-1 list" % ⋈⁼ ↕0
!"⋈⁼𝕩: Argument wasn't a length-1 list" % ⋈⁼ ≍"f"

≍⁼ ⋈"foo" %% <"foo"
≍⁼ ⋈5 %% <5
≍⁼ 1‿2‿3⥊↕6 %% 2‿3⥊↕6
≍⁼ 1‿2⥊↕2 %% ↕2
!"≍⁼𝕩: Argument must have a leading axis of 1" % ≍⁼ 5
!"≍⁼𝕩: Argument must have a leading axis of 1" % ≍⁼ <"foo"
!"≍⁼𝕩: Argument must have a leading axis of 1" % ≍⁼ "foo"
!"≍⁼𝕩: Argument must have a leading axis of 1" % ≍⁼ 3‿1‿1⥊<"foo"

!"⌽𝕩: 𝕩 cannot be a unit" % ⌽⁼ 0
⌽⁼ ↕10 %% 9-↕10

!"⍉⁼𝕩: 𝕩 must not be an atom" % ⍉⁼ 3
!"𝕨⍉⁼𝕩: 𝕩 must not be an atom" % 0 ⍉⁼ 3
!"𝕨⍉⁼𝕩: Length of 𝕨 must be at most rank of 𝕩" % 0‿0 ⍉⁼ ↕3
!"𝕨⍉⁼𝕩: Axis 9 does not exist (1≡=𝕩)" % 9 ⍉⁼ ↕3
!"𝕨⍉⁼𝕩: Axis 9 does not exist (3≡=𝕩)" % 9 ⍉⁼ 3‿3‿3⥊3
!"𝕨⍉⁼𝕩: 𝕨 must have rank at most 1" % (1‿1⥊0) ⍉⁼ ↕3
⍉⁼ <3 %% <3
