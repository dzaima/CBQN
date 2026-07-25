%DEF k _k ← {𝔽○•internal.Keep}
%DEF eq Eq ← •internal.Indistinguishable
%DEF r1 _r1_ ← {ok←{⇐} ⋄ _e←{ok∘𝔽⎊{𝕊: •CurrentError@}} ⋄ r←𝕨(𝔽⍟𝔾_e⋈(0≤𝔾)◶⟨𝔽⁼,𝔽⟩_e)𝕩 ⋄ {⊑ok∊r? !"TEST FAIL: case didn't error"; ≢´r? !"TEST FAIL: repeat error differs from single iteration"‿r; !⊑r}}
%DEF rand r ← (⊑•args).GetRand@

# Monadic
⟨0,↕5⟩ {! @ ≡ 𝕏⍟¯2⎊@ 𝕨}⌜ =‿≠‿≡‿≢‿⥊‿⊐‿⍷‿⊏‿»‿«
a ← ¯∞‿¯2‿¯1‿1.1‿9.007199254740994e15 ⋄ ! -‿+‿¬ ({𝕎⍟𝕩a}⌜ ≡ ⊑⊸{𝕎⍟𝕩a}⎉0‿1) ¯4+↕9
a ← "hello" ⋄ ! ⊣‿⊢‿⌽‿⍉ ({𝕎⍟𝕩a}⌜ ≡ ⊑⊸{𝕎⍟𝕩a}⎉0‿1) ¯4+↕9
a ←        "hello" ⋄ ! ⥊‿⊐‿⍷‿∧‿∨‿=‿≠‿≡‿≢‿≍‿«‿» ({𝕎⍟𝕩a}⌜ ≡ ⊑⊸{𝕎⍟𝕩a}⎉0‿1) ↕6
a ← 3‿2‿1⥊↓"hello" ⋄ ! ⥊‿⊐‿⍷‿∧‿∨‿=‿≠‿≡‿≢‿≍‿«‿» ({𝕎⍟𝕩a}⌜ ≡ ⊑⊸{𝕎⍟𝕩a}⎉0‿1) ↕6
# Dyadic
⟨0,↕5⟩ {! @ ≡ 3 𝕏⍟¯2⎊@ 𝕨}⌜ ⌊‿⌈‿⥊‿↑‿≡‿≢‿↓‿«‿»
a ← ↕3‿2‿1 ⋄ ! ⊢‿-‿⌽‿⍉ ({2𝕎⍟𝕩a}⌜ ≡ ⊑⊸{2𝕎⍟𝕩a}⎉0‿1) ¯4+↕9
%USE eq ⋄ a ← ¯∞‿¯2‿¯1.1‿1‿9.007199254740994e15 ⋄ ! ⟨-⟩ ({a𝕎⍟𝕩<a}⌜ Eq ⊑⊸{a𝕎⍟𝕩<a}⎉0‿1) ¯4+↕9
a ← 2÷˜¯1+↕5 ⋄ ! <‿≤‿=‿≠‿≥‿> ({a𝕎⍟𝕩<a}⌜ ≡ ⊑⊸{a𝕎⍟𝕩<a}⎉0‿1) ↕6
a ← "hello" ⋄ ! ⊣‿⥊‿↑‿<‿≤‿=‿≠‿≥‿>‿≡‿≢‿↓‿∾‿«‿» ({1𝕎⍟𝕩a}⌜ ≡ ⊑⊸{1𝕎⍟𝕩a}⎉0‿1) ↕6
a ← 3‿2‿1⥊↓"hello" ⋄ ! ⥊‿↑‿↓‿⌽‿⍉ ({1‿2𝕎⍟𝕩a}⌜ ≡ ⊑⊸{1‿2𝕎⍟𝕩a}⎉0‿1) ↕6

%USE k ⋄ (≍⍟(255-=) !∘≡ ≍_k⍟(255-=))¨ ↕¨↑4‿3‿2
≍⍟¯4(10⥊1)⥊@ %% (6⥊1)⥊@
!"≍𝕩: Result rank too large (255≡=𝕩)" % ≍⍟256 @
!"≍𝕩: Result rank too large (255≡=𝕩)" % ≍⍟255 ⥊@
!"≍𝕩: Result rank too large (255≡=𝕩)" % ≍⍟128 (128⥊1)⥊@
!"≍𝕩: Result rank too large (255≡=𝕩)" % ≍⍟((2⋆10)×1-˜2⋆53) @
!"≍𝕩: Result rank too large (255≡=𝕩)" % ≍⍟(1-˜2⋆53) ↕10
!"≍𝕩: Result rank too large (255≡=𝕩)" % %USE r1 ⋄ ≍_r1_(1-˜2⋆53) (255⥊1)⥊@
!"≍⁼𝕩: Argument must have a leading axis of 1 (2 ≡ ≠𝕩)" % ≍⍟¯4 1‿1‿2⥊@
!"≍⁼𝕩: Argument must have a leading axis of 1 (0 ≡ =𝕩)" % ≍⍟¯4 1‿1‿1⥊@
!"≍⁼𝕩: Argument must have a leading axis of 1 (0 ≡ =𝕩)" % ≍⍟¯255 1‿1‿1⥊@
!"≍⁼𝕩: Argument must have a leading axis of 1 (2 ≡ ≠𝕩)" % ≍⍟(-2⋆63) 1‿1‿2⥊@
!"≍⁼𝕩: Argument must have a leading axis of 1 (0 ≡ =𝕩)" % ≍⍟(-2⋆63) 1‿1‿1⥊@
!"≍⁼𝕩: Argument must have a leading axis of 1 (0 ≡ =𝕩)" % %USE r1 ⋄ ≍_r1_(-2⋆63) @
!"≍⁼𝕩: Argument must have a leading axis of 1 (3 ≡ ≠𝕩)" % ≍⍟¯250 (3«240⥊1)⥊@

!"⊏𝕩: 𝕩 cannot be an atom" % %USE r1 ⋄ ⊏_r1_(2⋆31) {+}
!"⊏𝕩: 𝕩 shape cannot start with 0 (⟨0⟩ ≡ ≢𝕩)" % ⊏⍟= 0⥊˜⌽↕255
!"⊏𝕩: 𝕩 shape cannot start with 0 (0‿3‿6 ≡ ≢𝕩)" % ⊏⍟1e3 ↕2(-˜⌈3×-)⌽↕10
!"⊏𝕩: 𝕩 shape cannot start with 0 (0‿0‿0‿0‿0 ≡ ≢𝕩)" % %USE r1 ⋄ ⊏_r1_ 2 0⥊˜5⥊0
⊏⍟4 @⥊˜¯4⌽↕7 %% @⥊˜↕3
!"⊏𝕩: 𝕩 cannot be rank 0" % ⊏⍟5 2‿3‿4⥊5
!"⊏𝕩: 𝕩 cannot be rank 0" % %USE r1 ⋄ ⊏_r1_ 5 <@
!"⊏𝕩: 𝕩 cannot be rank 0" % ⊏⍟256 0⥊˜255⥊1
!"⊏𝕩: 𝕩 cannot be rank 0" % ⊏⍟((2⋆10)×1-˜2⋆53) 0⥊˜255⥊1

!"»𝕩: 𝕩 cannot be a unit" % %USE r1 ⋄ »_r1_⊢9
!"«𝕩: 𝕩 cannot be a unit" % %USE r1 ⋄ «_r1_⊢9
»⍟9e18 ↕5 %% 5⥊0

!"𝕨⌽𝕩: 𝕩 must have rank at least 1 for atom 𝕨" % %USE r1 ⋄ 2 ⌽_r1_ 5 @
!"𝕨⌽⁼𝕩: 𝕩 must have rank at least 1 for atom 𝕨" % %USE r1 ⋄ ∞ ⌽_r1_ ¯2 <1
!"𝕨⌽⁼𝕩: 𝕩 cannot be an atom" % %USE r1 ⋄ ⟨⟩ ⌽_r1_ ¯5 @
!"𝕨⌽⁼𝕩: 𝕩 cannot be an atom" % %USE r1 ⋄ 2 ⌽_r1_ ¯5 @
!"𝕨⌽⁼𝕩: 𝕩 cannot be an atom" % %USE r1 ⋄ 3‿2‿1 ⌽_r1_ ¯5 @
!"Expected integer, got ¯5.5" % %USE r1 ⋄ ¯5.5 ⌽_r1_ 2 ↕10
4321‿9e18 ⌽⍟12345 8e18‿0‿7e18⥊0 %% 8e18‿0‿7e18⥊0 %USZ64
⟨⟩ ⌽⍟9e18 ↕2‿3 %% ↕2‿3
⊑ (9e15+7) ⌽⍟(3e15+6) ↕100 %% 42
⊑ (9e15+5‿3) ⌽⍟(3e15+7) ↕20‿100 %% 15‿21
!"𝕨⌽𝕩: Length of list 𝕨 must be at most rank of 𝕩 (10 ≡ ≠𝕨, ⟨⟩ ≡ ≢𝕩⟩" % %USE r1 ⋄ (↕10) ⌽_r1_ 4 5
!"𝕨⌽⁼𝕩: Length of list 𝕨 must be at most rank of 𝕩 (10 ≡ ≠𝕨, ⟨5⟩ ≡ ≢𝕩⟩" % %USE r1 ⋄ (↕10) ⌽⁼_r1_ 4 ↕5
!"𝕨⌽𝕩: 𝕨 contained non-number" % %USE r1 ⋄ ⟨1‿2,1e20⟩⊸⌽_r1_ 5 2‿2⥊↕4
!"𝕨⌽⁼𝕩: 𝕨 contained non-number" % %USE r1 ⋄ ⟨1‿2,1e20⟩⊸⌽_r1_ ¯5 2‿2⥊↕4
!"𝕨⌽⁼𝕩: 𝕨 contained non-number" % %USE r1 ⋄ ⟨1e20,1‿2⟩⊸⌽_r1_ ¯5 2‿2⥊↕4
!"Integer out of range: 1e20" % %USE r1 ⋄ ⟨1e20,1e30⟩⊸⌽_r1_ 5 2‿2⥊↕4

!"𝕨↓𝕩: Result rank too large" % %USE r1 ⋄ (256⥊0) ↓_r1_ 123 ↕0
!"𝕨↓𝕩: 𝕨 must have rank at most 1 (1‿1‿256 ≡ ≢𝕨)" % %USE r1 ⋄ (≍≍256⥊0) ↓_r1_ 123 ↕0
≢ (×´1+2⋆15‿30) ↓⍟(1-˜2⋆15) ⟨2⋆60,0⟩⥊<@ %% 1‿0 %USZ64
!"Expected integer, got 0.25" % %USE r1 ⋄ 0.25 ↓_r1_ 8 ↕10
!"Expected integer, got 1.5" % %USE r1 ⋄ 0‿1.5 ↓_r1_ 8 ↕10
1e10 ↓⍟⊣ ↕1e3 %% ↕0

!"𝕨»𝕩: 𝕩 cannot be a unit" % %USE r1 ⋄ 4»_r1_+5
!"𝕨»𝕩: =𝕨 must be =𝕩 or ¯1+=𝕩 (0≡=𝕨, 2≡=𝕩)" % %USE r1 ⋄ (<'x')»_r1_ 10["ab","cd"]
!"𝕨»𝕩: =𝕨 must be =𝕩 or ¯1+=𝕩 (3≡=𝕨, 1≡=𝕩)" % %USE r1 ⋄ (0‿1e4‿1e5⥊0)»_r1_ 10↕0
!"𝕨»𝕩: Lengths not matchable (⟨2⟩ ≡ ≢𝕨, 3‿3 ≡ ≢𝕩)" % %USE r1 ⋄ 1‿2»_r1_ 10↕3‿3
!"𝕨«𝕩: 𝕩 cannot be a unit" % %USE r1 ⋄ 4«_r1_+5
!"𝕨«𝕩: =𝕨 must be =𝕩 or ¯1+=𝕩 (0≡=𝕨, 2≡=𝕩)" % %USE r1 ⋄ (<'x')«_r1_ 10["ab","cd"]
!"𝕨«𝕩: =𝕨 must be =𝕩 or ¯1+=𝕩 (3≡=𝕨, 1≡=𝕩)" % %USE r1 ⋄ (0‿1e4‿1e5⥊0)«_r1_ 10↕0
!"𝕨«𝕩: Lengths not matchable (⟨2⟩ ≡ ≢𝕨, 3‿3 ≡ ≢𝕩)" % %USE r1 ⋄ 1‿2«_r1_ 10↕3‿3
'x'»⍟(2⋆54) ↕100 %% 100⥊'x'
(↕7) «⍟((2⋆10)×1-˜2⋆53) 100⥊'x' %% 100⊸⥊⌾⌽↕7

!"𝕨∾𝕩: Argument ranks must differ by 1 or less (0≡=𝕨, 255≡=𝕩)" % %USE r1 ⋄ 1∾_r1_ 6↕255⥊1
!"𝕨∾𝕩: Lengths not matchable (2‿4 ≡ ≢𝕨, 0‿5 ≡ ≢𝕩)" % %USE r1 ⋄ (2‿4⥊0) ∾_r1_ 3 0‿5⥊0
!"𝕨∾𝕩: Lengths not matchable (1‿2‿4 ≡ ≢𝕨, 2‿0 ≡ ≢𝕩)" % %USE r1 ⋄ (1‿2‿4⥊0) ∾_r1_ 3 2‿0⥊0
!"Out of memory" % (↕1e5) ∾⍟1e15 0
!"Out of memory" % ∾⟜0⍟(2⋆54) ↕1e5
'a'∾⍟6'b' %% "aaaaaab"
(4‿0⥊0) ∾⍟3 5‿0⥊0 %% 17‿0⥊0
(4‿0⥊0) ∾˜⍟3 ↕0 %% 13‿0⥊0
∾ ∾⟜(⋈¨"xyz")⍟2 ⋈¨"ab" %% "abxyzxyz"
9‿8⊸∾⍟5 3‿2⥊"abcdef" %% 5‿1‿1‿1/[9‿8,"ab","cd","ef"]
∾⟜(2‿2⥊1‿0)⍟3 0‿2‿2⥊0 %% 3‿2‿2⥊1‿0
⟨»⍟2 1‿'a'‿2, «⍟5 1‿'a'‿2⟩ %!PROPER_FILLS %% ⟨0‿0‿1, 0‿0‿0⟩
!"»𝕩: Fill element of 𝕩 not known" % %USE r1 ⋄ »_r1_ 2 1‿'a'‿2 %PROPER_FILLS
!"«𝕩: Fill element of 𝕩 not known" % %USE r1 ⋄ «_r1_ 5 1‿'a'‿2 %PROPER_FILLS

!"⍉⁼𝕩: 𝕩 must not be an atom" % %USE r1 ⋄ ⍉_r1_ ¯7 7
⍉⍟⊢ 7 %% <7
⍉⍟¯7 <7 %% <7
! ⍉⍟=⊸≡ (3⌽↕7)⥊0
! (⍉⍟(-2⋆63) ≡ ⍉⁼) ⥊⟜(↕×´)7⥊2

!"𝕨⍉𝕩: 𝕨 must have rank at most 1" % %USE r1 ⋄ (1‿1⥊0) ⍉_r1_ 4 ↕3
!"𝕨⍉𝕩: Length of 𝕨 must be at most rank of 𝕩" % %USE r1 ⋄ 0‿0 ⍉_r1_ 4 ↕3
!"𝕨⍉𝕩: Axis 3 does not exist (3≡=𝕩)" % %USE r1 ⋄ 3 ⍉_r1_ 4 ↕↕3
!"Expected non-negative integer, got ¯1" % %USE r1 ⋄ 0‿¯1‿1 ⍉_r1_ 123 ↕↕3
!"𝕨⍉𝕩: Skipped result axis" % %USE r1 ⋄ 2‿0‿0 ⍉_r1_ 123 ↕↕3
!"𝕨⍉⁼𝕩: 𝕩 must not be an atom" % %USE r1 ⋄ 0 ⍉_r1_ ¯4 3
!"𝕨⍉⁼𝕩: 𝕨 must have rank at most 1" % %USE r1 ⋄ (1‿1⥊0) ⍉_r1_ ¯4 ↕3
!"𝕨⍉⁼𝕩: Length of 𝕨 must be at most rank of 𝕩" % %USE r1 ⋄ 0‿0 ⍉_r1_ ¯4 ↕3
!"𝕨⍉⁼𝕩: Axis 9 does not exist (1≡=𝕩)" % %USE r1 ⋄ 9 ⍉_r1_ ¯4 ↕3
⟨⟩ ⍉⍟9e9 @ %% <@
2 ⍉⍟1e4 1‿1‿3⥊↕3 %% 1‿3‿1⥊↕3
4 ⍉⍟9876543 ⥊⟜(↕×´)1+↕6 %% (¯3⌽↕5) ⍉ ⥊⟜(↕×´)1+↕6
2‿0‿1 ⍉⍟1e4 1‿1‿3‿4⥊↕12 %% 1‿3‿1‿4⥊↕12
(⌽↕4) ⍉⍟1111 ↕↕5 %% (⌽↕4)⍉↕↕5
3‿4‿6‿1‿5‿2‿0 ⍉⍟180 ⥊⟜(↕×´)1+↕7 %% 2‿0‿4‿6‿3‿1‿5 ⍉ ⥊⟜(↕×´)1+↕7
0‿0 ⍉⍟5 ⥊⟜(↕×´)3⌈↕7 %% (4836×↕3)+⌜↕6
!"𝕨⍉⁼𝕩: Axis 3 does not exist (3≡=𝕩)" % %USE r1 ⋄ 0‿0‿3 ⍉_r1_ ¯2 0‿0‿0⥊0
%USE rand ⋄ %USE eq ⋄ ⟨Keep⟩←•internal ⋄ aa←↕10000 ⋄ R↩r.Range ⋄ ≠{𝕊: k←¯20+R 10 ⋄ w←(R 7) R 7 ⋄ x←aa⥊˜(R 7)R 7 ⋄ k‿w‿x ! Eq´ {w 𝕏⍟k⎊{𝕊:•CurrentError@} x}¨ ⟨⍉, ⊢∘⍉⟩}¨ ↕10000 %SLOW

{"foo"⍟𝕩  "bar"}¨ ↕4 %% "bar"‿"foo"‿"foo"‿"foo"
{"foo"⍟𝕩˜ "bar"}¨ ↕4 %% "bar"‿"foo"‿"foo"‿"foo"
{"foo"⍟𝕩  "foo"}¨ ¯2+↕5 %% 5⥊<"foo"
{"foo"⍟𝕩˜ "foo"}¨ ¯2+↕5 %% 5⥊<"foo"
!"⁼: Inverse does not exist" % %USE r1 ⋄ "foo"_r1_ ¯3  "bar"
!"⁼: Inverse does not exist" % %USE r1 ⋄ "foo"_r1_ ¯3˜ "bar"
!"Cannot call a 1-modifier as a function" % %USE r1 ⋄ {¨}_r1_ 3  "foo"
!"Cannot call a 1-modifier as a function" % %USE r1 ⋄ {¨}_r1_ 3˜ "foo"
!"Cannot invert modifier" % %USE r1 ⋄ {{𝕘‿𝕩}}_r1_ ¯1  "foo"
!"Cannot invert modifier" % %USE r1 ⋄ {{𝕘‿𝕩}}_r1_ ¯1˜ "foo"
