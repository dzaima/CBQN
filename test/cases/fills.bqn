%DEF fill Fill←•internal.HasFill◶'e'‿{⊑1↑0⥊𝕩}
%DEF eqall EqAll ← {!∘≡⟜(⊑𝕩)¨ 𝕩 ⋄ ⊑𝕩}

%USE fill ⋄ F←¬ ⋄ ⟨Fill¨ F ⟨"",↕0,↕0‿0⟩, Fill∘F¨ ⟨⟨⟩, ↕0‿0, ↕0, 0‿2⥊↕0, "", 2‿0⥊""⟩⟩ %!HEAPVERIFY %% ⟨⟨' ',0,0‿0⟩,⟨'e',0‿0,0,0,' ',' '⟩⟩
%USE fill ⋄ F←- ⋄ ⟨Fill¨ F ⟨"",↕0,↕0‿0⟩, Fill∘F¨ ⟨⟨⟩, ↕0‿0, ↕0, 0‿2⥊↕0, "", 2‿0⥊""⟩⟩ %!HEAPVERIFY %% ⟨⟨' ',0,0‿0⟩,⟨'e',0‿0,0,0,' ',' '⟩⟩
%USE fill ⋄ F←| ⋄ ⟨Fill¨ F ⟨"",↕0,↕0‿0⟩, Fill∘F¨ ⟨⟨⟩, ↕0‿0, ↕0, 0‿2⥊↕0, "", 2‿0⥊""⟩⟩ %!HEAPVERIFY %% ⟨⟨' ',0,0‿0⟩,⟨'e',0‿0,0,0,' ',' '⟩⟩

%USE fill ⋄ F←⌊  ⋄ ⟨0,1,{⇐},+,@,'𝕩'⟩ Fill∘F⌜ ⟨⟨⟩, ↕0‿0, ↕0, 0‿2⥊↕0, "", 2‿0⥊""⟩ %!HEAPVERIFY  %%  2‿4/[⟨'e',0‿0,0,0,'e','e'⟩,⟨'e','e','e','e','e','e'⟩]
%USE fill ⋄ F←×  ⋄ ⟨0,1,{⇐},+,@,'𝕩'⟩ Fill∘F⌜ ⟨⟨⟩, ↕0‿0, ↕0, 0‿2⥊↕0, "", 2‿0⥊""⟩ %!HEAPVERIFY  %%  2‿4/[⟨'e',0‿0,0,0,'e','e'⟩,⟨'e','e','e','e','e','e'⟩]
%USE fill ⋄ F←⋆  ⋄ ⟨0,1,{⇐},+,@,'𝕩'⟩ Fill∘F⌜ ⟨⟨⟩, ↕0‿0, ↕0, 0‿2⥊↕0, "", 2‿0⥊""⟩ %!HEAPVERIFY  %%  2‿4/[⟨'e',0‿0,0,0,'e','e'⟩,⟨'e','e','e','e','e','e'⟩]
%USE fill ⋄ F←=  ⋄ ⟨0,1,{⇐},+,@,'𝕩'⟩ Fill∘F⌜ ⟨⟨⟩, ↕0‿0, ↕0, 0‿2⥊↕0, "", 2‿0⥊""⟩ %!HEAPVERIFY  %%  6‿6⥊⟨'e',0‿0,0,0,0,0,'e',0‿0,0,0,0,0,'e','e',0,0,0,0,'e','e',0,0,0,0,'e',0‿0,0,0,0,0,'e',0‿0,0,0,0,0⟩
%USE fill ⋄ F←+  ⋄ ⟨0,1,{⇐},+,@,'𝕩'⟩ Fill∘F⌜ ⟨⟨⟩, ↕0‿0, ↕0, 0‿2⥊↕0, "", 2‿0⥊""⟩ %!HEAPVERIFY  %%  6‿6⥊⟨'e',0‿0,0,0,' ',' ','e',0‿0,0,0,' ',' ','e','e','e','e','e','e','e','e','e','e','e','e','e',"  ",' ',' ','e','e','e',"  ",' ',' ','e','e'⟩
%USE fill ⋄ F←-  ⋄ ⟨0,1,{⇐},+,@,'𝕩'⟩ Fill∘F⌜ ⟨⟨⟩, ↕0‿0, ↕0, 0‿2⥊↕0, "", 2‿0⥊""⟩ %!HEAPVERIFY  %%  6‿6⥊⟨'e',0‿0,0,0,'e','e','e',0‿0,0,0,'e','e','e','e','e','e','e','e','e','e','e','e','e','e','e',"  ",' ',' ',0,0,'e',"  ",' ',' ',0,0⟩
%USE fill ⋄ F←-˜ ⋄ ⟨0,1,{⇐},+,@,'𝕩'⟩ Fill∘F⌜ ⟨⟨⟩, ↕0‿0, ↕0, 0‿2⥊↕0, "", 2‿0⥊""⟩ %!HEAPVERIFY  %%  6‿6⥊⟨'e',0‿0,0,0,' ',' ','e',0‿0,0,0,' ',' ','e','e','e','e','e','e','e','e','e','e','e','e','e','e','e','e',0,0,'e','e','e','e',0,0⟩
%USE fill ⋄ (> !∘≡○Fill ⊢)¨ ⟨↕0, ""⟩
%USE fill ⋄ ⟨⊢,Fill,Fill Fill⟩{𝕎𝕩}¨< 0⥊0‿1⊔2⥊<↕2 %% ⟨⟨⟩,⟨⟩,0‿0⟩

!¬•internal.HasFill <"hello"‿+
%USE fill ⋄ fa ← Fill¨ a←⟨0, 2‿3‿4, 2‿0‿4, 0‿2‿3, 2⟩⥊⟜<⌜⟨0,'a',{⇐},1‿2⟩ ⋄ {𝕊f : fa {!𝕨≡Fill F 𝕩}¨ a}¨ ⟨⥊, 0⊸/, 0⊸⥊, 0⊸↑, 0⊸↓, ⊑∘⋈⟩ ⋄ fa %% 5/≍⟨0,' ','e',0‿0⟩

%USE fill ⋄     a←10⥊<1‿2 ⋄ {        a∾↩𝕩⋄@}¨100⥊<<1‿2 ⋄ Fill a %% 0‿0
%USE fill ⋄     a←10⥊<1‿2 ⋄ {a↩0↑a ⋄ a∾↩𝕩⋄@}¨100⥊<<1‿2 ⋄ Fill a %% 0‿0
%USE fill ⋄ {𝕊: a←10⥊<1‿2 ⋄ {        a∾↩𝕩⋄@}¨(𝕩⥊<<1‿2)∾<<1‿2‿3 ⋄ a!'e'≡Fill a}¨ ↕100
%USE fill ⋄ {𝕊: a←10⥊<1‿2 ⋄ {a↩0↑a ⋄ a∾↩𝕩⋄@}¨(𝕩⥊<<1‿2)∾<<1‿2‿3 ⋄ a!'e'≡Fill a}¨ ↕100

%USE fill ⋄ Fill ≠¨  ⟨⟩ %!EACH_FILLS %% 'e'
%USE fill ⋄ Fill ≠¨  ⟨⟩ %EACH_FILLS  %% 0
%USE fill ⋄ Fill ≠∘⊢¨⟨⟩              %% 'e'
%USE fill ⋄ Fill   ⊢¨⟨⟩              %% 'e'
%USE fill ⋄ Fill ⊢∘⊢¨⟨⟩              %% 'e'

%USE fill ⋄ Fill ≠¨  "" %!EACH_FILLS %% ' '
%USE fill ⋄ Fill ≠¨  "" %EACH_FILLS  %% 0
%USE fill ⋄ Fill ≠∘⊢¨"" %!EACH_FILLS %% ' '
%USE fill ⋄ Fill   ⊢¨"" %!EACH_FILLS %% ' '
%USE fill ⋄ Fill ⊢∘⊢¨"" %!EACH_FILLS %% ' '
%USE fill ⋄ Fill ≠∘⊢¨"" %EACH_FILLS  %% 0
%USE fill ⋄ Fill   ⊢¨"" %EACH_FILLS  %% ' '
%USE fill ⋄ Fill ⊢∘⊢¨"" %EACH_FILLS  %% ' '

{3↑      -˝𝕩‿2⥊<<'a'}¨ 5‿6 %!HEAPVERIFY %% <¨¨ ⟨"aa ", 3⥊0⟩
{3↑ (2⥊2)-˝𝕩‿2⥊<<'x'}¨ 5‿6 %!HEAPVERIFY %% <¨¨ ⟨"vv ", 2‿2‿0⟩
{3↑  "ee"-˝𝕩‿2⥊<<'a'}¨ 5‿6 %!HEAPVERIFY %% <¨¨ ⟨¯4‿¯4‿0, "ee "⟩
%USE fill ⋄ Fill (↕ 4) ≠˝ "Ah"•internal.Variation 4‿4⥊'a' %% 0
%USE fill ⋄ Fill (↕⋈4) ≠˝ "Ah"•internal.Variation 4‿4⥊'a' %% 'e'
%USE fill ⋄ Fill (↕ 4) ≠˝ 4‿4⥊'a' %% 0
%USE fill ⋄ Fill (↕⋈4) ≠˝ 4‿4⥊'a' %% ⋈0 %!HEAPVERIFY

%DEF sqfill SqFill ← {•internal.Squeeze "Ah"•internal.Variation 𝕩}
%USE fill ⋄ %USE eqall ⋄ %USE sqfill ⋄ F←•internal.Keep∘⊢ ⋄ {EqAll Fill¨ ⟨SqFill ⟨𝕩⟩, F¨<𝕩, @F¨<𝕩, @F¨⋈𝕩⟩}¨ ⟨"abc", 4, 'a', {⇐}⟩
%USE fill ⋄ %USE eqall ⋄ %USE sqfill ⋄ F←•internal.Keep∘⊢ ⋄ {EqAll Fill¨ ⟨SqFill ⥊𝕩,  F¨ 𝕩, @F¨ 𝕩       ⟩}¨ ⟨"abc", 4, 'a', {⇐}⟩

2↑     'a'⊸+¨ 1 %% "b "
2↑ {⇐} 'a'⊸+¨ 1 %% "b "

# TODO:
# ⟨↑‿3⥊↕10 ⋄ ↑‿3⥊<¨↕10 ⋄ ↑‿3⥊<˘↕10⟩
# 5‿¯5↑⌜⟨↕2 ⋄ "ab" ⋄ <¨↕2 ⋄ <˘↕2⟩
