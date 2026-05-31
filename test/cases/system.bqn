%DEF var V←•internal.Variation ⋄ LV←•internal.ListVariations ⋄ CLR←•internal.ClearRefs
%DEF tvar %USE var ⋄ _tvar ← {F _𝕣 x: (CLR@) ⊢ {F 𝕩 V x}¨ LV 𝕩; w F _𝕣 x: (CLR@) ⊢ (LV 𝕨) {(𝕨 V w) F 𝕩 V x}⌜ LV 𝕩}
%DEF eqvar ⟨_eqvar⟩←⊑•args
%DEF eqvarv ⟨_eqvarv_⟩←⊑•args
%DEF rand r ← (⊑•args).GetRand@
# not tested here: •Out •Show •Exit •GetLine •Cmp •CurrentError •FFI •internal.HeapDump



# standard system values

# •BQN
%USE eqvar ⋄    •BQN _eqvar "1↑""""" %% " "
%USE eqvar ⋄ ⟨⟩ •BQN _eqvar "1↑""""" %% " "
! ≡´ t←⟨•path,"custom-name", "arg"⟩•BQN"⟨•path‿•name‿•args ⋄ •state⟩" ⋄ ! ⟨•path, "custom-name", "arg"⟩ ≡ ⊑t
! ≡´ t←⟨•path,"custom-name"       ⟩•BQN"⟨•path‿•name‿•args ⋄ •state⟩" ⋄ ! ⟨•path, "custom-name", ⟨⟩⟩    ≡ ⊑t
! ≡´ t←⟨•path                     ⟩•BQN"⟨•path‿•name‿•args ⋄ •state⟩" ⋄ ! ⟨•path, "",            ⟨⟩⟩    ≡ ⊑t
! ≡´ t←⟨                          ⟩•BQN"⟨•path‿•name‿•args ⋄ •state⟩" ⋄ !        ⟨"",            ⟨⟩⟩    ≡ 1↓⊑t
!"•BQN: Path must be a string" % ⟨1‿2⟩ •BQN "2+2"
!"•BQN: Filename must be a string" % ⟨•path, 1‿2⟩ •BQN "2+2"
⟨•path, "abc/def"⟩ •BQN "•name" %% "abc/def"
!"•BQN: 𝕩 must be a string" % •BQN ↕3
!"No path present for •path" % •BQN"•path"
!"No arguments present for •args" % •BQN"•args"
!"No name present for •name" % •BQN"•name"
!"No arguments present for •state" % •BQN"•state"
! •path‿""‿"" ≡ ⟨•path⟩•BQN"•state"
! ""‿"" ≡ ¯2↑⟨⟩•BQN"•state"
"args"‿"path"‿"name"‿"helloworld"‿"state"‿"rebqn"‿"hashmap"‿"testsysval1"‿"testsysval2"∊•BQN"•listsys" %% 1‿1‿1‿0‿1‿1‿1‿0‿0
!"System value •testsysval1 is not available in this CBQN build" % •testsysval1 %DEBUG
!"System value •testsysval2 is not available in this CBQN build" % •testsysval2 %DEBUG
!"Unknown system function •testsysval9" % •testsysval9
!"Failed to resolve ""path that doesn't exist"": No such file or directory" % ⟨"path that doesn't exist"⟩•BQN"•path"
!"Failed to resolve ""path that doesn't exist"": No such file or directory" % ⟨"path that doesn't exist"⟩•BQN"•state"
⟨"path that doesn't exist"⟩•BQN"2+2" %% 4
!"•file.Chars: Using relative path with no absolute base path known" % f←•BQN"•FChars" ⋄ F "a"

# •ReBQN
!"REPL: Filename must be a string" % f←•ReBQN{⇐} ⋄ ⟨•path, 1‿2⟩ F "2+2"
!"REPL: Path must be a string" % f←•ReBQN{⇐} ⋄ ⟨1‿2⟩ F "2+2"
f←•ReBQN{⇐} ⋄ ! ⟨•path, "custom-name", "arg"⟩ ≡ ⟨•path,"custom-name", "arg"⟩ F "•state"

f←•ReBQN {system⇐"all"} ⋄ F "•internal.Type ↕10" %% "i8arr"
!"Unknown system function •internal" % f←•ReBQN {system⇐"none"} ⋄ F "•internal.Type ↕10"
f←•ReBQN {system⇐⟨"range"‿↕ ⋄ "add"‿+ ⋄ "reverse"‿⌽⟩} ⋄ F "•Reverse ¯5 •Add •Range 10" %% 4-↕10

f←•ReBQN{⇐} ⋄ F "Ah"•internal.Variation"1↑""""" %% " "

F←(•ReBQN{repl⇐"none"  })⎊({𝕩↑˜⊑𝕩⊐@+10}•CurrentError) ⋄ ⟨F"a←↕2", F"a←↕3", F"a"⟩ %% ⟨0‿1,0‿1‿2,"Undefined identifier"⟩
F←(•ReBQN{repl⇐"loose" })⎊({𝕩↑˜⊑𝕩⊐@+10}•CurrentError) ⋄ ⟨F"a←↕2", F"a←↕3", F"a"⟩ %% ⟨0‿1,0‿1‿2,0‿1‿2⟩
F←(•ReBQN{repl⇐"strict"})⎊({𝕩↑˜⊑𝕩⊐@+10}•CurrentError) ⋄ ⟨F"a←↕2", F"a←↕3", F"a"⟩ %% ⟨0‿1,"Redefinition",0‿1⟩

f ← •ReBQN{repl⇐"none"} ⋄ !6≡•Type F "a⇐1"
f ← •ReBQN{repl⇐"none"} ⋄ !6≡•Type F "⇐"
!"Cannot construct a namespace for a REPL result" % f ← •ReBQN{repl⇐"loose"} ⋄ F "a⇐1"
!"Cannot construct a namespace for a REPL result" % f ← •ReBQN{repl⇐"loose"} ⋄ F "⇐"
!"Cannot construct a namespace for a REPL result" % f ← •ReBQN{repl⇐"strict"} ⋄ F "a⇐1"
!"Cannot construct a namespace for a REPL result" % f ← •ReBQN{repl⇐"strict"} ⋄ F "⇐"

f←•ReBQN {primitives⇐⋈'÷'‿-} ⋄ ! ¯5 ≡ F"•BQN""÷5""" ⋄ b←F"•BQN" ⋄ ! ¯5 ≡ B "÷5" ⋄ ! 0.2 ≡ •BQN "÷5"
f←•ReBQN {primitives⇐⋈'÷'‿-} ⋄ a1 ← •bqn ⋄ r1 ← F"•BQN" ⋄ a2 ← A1"•BQN" ⋄ r2 ← R1"•BQN" ⋄ a3 ← A1"•ReBQN{⇐}" ⋄ r3 ← R1"•ReBQN{⇐}" ⋄ {𝕏"÷"}¨ ⟨a1,r1,a2,r2,a3,r3⟩ %% ⟨÷,-,÷,-,÷,-⟩
f←•ReBQN {repl⇐"none"  ⋄ primitives⇐⋈'÷'‿÷} ⋄ ! "Unknown character: '-'" ≡ F⎊{𝕊: •CurrentError@} "-5" ⋄ ! 0.2 ≡ •BQN "÷5"
f←•ReBQN {repl⇐"loose" ⋄ primitives⇐⋈'÷'‿÷} ⋄ ! "Unknown character: '-'" ≡ F⎊{𝕊: •CurrentError@} "-5" ⋄ ! 0.2 ≡ •BQN "÷5"
ps ← ⟨'÷'‿÷⟩ ⋄ f←•ReBQN {repl⇐"none"  ⋄ primitives⇐ps} ⋄ ! ps ≡ F "•primitives" ⋄ ! •primitives ≡ •BQN "•primitives" ⋄ ! ps ≡ F "•primitives"
ps ← ⟨'÷'‿÷⟩ ⋄ f←•ReBQN {repl⇐"loose" ⋄ primitives⇐ps} ⋄ ! ps ≡ F "•primitives" ⋄ ! •primitives ≡ •BQN "•primitives" ⋄ ! ps ≡ F "•primitives"

f←•ReBQN{primitives⇐⋈'÷'‿- ⋄ system⇐⟨"foo"‿⋈, "oldbqn"‿•BQN, "bqn"⟩} ⋄ {𝕏 "÷5"}¨ F "⟨•foo, •OldBQN, •BQN⟩" %% ⟨⋈"÷5", 0.2, ¯5⟩
!"Character set conflict: ," % •ReBQN{primitives ⇐ ⟨','‿+⟩}
!"•ReBQN: No system value ""baz"" to inherit" % •ReBQN{system⇐⟨"foo"‿"bar", "bqn", "baz", "rebqn"⟩}
!"•ReBQN: No system value ""bqn"" to inherit" % f←•ReBQN{system⇐⟨"baz"‿1, "rebqn", "foo"‿2⟩} ⋄ g←F"•ReBQN" ⋄ G {system⇐⟨"foo"‿"bar", "baz", "bqn", "rebqn"⟩}
f←•ReBQN{primitives⇐⋈'÷'‿- ⋄ system⇐⟨"primitives", "foo"‿⋈, "bqn", "listsys"⟩} ⋄ F"•primitives‿•listsys" %% ⟨⟨'÷'‿-⟩ ⋄ ⟨"primitives","foo","bqn","listsys"⟩⟩
f←•ReBQN{system⇐⟨"e1"⋈↕0, "e2"⋈"", "e3"⋈0⥊<"hi"⟩ ⋄ primitives⇐⟨'+',(<"")⊸∾⟩‿⟨'-',(<↕0)⊸∾⟩} ⋄ (⊑1↑⊢)¨ F "+-•e2‿•e1‿•e3" %% ⟨' ', 0, ' ', 0, "  "⟩
a←b←0 ⋄ f←•ReBQN{primitives⇐⟨'¨'‿{a+↩1⋄𝕗},'∘'‿{b+↩1⋄𝕘},'⊢'‿⊢⟩} ⋄ g←F "{⊢∘⊢𝕨 ⋄ ⊢¨𝕩}" ⋄ G¨˜ ↕10 ⋄ a‿b %% 10‿10

# TODO test stacktrace of {𝕏"!0"}•ReBQN{⇐}

# •primitives
!∧´2=•Type¨ ⊑¨•primitives
!¨ (•Type¨1⊑¨•primitives)∊3‿4‿5

# •state
6⊸↑⌾(1⊸⊑) 1⊸↓⌾(2⊸⊑) •file.Name⌾⊑ •state %% ⟨"testDir","system",⟨"arg0",1⟩⟩

# •args
1↓•args %% ⟨"arg0",1⟩

# •path & •file.path
•file.Name •path %% "testDir"
•file.Name •file.path %% "testDir"

# •name
6↑•name %% "system"

# •wdpath
! ∧´ 2=•Type¨ •wdpath

# •ns
%USE eqvar ⋄ {abc⇐1 ⋄ dÊF⇐2}⊸•ns.Get _eqvar¨ ⟨"abc", "ABC", "abC", "abc___", "DêF", "__D__Ê__F__"⟩ %% 1‿1‿1‿1‿2‿2
!"Field named ""abcdefghijklmnopqrstuvwxyzàáâãäåæçèéêëìíîïðñòóôõöøùúûüýþabcdefghijklmnopqrstuvwxyzàáâãäåæçèéêëìíîïðñòóôõöøùúûüýþ𝕨𝕨⍉!0123456789"" not found" % {abc⇐1 ⋄ dÊF⇐2}⊸•ns.Get "ABCDEFGHIJKLMNOPQRSTUVWXYZÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝÞabcdefghijklmnopqrstuvwxyzàáâãäåæçèéêëìíîïðñòóôõöøùúûüýþ𝕨𝕨⍉!0123456789_"
{abc⇐1 ⋄ dÊF⇐2 ⋄ n10⇐3}•ns.Has¨ ⟨"abc", "ABC", "abC", "abc___", "DêF", "__D__Ê__F__", "àbc", "def", "", ↕0, "𝕨", "012", "n10", "n1_0"⟩ %% 1‿1‿1‿1‿1‿1‿0‿0‿0‿0‿0‿0‿1‿1
∨´{⇐}•ns.Has¨↑"abc123AEWQE___àÀ𝕨𝕩" %% 0
!"•ns.Has: 𝕩 must be a string" % {⇐}•ns.Has ≍""
!"•ns.Has: 𝕩 must be a string" % {⇐}•ns.Has 1‿2
!"•ns.Get: 𝕩 must be a string" % {⇐}•ns.Get ≍""
!"•ns.Get: 𝕩 must be a string" % {⇐}•ns.Get 1‿2
!"𝕨 •ns.Get 𝕩: 𝕨 must be a namespace" % 1•ns.Get "hi"
!"𝕨 •ns.Has 𝕩: 𝕨 must be a namespace" % 1•ns.Has "hi"
!"•ns.Keys 𝕩: 𝕩 must be a namespace" % •ns.Keys 1
{m←𝕩 ⋄ ⟨{𝕩 ⋈ m•ns.Get 𝕩}¨ •ns.Keys m ⋄ {m•ns.Has⋈𝕩}¨ "abcABC"⟩}¨ ⟨{a⇐1⋄b⇐↕10} ⋄ {a⇐'a'⋄b←2‿3⋄cde⇐𝕩}"abc"⟩ %% ⟨⟨⟨⟨"a",1⟩,⟨"b",0‿1‿2‿3‿4‿5‿6‿7‿8‿9⟩⟩,1‿1‿0‿1‿1‿0⟩,⟨⟨⟨"a",'a'⟩,⟨"cde","abc"⟩⟩,1‿0‿0‿1‿0‿0⟩⟩



# files; tests are ordered!
{•file.Exists 𝕩? ⊑•SH⟨"rmdir", •file.At 𝕩⟩; 0} "testdirNested" %% 0
•file.Remove⍟•file.Exists¨ "testfile.bqn"‿"testfile2.bqn"‿"testfile3B.bqn"‿"badwrite" ⋄ 1 %% 1
•file.At "/a/b" %% "/a/b"
! (•file.At "a/b") ≡ •file.path •file.At "a/b"
"a/b" •file.At "c/d" %% "a/b/c/d"
"a/b/" •file.At "c/d" %% "a/b/c/d"
"a/b/" •file.At "c/d/" %% "a/b/c/d/"
"a/b/" •file.At "/c/d" %% "/c/d"
"a/b" •file.At "/c/d/" %% "/c/d/"
•file.Exists "/dev/file that doesn't exist" %% 0
!"Couldn't read file ""/dev/file that doesn't exist""" % •FChars "/dev/file that doesn't exist"
! (•path∾"/") ≡ •file.Parent •file.At "hi"
!"•file.Parent: Path must be non-empty" % •file.Parent ""
!"•file.Name: Path must be non-empty" % •file.Name ""

!"𝕨 •file.Lines 𝕩: 𝕩 must be a list" % "testfile.bqn" •FLines 2‿2⥊<"abcd"
!"𝕨 •file.Lines 𝕩: 𝕩 must be a list" % "testfile.bqn" •file.Lines <"abcd"
!"𝕨 •file.Lines 𝕩: Elements of 𝕩 must be lists of characters" % "testfile.bqn" •FLines "abcd"
!"Expected character" % "testfile.bqn" •FLines ⋈"ab"‿"cd"
!"Expected character" % "testfile.bqn" •FLines ⋈1‿2
!"Expected character" % "testfile.bqn" •FChars ⋈"abcd"
!"𝕨 •file.Chars 𝕩: 𝕩 must be a list of characters" % "testfile.bqn" •FChars <'a'
!"Expected character" % "testfile.bqn" •FBytes ⋈"abcd"
!"𝕨 •file.Bytes 𝕩: 𝕩 must be a list" % "testfile.bqn" •file.Bytes <'a'
!"Expected character" % "badwrite"•FChars ""‿""
!"Expected character" % "badwrite"•FLines ⟨⟨"foo"⟩⟩
!"𝕨 •file.Chars 𝕩: 𝕩 must be a list of characters" % "badwrite"•FChars @

! {𝕩≡•file.Name 𝕩 •FChars         "abc"∾(@+10)∾"def𝕩"}"testfile.bqn"
! {𝕩≡•file.Name 𝕩 •FBytes •ToUTF8 "abc"∾(@+10)∾"def𝕩"}"testfile2.bqn"
! {𝕩≡•file.Name 𝕩 •FLines         "abc"‿"def𝕩"}       "testfile3.bqn"
•file.Type "testfile.bqn" %% 'f'
•file.Type •file.path %% 'd'
!¨ 0≤{𝕏 "testfile.bqn"}¨ •file.Created‿•file.Accessed‿•file.Modified
•file.Size "testfile.bqn" %% 11

•FBytes¨ "testfile.bqn"‿"testfile2.bqn"‿"testfile3.bqn" %% ∾⟜(@+10)⌾(2⊸⊑) 3⥊< •ToUTF8 "abc"∾(@+10)∾"def𝕩"
•FChars "testfile.bqn" %% "abc"∾(@+10)∾"def𝕩"
•FBytes "testfile.bqn" %% @+97‿98‿99‿10‿100‿101‿102‿240‿157‿149‿169
•FLines "testfile.bqn" %% "abc"‿"def𝕩"
! 97‿98‿99‿10‿100‿101‿102‿240‿157‿149‿169 ≡ @-˜ •file.MapBytes "testfile.bqn"

•file.Name "testfile3B.bqn" •file.Rename "testfile3.bqn" %% "testfile3B.bqn"
!"𝕨 •file.Rename 𝕩: Failed to rename file" % "testfile3B.bqn" •file.Rename "testfile.bqn"
•file.Exists¨ "testfile.bqn"‿"nontestfile.bqn"‿"testfile3.bqn"‿"testfile3B.bqn" %% 1‿0‿0‿1
¯22↑•file.Createdir "testdirNested" %% "/testDir/testdirNested"
¯8↑•file.RealPath "testdirNested/.././" %% "/testDir"
!"•file.CreateDir 𝕩: Failed to create directory" % •file.Createdir "testdirNested"
"testfile.bqn"‿"testfile2.bqn"‿"testfile3.bqn"‿"testfile3B.bqn"‿"testdirNested" ∊ •file.List "." %% 1‿1‿0‿1‿1
"testfile.bqn"‿"testfile2.bqn"‿"testfile3.bqn"‿"testfile3B.bqn"‿"testdirNested" ∊ •file.List •file.path %% 1‿1‿0‿1‿1

•file.Remove "testfile.bqn" %% 1
•file.Remove "testfile2.bqn" %% 1
•file.Remove "testfile3B.bqn" %% 1
"testfile.bqn"‿"testfile2.bqn"‿"testfile3B.bqn"‿"badwrite" ∊ •file.List "." %% 0‿0‿0‿0

!"Failed to resolve ""/dev/file that doesn't exist"": No such file or directory" % •file.RealPath "/dev/file that doesn't exist"
!"Failed to resolve ""/dev/file that doesn't exist"": No such file or directory" % •Import "/dev/file that doesn't exist"

# bad paths
!"•file.At: Path must be a list of characters" % •file.At 1‿2
!"•file.At: Path must be a list of characters" % •file.At 1‿2
!"•file.At: Path must be a list of characters" % "foo" •file.At 1‿2
!"•file.At: 𝕨 must be a string" % 1‿2 •file.At "foo"
!"•file.List: Path must be a list of characters" % •file.List 1‿2
!"•file.Bytes: Path must be a list of characters" % •file.Bytes 1‿2
!"•file.Chars: Path must be a list of characters" % •file.Chars 1‿2
!"•file.Lines: Path must be a list of characters" % •file.Lines 1‿2
!"•file.Bytes: Path must be a list of characters" % 1‿2 •FBytes "abc"
!"•file.Chars: Path must be a list of characters" % 1‿2 •FChars "abc"
!"•file.Lines: Path must be a list of characters" % 1‿2 •FLines "abc"‿"def"
!"•file.Type: Path must be a list of characters" % •file.Type 1‿2
!"•file.Exists: Path must be a list of characters" % •file.Exists 1‿2
!"•file.Name 𝕩: Argument must be a string" % •file.Name 1‿2
!"•file.Parent 𝕩: Argument must be a string" % •file.Parent 1‿2
!"•file.MapBytes: Path must be a list of characters" % •file.MapBytes 1‿2
!"•file.CreateDir: Path must be a list of characters" % •file.CreateDir 1‿2
!"•file.RealPath: Path must be a list of characters" % •file.RealPath 1‿2
!"•file.Rename: Path must be a list of characters" % "foo" •file.Rename 1‿2
!"•file.Rename: Path must be a list of characters" % 1‿2 •file.Rename "foo"
!"•file.Remove: Path must be a list of characters" % •file.Remove 1‿2
!"•file.Created: Path must be a list of characters" % •file.Created 1‿2
!"•file.Modified: Path must be a list of characters" % •file.Modified 1‿2
!"•file.Accessed: Path must be a list of characters" % •file.Accessed 1‿2
!"•file.Size: Path must be a list of characters" % •file.Size 1‿2
!"•file.Size: Path must be a list of characters" % •file.Size 2‿2⥊"abcd"



# some of •term
!"Expected character" % •term.OutRaw 2⥊{⇐}
!"•term.ErrRaw 𝕩: 𝕩 must be a list" % •term.ErrRaw 2‿2⥊0



# •ParseFloat & •Repr
v←1 ⋄ ! •BQN∘•Repr⊸≡ ⟨+,1‿2,+¨,(+V)(V+V),2‿2⥊↕4⟩
# v←1 ⋄ •BQN∘•Repr⊸≡ ⟨V V V, V V⟩ # TODO enable
64‿1•bit._cast •ParseFloat¨ "123.456000000000000"‿"123.45600000000002" %% "11101110011111011111100101011000111101001011101101111010000000100001111001111101111110010101100011110100101110110111101000000010"-'0'
! ∧´'e'=•ParseFloat⎊'e'¨ "-"‿"e2"‿"-e2"‿"."‿".e2"‿"123e"‿"123e-"‿"123e+"
•ParseFloat "0."∾(n⥊'0')∾"1234e"∾•Repr n←1000000 %% 0.1234
(@≢⊑)¨⊸/ •ParseFloat⎊@∘⋈⊸⋈¨ @+↕257 %% (⊢⋈¨'0'⊸+) ↕10
•ParseFloat 1e6⥊'1' %% ∞
!"•ParseFloat 𝕩: Input too long" % •ParseFloat 2e6⥊'1'
•Repr∘•ParseFloat¨ ⟨"1.23516411460311636e-323", "1.23516411460311637e-323"⟩ %% ⟨"1e¯323","1.5e¯323"⟩
•Repr∘•ParseFloat¨ ⟨"1.235164114603116360e-323", "1.235164114603116361e-323"⟩ %% ⟨"1e¯323","1e¯323"⟩
•Repr∘•ParseFloat¨ ⟨"1.2351641146031163604e-323", "1.2351641146031163605e-323"⟩ %% ⟨"1e¯323","1e¯323"⟩
a←1↓@∾↕3 ⋄ •ParseFloat⎊"err" a ⋄ ⟨/⁼ a, ⍷ ⟨1, a, ↕3⟩⟩ %% ⟨3⥊1, 1⋈↕3⟩
!"•ParseFloat 𝕩: Input was empty" % •ParseFloat ""
!"•ParseFloat 𝕩: Input was empty" % •ParseFloat ↕0
!"•ParseFloat 𝕩: Expected a character list argument" % •ParseFloat ↕3
!"•ParseFloat 𝕩: Input must have rank 1" % •ParseFloat <@
!"•ParseFloat 𝕩: Input must have rank 1" % •ParseFloat 1‿1⥊'1'
!"•ParseFloat 𝕩: Expected a character list argument" % •ParseFloat 48‿49
!"•ParseFloat 𝕩: Expected a character list argument" % •ParseFloat <48
!"•ParseFloat 𝕩: Malformed input" % •ParseFloat "1e1e1"
!"•ParseFloat 𝕩: Malformed input" % •ParseFloat ⋈@
!"•ParseFloat 𝕩: Malformed input" % •ParseFloat ⋈@+254
!"•ParseFloat 𝕩: Malformed input" % •ParseFloat ⋈@+1111
!"•ParseFloat 𝕩: Malformed input" % •ParseFloat ⋈@+1114111

# •Fmt
•Fmt 123 %% "123"

# •SH
•SH⟨"true"⟩ %% 0‿⟨⟩‿⟨⟩
•SH⟨"false"⟩ %% 1‿⟨⟩‿⟨⟩
{stdin⇐"𝕩"∾@+↕256} •SH⟨"cat"⟩ %% ⟨0, "𝕩"∾@+↕256, ""⟩
{stdin⇐"𝕩"∾@+↕256 ⋄ raw⇐0} •SH⟨"cat"⟩ %% ⟨0, "𝕩"∾@+↕256, ""⟩
{stdin⇐@+↕256 ⋄ raw⇐1} •SH⟨"cat"⟩ %% ⟨0, @+↕256, ""⟩

# •Type
•Type¨ ⟨"ab",'a'‿1,1,0÷0,'a',@+1114111,+,{𝕩},¨,{𝕗},∘,{𝔽𝕘},{⇐},•rand⟩ %% 0‿0‿1‿1‿2‿2‿3‿3‿4‿4‿5‿5‿6‿6

# •Glyph
•Glyph¨ ⟨+,¨,∘⟩ %% "+¨∘"

# •Decompose
{!  0‿𝕩 ≡ •Decompose 𝕩}¨ ⟨+, ¨, ˝, ⁼, ⌾, ⎊, •internal.PureKeep⟩
{!  1‿𝕩 ≡ •Decompose 𝕩}¨ ⟨{𝕩}, {𝔽}, {𝔽𝕩}, {𝔽𝕘}, {𝔽𝔾𝕩}⟩
{!  1‿𝕩 ≡ •Decompose 𝕩}¨ ⟨•Type, •internal.Keep, •FChars, •_timed, •_while_⟩
{! ¯1‿𝕩 ≡ •Decompose 𝕩}¨ ⟨1,'a',"ab"⟩
•Decompose¨ ⟨+-×, -÷, +¨, 1∘+⟩ %% ⟨⟨3,+,-,×⟩,⟨2,-,÷⟩,⟨4,+,¨⟩,⟨5,1,∘,+⟩⟩

# •UnixTime
•Type •UnixTime ↕10 %% 1

# •MonoTime
! ≤´•MonoTime¨↕2

# •Delay
t0←•MonoTime@ ⋄ ! 0.1≤•Delay 0.1 ⋄ ! 0.1≤(•MonoTime@)-t0

# •_timed tested more at perf.bqn
!"𝕨 𝔽•_timed 𝕩: 𝕨 must be an integer greater than 0" % 0 ⊢•_timed 2
!"Expected integer, got character" % @ ⊢•_timed 2
a←1 ⋄ a ⊣ {𝕊: a+↩𝕩}•_timed 10 %% 11
a←3 ⋄ 12 {𝕊: a+↩𝕩}•_timed 10 ⋄ a %% 123

# •math
%USE rand ⋄ ! ∧´0=⌊|1e10×{(+´𝕩)-•math.Sum 𝕩}¨ ↑1000 r.Range 0
!"•math.Sum 𝕩: 𝕩 must be a list (⟨⟩ ≡ ≢𝕩)" % •math.Sum 2
!"•math.Sum 𝕩: 𝕩 must be a list (2‿2 ≡ ≢𝕩)" % •math.Sum 2‿2⥊1
!"•math.Sum 𝕩: 𝕩 elements must be numbers" % •math.Sum 1‿'a'



# two assumptions made by the following tests
%DEF nan nan←⊑1‿64•bit._cast/51‿13 ⋄ nans ← nan∾-nan
%USE nan ⋄ 64‿32•bit._cast nans %% 0‿¯524288‿0‿2146959360
%USE nan ⋄ 64‿32•bit._cast (¯1⊸×≍⊢) (↕3)∾∞∾nan %% 2‿10⥊0‿¯2147483648‿0‿¯1074790400‿0‿¯1073741824‿0‿¯1048576‿0‿¯524288‿0‿0‿0‿1072693248‿0‿1073741824‿0‿2146435072‿0‿¯524288
64‿32•bit._cast ⟨π, 0, •ParseFloat"-0"⟩ %% 1413754136‿1074340347‿0‿0‿0‿¯2147483648

! {(𝕩 •math.Atan2 0) ≡ 𝕩 •math.Atan2 •ParseFloat"-0"} (-≍⊢) ∞∾˜↕3
! {(0 •math.Atan2 𝕩) ≡ (•ParseFloat"-0") •math.Atan2 𝕩} (-≍⊢) ∞∾˜↕3
E←•internal.EEqual ⋄ a←(¯1⊸×≍⊢) (↕3)∾∞∾0÷0 ⋄ {𝕩!(a 𝕏 0)E a 𝕏 •ParseFloat"-0" ⋄ 𝕩!(0 𝕏 a)E(•ParseFloat"-0") 𝕏 a}¨ (⊢∾{𝕏¨}¨) ⟨+,-,×,÷,⋆,√,⌊,⌈,|,¬,∧,∨,<,>,≠,=,≤,≥, •math.Atan2,•math.Comb,•math.Hypot⟩
E←•internal.EEqual ⋄ {! (𝕏 4⥊0) E 𝕏 1↓¯1×π∾4⥊0}¨ (⊢∾{𝕏¨}¨) ⟨+,-,×,÷,⋆,√,⌊,⌈,|,¬, •math.Sin,•math.Cos,•math.Tan,•math.Asin,•math.Acos,•math.Atan,•math.Sinh,•math.Cosh,•math.Tanh,•math.Asinh,•math.Acosh,•math.Atanh,•math.Cbrt,•math.Log2,•math.Log10,•math.Log1p,•math.Expm1,•math.Fact,•math.LogFact,•math.Erf,•math.ErfC⟩
•internal.EEqual´¨ 1⊏˘¨{⟨•math.Atan2⟜2⁼𝕩, •math.Atan2⟜0⁼𝕩, 2⊸•math.Atan2⁼𝕩, 0⊸•math.Atan2⁼𝕩⟩} (¯1⊸×≍⊢) 1.2‿0 %% 1‿1‿1‿1
(0÷0) •internal.EEqual -0÷0 %% 1
%USE eqvar ⋄ •internal.EEqual _eqvar⟜- ⟨0÷0, 3⥊0⟩ %% 1

{15 𝕏 10}¨ ⟨•math.GCD,•math.LCM⟩ %% 5‿30
⌊1e5×{0.1 𝕏 0.2}¨ ⟨•math.Atan2,•math.Comb,•math.Hypot,•math.Atan2⁼,•math.Atan2˜⁼⟩ %% 110714‿96959‿22360‿2027‿49331
⌊1e5×{=˜◶99‿⊢¨ 𝕏 0.1‿3}¨ ⟨•math.Acos,•math.Acosh,•math.Asin,•math.Asinh,•math.Atan,•math.Atanh,•math.Cbrt,•math.Cos,•math.Cosh,•math.Erf,•math.ErfC,•math.Expm1,•math.Fact,•math.Log10,•math.Log1p,•math.Log2,•math.LogFact,•math.Sin,•math.Sinh,•math.Tan,•math.Tanh⟩ %% ⟨147062‿9900000,9900000‿176274,10016‿9900000,9983‿181844,9966‿124904,10033‿9900000,46415‿144224,99500‿¯99000,100500‿1006766,11246‿99997,88753‿2,10517‿1908553,95135‿600000,¯100000‿47712,9531‿138629,¯332193‿158496,¯4988‿179175,9983‿14112,10016‿1001787,10033‿¯14255,9966‿99505⟩
{𝕩 ! 1‿2 (𝕏 ≡ 𝕏¨) 3‿4}¨ ⟨•math.Atan2,•math.Comb,•math.Hypot,•math.Atan2⁼,•math.Atan2˜⁼,•math.GCD,•math.LCM⟩
!"𝕨 •math.GCD 𝕩: Inputs other than natural numbers not yet supported" % 1.5 •math.GCD 3
!"𝕨 •math.LCM 𝕩: Inputs other than natural numbers not yet supported" % 3 •math.LCM 1.5
!"𝕨 •math.LCM 𝕩: Unexpected argument types" % "foo" •math.LCM 3
!"𝕨 •math.GCD 𝕩: Unexpected argument types" % 3 •math.GCD {+}
!"•math.Sin 𝕩: 𝕩 contained non-number" % •math.Sin "foo"
!"•math.Log10 𝕩: 𝕩 contained non-number" % •math.Log10 "foo"
!"𝕨 •math.Atan2 𝕩: Unexpected argument types" % 1 •math.Atan2 'a'
!"𝕨 •math.Atan2⁼ 𝕩: Unexpected argument types" % 1 •math.Atan2⁼ {⇐}
!"𝕨 •math.Atan2˜⁼ 𝕩: Unexpected argument types" % {⇐} •math.Atan2˜⁼ 'b'

# •rand / •MakeRand
0 (•MakeRand 0).Range¨↕2 %% ⟨⟩‿⟨⟩
0‿0 (•MakeRand 0).Range¨↕2 %% ⟨⟩‿⟨⟩
(•MakeRand 0).Deal 0 %% ⟨⟩
0 (•MakeRand 0).Subset 0 %% ⟨⟩
{!(∧•rand.Deal 𝕩)≡↕𝕩}¨ ⥊ ¯1‿0‿1+⌜2⋆↕22 # %SLOW
!"𝕨 (rand).Range 𝕩: Result rank too large (256≡≢𝕨)" % (256⥊1) •rand.Range 0
! ≡˝ a←{r←•MakeRand 𝕩 ⋄ ⟨r.Range 1e9, 100 r.Range 1e9, r.Deal 100, 100 r.Deal 200, 100 r.Deal 200, 100 r.Subset 200⟩}¨ 2‿2⥊↕2 ⋄ !∘≢¨´ ⊏a
r←•MakeRand 1 ⋄ ! 1¨⊸≡ ∊{𝕊: 800 r.Subset 1000}¨ ↕4
r←•MakeRand 1 ⋄ ! 1¨⊸≡ ∊{𝕊: 50 r.Subset 10000}¨ ↕4
r←•MakeRand 1 ⋄ ! 1¨⊸≡ ∊{𝕊: 10    r.Range 10000}¨ ↕4
r←•MakeRand 1 ⋄ ! 1¨⊸≡ ∊{𝕊: 10‿10 r.Range 128}¨ ↕4
r←•MakeRand 1 ⋄ ! 1¨⊸≡ ∊{𝕊:     r.Deal 1000}¨ ↕4
r←•MakeRand 1 ⋄ ! 1¨⊸≡ ∊{𝕊: 500 r.Deal 1000}¨ ↕4

# •bit
!"•bit._cast: 𝕩 must have rank at least 1" % 8‿1 •bit._cast 123
(↕10) {! (𝕨⥊¯1) ≡ 1‿𝕩•bit._cast (𝕨×𝕩)⥊1}⌜ 8‿16‿32
%USE eqvar ⋄ {t←𝕩⊏1‿8‿16‿32‿64 ⋄ 1≠⊑t? {t•bit._cast 𝕩}_eqvar   ↕128; @}¨ ↕4‿4
%USE eqvar ⋄ {t←𝕩⊏1‿8‿16‿32‿64 ⋄       {t•bit._cast 𝕩}_eqvar 4=↕128}¨ ↕4‿4
%USE rand ⋄ ! 10‿1 ≡ ≢8‿16•bit._cast c←0↓2↕'a'+11 r.Range 26 ⋄ ! 10‿2 ≡ ≢c
%USE rand ⋄ ! 10‿1 ≡ ≢8‿16•bit._cast c←  2↕'a'+11 r.Range 26 ⋄ ! 10‿2 ≡ ≢c
!"•bit._cast: Incompatible lengths" % 1‿8•bit._cast 1⥊0
!"•bit._cast: Incompatible lengths" % 8‿32•bit._cast 2⥊0
!"•bit._cast: Incompatible lengths" % 8‿32•bit._cast 10⥊0

!"•bit._cast: Unsupported width 1073741824" % (2⥊2⋆30)•bit._cast ⋈0
!"•bit._cast: Unsupported width 1125899906842624" % (2⥊2⋆50)•bit._cast ⋈0 %USZ64
!"Integer out of range: 1.125899906842624e15" % (2⥊2⋆50)•bit._cast ⋈0 %USZ32
!"Integer out of range: 1.1805916207174113e21" % (2⥊2⋆70)•bit._cast ⋈0
!"Integer out of range: ∞" % (2⥊∞)•bit._cast ⋈0

!"•bit._cast: Unsupported width 12" % 1‿12•bit._cast 11⥊0
!"•bit._cast: Unsupported width 12" % 12‿32•bit._cast (12×32)⥊0
!"•bit._cast: Unsupported width 12" % 8‿12•bit._cast (8×12)⥊0
!"•bit._cast: Unsupported width 0" % 0‿32•bit._cast 128⥊0
!"•bit._cast: Unsupported width 0" % 8‿0•bit._cast 128⥊0

!"•bit._cast: Unsupported width 16 for type 'f'" % ⟨16‿'f',32⟩•bit._cast 128⥊0
!"•bit._cast: Unsupported width 32 for type 'f'" % ⟨32,32‿'f'⟩•bit._cast 128⥊0
!"•bit._cast: Unsupported width 0 for type 'u'" % ⟨0‿'u',32⟩•bit._cast 128⥊0
!"•bit._cast: Unsupported width 8 for type 'u'" % ⟨8‿'u',32⟩•bit._cast 128⥊0
!"•bit._cast: Unsupported width 32 for type 'u'" % ⟨8,32‿'u'⟩•bit._cast 128⥊0
!"•bit._cast: Unsupported width 1 for type 'i'" % ⟨8, 1‿'i'⟩•bit._cast 128⥊0

!"•bit._add: Unsupported data width 2" % 8‿2•bit._add˜ 16⥊0
!"•bit._add: Sizes in 𝕗 must be powers of 2 (contained 12)" % 8‿12•bit._add˜ 16⥊0
!"•bit._add: Sizes in 𝕗 must be powers of 2 (contained 0)" % 8‿0•bit._add˜ 16⥊0
!"•bit._add: Unsupported data width 128" % 8‿128•bit._add˜ 16⥊0

!"•bit._add: Operation width 1 too small" % 1‿32•bit._add˜ 16⥊0
!"•bit._add: Operation width 2 too small" % 2‿32•bit._add˜ 16⥊0
!"•bit._add: Sizes in 𝕗 must be powers of 2 (contained 3)" % 3‿32•bit._add˜ 16⥊0
!"•bit._add: Operation width 4 too small" % 4‿32•bit._add˜ 16⥊0
!"•bit._add: Operation width 128 too large" % 128‿32•bit._add˜ 16⥊0

(¯3×↕32) 64‿32‿8‿8•bit._mul ↕32 %% ¯184745984‿1721813476‿¯331589824‿1537122403‿¯587748096‿1074686809‿¯919600832‿435498697
¯100‿¯30 64‿32‿8‿32•bit._mul 100-7×↕32 %% 37979376‿7727981‿¯130450736‿1422540730‿¯1983181648‿664605348‿2143355536‿2079418098
2‿1•bit._not 0‿0‿1‿1‿0‿1‿1‿0 %% 1‿1‿0‿0‿1‿0‿0‿1
128‿1•bit._not 256⥊1‿0‿0 %% 256⥊0‿1‿1
1‿0 2‿1•bit._or 16⥊0‿0‿1‿1 %% 1‿0‿1‿1‿1‿0‿1‿1‿1‿0‿1‿1‿1‿0‿1‿1
(16⥊1‿0‿0‿0‿0) 2‿1•bit._or 16⥊0‿0‿1‿0 %% 1‿0‿1‿0‿0‿1‿1‿0‿0‿0‿1‿0‿0‿0‿1‿1
(256⥊1‿1‿0‿0) 128‿1•bit._or 256⥊1‿0 %% 256⥊1‿1‿1‿0
!"•bit._or: Scalar extension with width over 64 not supported" % (128⥊1) 128‿1•bit._or 256⥊1
!"•bit._neg: Operation width 2 too small" % 2‿1•bit._neg 0‿0‿1‿1‿0‿1‿1‿0
!"•bit._add: Operation width 4 too small" % 1‿0‿0‿0 4‿1•bit._add 8⥊1
0‿123 16‿16‿16‿8•bit._add -⊸∾ 4000×↕8 %% 31488‿27488‿23488‿19488‿15488‿11488‿7488‿3488‿31488‿¯30048‿¯26048‿¯22048‿¯18048‿¯14048‿¯10048‿¯6048
3141592 32•bit._mul 10⋆↕10 %% 3141592‿31415920‿314159200‿¯1153375296‿1351148928‿626587392‿1970906624‿¯1765770240‿¯477833216‿¯483364864

!"•bit._add: 𝕨 or 𝕩 1-cell width must equal operation width if extended" % 8‿8‿2‿8•bit._add˜ ↕32
!"•bit._add: 𝕨 or 𝕩 1-cell width must equal operation width if extended" % 8‿8‿8‿2•bit._add˜ ↕32
!"•bit._add: Unsupported data width 2" % 8‿2‿8‿8•bit._add˜ ↕32
!"•bit._add: Unsupported data width 2" %  8 8‿8‿2‿8•bit._add○↕ 32
!"•bit._add: Unsupported data width 2" % 32 8‿8‿8‿2•bit._add○↕ 8
!"•bit._not: Unsupported data width 2" % 8‿2‿8•bit._not ↕32
!"•bit._not: Unsupported data width 2" % 8‿8‿2•bit._not ↕32

%USE eqvar ⋄ {𝕊 f: {•internal.Keep 𝕩⍋1  ⋄ •internal.Validate F 𝕩}_eqvar  ↕64}¨ ({𝕩•bit._neg}¨ 8‿16‿32‿64) ∾ {𝕩 •bit._cast}¨ ≠´¨⊸/ ⥊ 8‿16‿32‿64 ⋈⌜ 1‿8‿16‿32‿64
%USE eqvar ⋄ {𝕊 f: {•internal.Keep 𝕩⍒1  ⋄ •internal.Validate F 𝕩}_eqvar ⌽↕64}¨ ({𝕩•bit._neg}¨ 8‿16‿32‿64) ∾ {𝕩 •bit._cast}¨ ≠´¨⊸/ ⥊ 8‿16‿32‿64 ⋈⌜ 1‿8‿16‿32‿64
%USE eqvar ⋄ {𝕊 f: {•internal.Squeeze 𝕩 ⋄ •internal.Validate F 𝕩}_eqvar 64↑2}¨ ({𝕩•bit._neg}¨ 8‿16‿32‿64) ∾ {𝕩 •bit._cast}¨ ≠´¨⊸/ ⥊ ⟨8⟩ ⋈⌜ 1‿8‿16‿32‿64
%USE eqvar ⋄ {𝕊 w: {⟨𝕩, w•bit._not 8↓𝕩⟩}_eqvar ↕16}¨ 8‿16‿32‿64

# •platform
Str ← {!=𝕩 ⋄ !×≠𝕩 ⋄ ! ∧´2=•Type¨𝕩} ⋄ Str •platform.os ⋄ Str •platform.cpu.arch ⋄ Str •platform.bqn.impl_version ⋄ Str •platform.environment
•platform.bqn.impl %% "CBQN"





# CBQN-specific extensions

# •_while_
1⊸+•_while_{𝕊: ∧´𝕩≤20} ↕10 %% 12+↕10
(10⥊1) +•_while_{𝕊: ∧´𝕩≤20} ↕10 %% 12+↕10

# •Hash
! ≡´  •Hash¨ 2⥊<⟨10,"ab"⟩
! ≡´1 •Hash¨ 2⥊<⟨10,"ab"⟩

# •ToUTF8 & •FromUTF8
@-˜•ToUTF8 "𝕩⍉hello" %% 240‿157‿149‿169‿226‿141‿137‿104‿101‿108‿108‿111
! •FromUTF8∘•ToUTF8⊸≡ "𝕩⍉hello"
(
  %USE eqvar
  ip←⋈¨ i←@+⍷0⌈∧(⥊(¯5+↕11)+⌜2⋆↕20)∾1114111-↕10
  {! 𝕩≡•FromUTF8•ToUTF8 𝕩 ⋄ •ToUTF8 _eqvar 𝕩}¨ ↑i
  t ← •ToUTF8¨ ip
  ! (∾t)≡•ToUTF8 i
  ! ip ≡ •FromUTF8¨ t
  ! (≠¨t) ≡ ¯1↓+`/⁼0‿49‿93‿148‿197
)

# •internal.GC
•internal.GC ↕10 %% 1
# •internal.Type & •internal.Variation & •internal.ElType & •internal.ListVariations
(
  raw ← {("a"⊸⋈¨ 𝕩)∾"s"⊸⋈¨ 1↓𝕩} ⟨"b","i8","i16","i32","f64","c8","c16","c32","h","f"⟩
  var ← {a‿e: ∾⟨a-32, e⟩}¨ raw
  ty ← {∾⌽ {"a":"arr";"slice"}‿{"b":"bit"; "f":"fill"; 𝕩} {𝕎𝕩}¨ 𝕩}¨ raw
  ty {"type"‿𝕨‿𝕩 ! 𝕨 ≡ •internal.Type 𝕩 •internal.Variation ⟨⟩}¨ var
  ! 0‿1‿2‿3‿4‿5‿6‿7‿8‿8‿1‿2‿3‿4‿5‿6‿7‿8‿8 ≡ {•internal.ElType 𝕩 •internal.Variation ⟨⟩}¨ var
  # TODO test •internal.ListVariations
)
%USE eqvarv ⋄ •internal.ListVariations _eqvarv_"i" π×↕10 %% ⟨"Af64","Sf64","Af64Inc","Sf64Inc","Ah","Sh","AhInc","ShInc","Af","Sf","AfInc","SfInc"⟩
%USE eqvarv ⋄ •internal.ListVariations _eqvarv_"i" (π×↕10)∾{⇐} %% ⟨"Ah","Sh","AhInc","ShInc","Af","Sf","AfInc","SfInc"⟩
%USE eqvarv ⋄ •internal.ListVariations _eqvarv_"i" ↕1000 %% ⟨"Ai16","Si16","Ai16Inc","Si16Inc","Ai32","Si32","Ai32Inc","Si32Inc","Af64","Sf64","Af64Inc","Sf64Inc","Ah","Sh","AhInc","ShInc","Af","Sf","AfInc","SfInc"⟩
%USE eqvarv ⋄ •internal.ListVariations _eqvarv_"i" 100⥊1‿0 %% ⟨"Ab","AbInc","Ai8","Si8","Ai8Inc","Si8Inc","Ai16","Si16","Ai16Inc","Si16Inc","Ai32","Si32","Ai32Inc","Si32Inc","Af64","Sf64","Af64Inc","Sf64Inc","Ah","Sh","AhInc","ShInc","Af","Sf","AfInc","SfInc"⟩

# •internal.Keep
•internal.Keep ↕10 %% ↕10
!"This function can't be called dyadically" % 1 •internal.Keep 1
# •internal.Refc
•internal.Refc •internal.Unshare ↕10 %% 1
a←•internal.Unshare ↕10 ⋄ b←10⥊<a ⋄ ! 11 ≡ •internal.Refc a ⋄ •internal.Keep b
# •internal.IsPure
•internal.IsPure¨ ⟨@,1,⊢,¨,∘,{⇐},1‿2,{𝕩}‿{𝔽}‿{𝔽𝔾𝕩}⟩ %% 8⥊1
! 1 ≡ •internal.IsPure ⊑⟨⊑∘+¨++⟩
! 1 ≡ •internal.IsPure ⊑⟨⊑⟜1‿2‿3∘+¨++⟩
! 1 ≡ •internal.IsPure ⊑⟨⊢◶⊢‿-⟩
! 0 ≡ •internal.IsPure ⊑⟨⊢◶{𝕩}‿-⟩
! 1 ≡ •internal.IsPure ⊑⟨⊑⟜1‿2‿{𝕩}∘+¨++⟩
! 0 ≡ •internal.IsPure •show
! 0 ≡ •internal.IsPure •file.chars
! 0 ≡ •internal.IsPure •internal.keep
! 1 ≡ •internal.IsPure •internal.pureKeep

! 0 ≡ •internal.IsPure ⊑⟨{𝕩}⊢⟩
! 0 ≡ •internal.IsPure ⊑⟨⊢{𝕩}⟩
! 0 ≡ •internal.IsPure ⊑⟨{𝕩}⊢⊢⟩
! 0 ≡ •internal.IsPure ⊑⟨⊢{𝕩}⊢⟩
! 0 ≡ •internal.IsPure ⊑⟨⊢⊢{𝕩}⟩
! 0 ≡ •internal.IsPure ⊑⟨{𝕩}¨⟩
! 0 ≡ •internal.IsPure ⊑⟨⊢{𝔽𝕩}⟩
! 0 ≡ •internal.IsPure ⊑⟨{𝕩}∘⊢⟩
! 0 ≡ •internal.IsPure ⊑⟨⊢{𝔽𝔾𝕩}⊢⟩
! 0 ≡ •internal.IsPure ⊑⟨⊢∘{𝕩}⟩

# •internal.Info
•internal.Info 1 %% "3ff0000000000000: not heap-allocated"
33↑•internal.Info •internal.Unshare ↕10 %% "fff7: refc:1 type:23=i8arr alloc:"
# •internal.HasFill
•internal.HasFill¨ ⟨↕0, ↕10, ⟨⟩, 'a'‿1, ""⟩ %% 1‿1‿0‿0‿1
# •internal.Squeeze
a←"Ai32"•internal.Variation ↕10 ⋄ •internal.Type •internal.Squeeze a %% "i8arr"
{! 𝕩 ≡ •internal.Squeeze 𝕩}¨ {0≥•Cmp´0‿¯1⊏𝕩? 𝕩⍋@; 𝕩⍒@}⊸⊢¨ ∾{∨´1e9≤|0‿¯1⊏𝕩? ⟨𝕩⟩; ⟨𝕩,"Ai32"•internal.Variation 𝕩⟩}¨ 1‿1000⊸/¨ ∾{⟨0‿𝕩,𝕩‿0⟩}¨ 1‿2‿¯1 ∾ ¯1⊸-⊸∾ ⥊ 0‿¯1 +⌜ 2⋆7‿15‿31
# •internal.DeepSqueeze
a←⋈"Ai32"•internal.Variation ↕10 ⋄ •internal.Type ⊑•internal.DeepSqueeze a %% "i8arr"
# •internal.ClearRefs
•internal.ClearRefs@ ⋄ •internal.Variation⟜(↕10)¨ 10⥊<"Ai32Inc" ⋄ •internal.ClearRefs "foo" %% 10
# •internal.Unshare
•internal.Refc∘•internal.Unshare¨ ⟨↕0, "", ⟨⟩, ↕10⟩ %% 1‿1‿1‿1
⟨a⟩‿b‿c ← •internal.Unshare ⟨{a⇐↕2},"hi",3⟩ ⋄ a‿b‿c %% ⟨0‿1, "hi", 3⟩
# •internal.EEqual
%USE nan ⋄ a←1⌽nans∾•ParseFloat¨"0"‿"1.2"‿"-0" ⋄ a •internal.EEqual ⌽a %% 1
# •internal.Validate
! •internal.Validate⊸≡ ↕10
! •internal.Validate⊸≡ ∨↕1000
