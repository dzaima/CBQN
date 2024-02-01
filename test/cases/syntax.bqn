%DEF jiteq _jiteq ← {r ← ⍷ (1⋈𝔽)⎊{𝕊: 0⋈•CurrentError@}¨ 4⥊<𝕩 ⋄ "TEST FAIL: all results weren't equal" ! 1=≠r ⋄ {0‿e: !e; 1‿e: e}⊑r}

# body usage
a←"zero"‿"one" ⋄ !a≡a⊸{𝕩?1⊑𝕨;0⊑𝕨}¨0‿1
!"No header matched argument" % { 𝕊a‿b: a+b ; d𝕊a‿b‿c: a+b+c} ↕⥊3
!"No header matched argument" % {d𝕊a‿b: a+b ;  𝕊a‿b‿c: a+b+c} ↕⥊2
{𝕊a‿b: a+b ; 𝕊a‿b‿c: a+b+c} ↕⥊2 %% ⟨1⟩
{𝕊a‿b: a+b ; 𝕊a‿b‿c: a+b+c} ↕⥊3 %% ⟨3⟩
{4=⊑𝕩? "four" ; "idk: "‿𝕩}4+↕3 %% "four"
{4=⊑𝕩? "four" ; "idk: "‿𝕩}3+↕3 %% "idk: "‿⟨3,4,5⟩
{𝕩+↩1 ⋄ 4=⊑𝕩? 1‿𝕩 ; 2‿𝕩}3+↕3 %% ⟨1,4‿5‿6⟩
{𝕩+↩1 ⋄ 4=⊑𝕩? 1‿𝕩 ; 2‿𝕩}4+↕3 %% ⟨2,4‿5‿6⟩
{𝕩↩𝕩+1 ⋄ 4=⊑𝕩? 1‿𝕩 ; 2‿𝕩}3+↕3 %% ⟨1,4‿5‿6⟩
{𝕩↩𝕩+1 ⋄ 4=⊑𝕩? 1‿𝕩 ; 2‿𝕩}4+↕3 %% ⟨2,4‿5‿6⟩
{1?"abc" ; "def"} %% "abc"
{0?"abc" ; "def"} %% "def"
{123456 ⋄ 1‿{2} ⋄ ⟨1⟩ ⋄ 1‿{1+5}} %% 1‿6

# forks with funky tines
{(𝕩⊏⟨⋈ ⋄ "ab" ⋄ 9⟩) {f‿g‿h←𝕨 ⋄ T←F G H ⋄ ⟨T⋈𝕩 ⋄ (≍𝕩‿𝕩)T⋈𝕩⟩} ↕3}¨ ↕2‿2‿2 %% 2‿2‿2⥊⟨⟨⟨⟨⟨0‿1‿2⟩⟩,⟨⟨0‿1‿2⟩⟩⟩,⟨⟨1‿2⥊⟨0‿1‿2,0‿1‿2⟩,⟨0‿1‿2⟩⟩,⟨1‿2⥊⟨0‿1‿2,0‿1‿2⟩,⟨0‿1‿2⟩⟩⟩⟩,⟨⟨⟨⟨0‿1‿2⟩⟩,"ab"⟩,⟨⟨1‿2⥊⟨0‿1‿2,0‿1‿2⟩,⟨0‿1‿2⟩⟩,"ab"⟩⟩,⟨"ab","ab"⟩,⟨"ab","ab"⟩,⟨⟨"ab",⟨⟨0‿1‿2⟩⟩⟩,⟨"ab",⟨1‿2⥊⟨0‿1‿2,0‿1‿2⟩,⟨0‿1‿2⟩⟩⟩⟩,⟨⟨"ab","ab"⟩,⟨"ab","ab"⟩⟩,⟨"ab","ab"⟩,⟨"ab","ab"⟩⟩

# body inverses
!"No monadic undo header found for this block" % {𝕩}⁼ 1
!"No dyadic F⁼ undo header found for this block" % 1 {𝕩}⁼ 1
!"No dyadic F˜⁼ undo header found for this block" % 1 {𝕩}˜⁼ 1
!"No monadic undo header found for this 1-modifier block" % +{𝔽𝕩}⁼ 1
!"No dyadic F⁼ undo header found for this 1-modifier block" % 1 +{𝔽𝕩}⁼ 1
!"No dyadic F˜⁼ undo header found for this 1-modifier block" % 1 +{𝔽𝕩}˜⁼ 1
!"No monadic undo header found for this 2-modifier block" % +{𝔽𝔾𝕩}+⁼ 1
!"No dyadic F⁼ undo header found for this 2-modifier block" % 1 +{𝔽𝔾𝕩}+⁼ 1
!"No dyadic F˜⁼ undo header found for this 2-modifier block" % 1 +{𝔽𝔾𝕩}+˜⁼ 1

# assignment
!"Attempting to read variable which is not yet defined" % {a} ⋄ a←1
!"Attempting to read variable which is not yet defined" % {a⊢↩↕10} ⋄ a←2
!"Assignment: Attempting to modify variable which is not yet defined" % {a↩↕10} ⋄ a←2
!"Attempting to read variable which is not yet defined" % {a (↕10)˙↩} ⋄ a←2
!"Attempting to read variable which is not yet defined" % a←1 ⋄ {a‿b⊢↩↕2} ⋄ b←2
!"Assignment: Attempting to modify variable which is not yet defined" % a←1 ⋄ {a‿b↩↕2} ⋄ b←2
!"Attempting to read variable which is not yet defined" % a←1 ⋄ {a‿b (↕10)˙↩} ⋄ b←2
!"Assignment: Mismatched shape for spread assignment" % a‿b←1
!"Assignment: Mismatched shape for spread assignment" % a‿b←1‿2‿3
!"Assignment: Mismatched shape for spread assignment" % ⟨a⟩←<1
!"Assignment: Mismatched shape for spread assignment" % ⟨⟩←1
!"Assignment: Mismatched shape for spread assignment" % ⟨⟩←⟨1⟩
⟨⟩←⟨⟩ %% ⟨⟩

# mid-update variable behavior
%USE jiteq ⋄ {𝕊: a←"foo" ⋄ {𝕊:a!↩@}⎊•CurrentError @ ⋄     1} _jiteq @ %% 1
!"Attempting to read variable which isn't available due to incomplete or aborted F↩" % %USE jiteq ⋄ {𝕊: a←"foo" ⋄ ! "foo" ≡ {𝕊:a!↩@}⎊•CurrentError @ ⋄ a ⋄ 1} _jiteq @
!"Attempting to read variable which isn't available due to incomplete or aborted F↩" % %USE jiteq ⋄ {𝕊: a←"foo" ⋄ a {𝕊:   ≠a}↩ ⋄ 1} _jiteq @
!"Attempting to read variable which isn't available due to incomplete or aborted F↩" % %USE jiteq ⋄ {𝕊: a←1     ⋄ a {𝕊: a ⊢↩}↩ ⋄ 1} _jiteq @

# not-yet-defined variable usage
!"Attempting to read variable which is not yet defined" % %USE jiteq ⋄ {𝕊: {a ⊢↩    } ⋄ a←1} _jiteq @
!"Attempting to read variable which is not yet defined" % %USE jiteq ⋄ {𝕊: {a ⊢↩ ⋄ 1} ⋄ a←1} _jiteq @
!"Attempting to read variable which is not yet defined" % %USE jiteq ⋄ {𝕊: {a    ⋄ 1} ⋄ a←1} _jiteq @
!"Assignment: Attempting to modify variable which is not yet defined" % %USE jiteq ⋄ {𝕊: {a‿a↩1‿2 ⋄ 1} ⋄ a←1} _jiteq @

# namespaces
⟨a,b⟩←•BQN"{x⇐1‿2⋄a⇐3‿4⋄y←5‿6⋄b⇐7‿8}" ⋄ a‿b %% ⟨3‿4 ⋄ 7‿8⟩
!"Field named ""b"" not found" % {a⇐1}.b
!"Field named ""y"" not found" % x‿y‿z ← {x‿z⇐1‿2}

# immediate modifiers
!"Field named ""b"" not found" % X←1‿2{a⇐𝕗} ⋄ ⟨b⟩←x
X←1‿2{a⇐𝕗} ⋄ ⟨a⟩←x ⋄ +´a %% 3
X←1‿2{a⇐𝕘-𝕗}3‿4 ⋄ ⟨a⟩←x ⋄ +´a %% 4
⟨a⟩←1‿2{a⇐𝕩+𝕗+𝕘}3‿4 1 ⋄ a %% 5‿7
⟨a⟩←1‿2{a⇐𝕩+𝕗} 1 ⋄ a %% 2‿3

# arguments
{x←𝕩⋄𝕩↩@⋄𝕩⋈x}-↕10 %% @⋈-↕10


{𝕊: a←"ab" ⋄ ⟨(a⋈↩"c")∘"d", a⟩}¨ ↕4 %% 4⥊<⟨⟨"ab","c"⟩∘"d", ⟨"ab","c"⟩⟩
{𝕊: a←"ab" ⋄ ⟨(a↩"c")∘"d", a⟩}¨ ↕4 %% 4⥊<⟨"c"∘"d", "c"⟩
{𝕊: a←"ab" ⋄ ⟨(b←"c")∘"d", a, b⟩}¨ ↕4 %% 4⥊<⟨"c"∘"d", "ab", "c"⟩
{𝕊: a←"ab" ⋄ ⟨(a⋈↩"c")‿"d", a⟩}¨ ↕4 %% 4⥊<⟨⟨"ab","c"⟩‿"d", ⟨"ab","c"⟩⟩
{𝕊: a←"ab" ⋄ ⟨(a↩"c")‿"d", a⟩}¨ ↕4 %% 4⥊<⟨"c"‿"d", "c"⟩
{𝕊: a←"ab" ⋄ ⟨(b←"c")‿"d", a, b⟩}¨ ↕4 %% 4⥊<⟨"c"‿"d", "ab", "c"⟩