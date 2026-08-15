%DEF test bin←(⊑•args).Bin@ ⋄ Test←{{132: 1; 𝕩}⌾⊑ {'␊'¨⌾((𝕩=@+10)⊸/)𝕩}¨⌾(1⊸↓) (𝕨{s𝕊𝕩: stdin⇐s; ⇐}@) •SH ⟨bin⟩∾𝕩} ⋄ lf←@+10

%DEF file ⟨AtRoot⟩ ← ⊑•args

%DEF repl %USE test ⋄ Re←{s‿d𝕊𝕩: d¨⌾((s=𝕩)⊸/)𝕩} ⋄ REPL ← {s←"`""" Re {2=≡𝕩? (⊣∾lf∾⊢)´𝕩; (';'∾@+10) Re 𝕩}𝕩 ⋄ s Test ⟨"-s"⟩}

%USE repl ⋄ REPL "2+2;3+3" %% ⟨0,"4␊6␊",⟨⟩⟩
%USE repl ⋄ REPL "2+2;)gc;3+3;)gc off;4+4;)gc;5+5;)gc on;6+6;)gc" %% ⟨0,"4␊6␊GC disabled␊8␊GC is disabled, but forcibly GCing anyway␊10␊GC enabled␊12␊",⟨⟩⟩
%USE repl ⋄ REPL "2+2;)exit;3+3" %% ⟨0,"4␊",⟨⟩⟩
%USE repl ⋄ REPL ⟨")escaped `a←\`abc\`\nF←{𝕏@}`", "F {!∘𝕩} 1+`abc`"⟩ %% ⟨0,"(function block)␊","Error: bcd␊at F←{𝕏@}␊      ^␊at F {!∘𝕩} 1+""abc""␊   ^␊"⟩
%USE repl ⋄ {𝕩/˜(𝕩=lf)∨⌽1≥↕≠𝕩}⌾(1⊸⊑) REPL "a←1;b←2⋄c←3;)vars;)erase a;b+c;a" %% ⟨0,"5␊","Error: Attempting to read variable which is not yet defined␊at a␊   ^␊"⟩

%USE repl ⋄ REPL ⟨"a←1 ⋄ b←2", ")e a←b", ")e a+c+b"⟩ %% ⟨0,"2␊ a←b ␊ │   ␊ a─b ␊╶──┘ ␊","Error: Undefined identifier␊at a+c+b␊     ^␊"⟩
%USE repl ⋄ REPL ")e 2+2" %% ⟨0," 2+2 ␊ │ │ ␊ 2+2 ␊╶─┘  ␊",⟨⟩⟩
%USE repl ⋄ REPL ")explain 2+2" %% ⟨0," 2+2 ␊ │ │ ␊ 2+2 ␊╶─┘  ␊",⟨⟩⟩

%USE repl ⋄ %USE file ⋄ get←"•Import "∾•Repr AtRoot "imports/identity.bqn" ⋄ REPL ⟨"a←"∾get, "b←"∾get, ")clearImportCache", "c←"∾get, "d←"∾get, "⊐a‿b‿c‿d"⟩ %% ⟨0,"{foo⇐}␊{foo⇐}␊{foo⇐}␊{foo⇐}␊⟨ 0 0 1 1 ⟩␊",⟨⟩⟩
%USE repl ⋄ %USE file ⋄ REPL ⟨")ex "∾AtRoot "imports/identity.bqn", "foo.a"⟩ %% ⟨0,"123␊",⟨⟩⟩
%USE repl ⋄ %USE file ⋄ (¬'0'⊸≤∧≤⟜'9')⊸/⌾(2⊸⊑) REPL ")mem" %% ⟨0,⟨⟩,"RAM allocated: ␊heap in use: ␊"⟩

%USE repl ⋄ {𝕩/˜(𝕩=lf)∨⌽1≥↕≠𝕩}⌾(1⊸⊑) REPL ")t 2+2" %% ⟨0,"s␊",⟨⟩⟩
%USE repl ⋄ {𝕩/˜(𝕩=lf)∨⌽1≥↕≠𝕩}⌾(1⊸⊑) REPL ")time 2+2" %% ⟨0,"s␊",⟨⟩⟩
%USE repl ⋄ {𝕩/˜(𝕩=lf)∨⌽1≥↕≠𝕩}⌾(1⊸⊑) REPL ")t:0.01s 2+2" %% ⟨0,"s␊",⟨⟩⟩
%USE repl ⋄ {𝕩/˜(𝕩=lf)∨⌽1≥↕≠𝕩}⌾(1⊸⊑) REPL ")time:0.01s 2+2" %% ⟨0,"s␊",⟨⟩⟩
%USE repl ⋄ {𝕩/˜(𝕩=lf)∨⌽1≥↕≠𝕩}⌾(1⊸⊑) REPL ")t:100 2+2" %% ⟨0,"s␊",⟨⟩⟩
%USE repl ⋄ {𝕩/˜(𝕩=lf)∨⌽1≥↕≠𝕩}⌾(1⊸⊑) REPL ")t:1e2 2+2" %% ⟨0,"s␊",⟨⟩⟩
%USE repl ⋄ {𝕩/˜(𝕩=lf)∨⌽1≥↕≠𝕩}⌾(1⊸⊑) REPL ")t !0" %% ⟨0,⟨⟩,"Error: Assertion error␊at !0␊   ^␊"⟩

%USE repl ⋄ REPL ")t:!1" %% ⟨0,"Timing repetition count not provided␊",⟨⟩⟩
%USE repl ⋄ REPL ")t:-1 2+2" %% ⟨0,"Timing repetition count not provided␊",⟨⟩⟩
%USE repl ⋄ REPL ")t:0 2+2" %% ⟨0,"Timing repetition count cannot be zero␊",⟨⟩⟩
%USE repl ⋄ REPL ")t:1.11e1 2+2" %% ⟨0,"Timing repetition count must be an integer␊",⟨⟩⟩
%USE repl ⋄ REPL ")t:1e1e1 2+2" %% ⟨0,"Bad timing repetition count␊",⟨⟩⟩
%USE repl ⋄ REPL ")t:9999999999999999999 2+2" %% ⟨0,"Timing repetition count too large␊",⟨⟩⟩
%USE repl ⋄ REPL ")t:9999999999999999999999999999999999999999999 2+2" %% ⟨0,"Timing repetition count too large␊",⟨⟩⟩

%USE repl ⋄ REPL ")profile@0 2+2" %% ⟨0,⟨⟩,"Cannot profile with 0hz sampling frequency␊"⟩
%USE repl ⋄ REPL ")profile@ 2+2" %% ⟨0,"Invalid profile sample rate value␊",⟨⟩⟩
%USE repl ⋄ REPL ")profile@999999999 2+2" %% ⟨0,⟨⟩,"Cannot profile with >999999hz frequency␊"⟩
%USE repl ⋄ REPL ")profile@99999999999999999999 2+2" %% ⟨0,"Invalid profile sample rate value␊",⟨⟩⟩
