%DEF test bin←(⊑•args).Bin@ ⋄ Test←{{132:1; 134:1; 𝕩}⌾⊑ {(n←2+≠bin)≤≠𝕩? (bin∾": ")≡n↑𝕩? "(self-bin): "∾n↓𝕩; 𝕩}⌾(2⊸⊑) {'␊'¨⌾((𝕩=@+10)⊸/)𝕩}¨⌾(1⊸↓) (𝕨{s𝕊𝕩: stdin⇐s; ⇐}@) •SH ⟨bin⟩∾𝕩} ⋄ lf←@+10

%DEF t0 %USE test ⋄ c0←"•Show ¯4" ⋄ c1←"!0" ⋄ c2←"aa" ⋄ c3←"a⇐1"

%USE test ⋄ Test "-e"‿"2+2" %% ⟨0,⟨⟩,⟨⟩⟩
%USE test ⋄ Test "-o"‿"2+2" %% ⟨1,⟨⟩,"Error: (-o): Value to print must be a string␊"⟩
%USE test ⋄ Test "-o"‿"3⥊'a'" %% ⟨0,"aaa␊",⟨⟩⟩
%USE test ⋄ Test "-p"‿"2+2" %% ⟨0,"4␊",⟨⟩⟩

%USE test ⋄ Test "-e"‿"!0" %% ⟨1,⟨⟩,"Error: Assertion error␊(-e):1:␊  !0␊  ^␊"⟩
%USE test ⋄ Test "-o"‿"!0" %% ⟨1,⟨⟩,"Error: Assertion error␊(-o):1:␊  !0␊  ^␊"⟩
%USE test ⋄ Test "-p"‿"!0" %% ⟨1,⟨⟩,"Error: Assertion error␊(-p):1:␊  !0␊  ^␊"⟩

%USE test ⋄ 6⊸↑⌾(1⊸⊑) Test ⋈"--help" %% ⟨0,"Usage:",⟨⟩⟩
%USE test ⋄ 5⊸↑⌾(1⊸⊑) Test ⋈"--version" %% ⟨0,"CBQN ",⟨⟩⟩

%USE t0 ⋄ Test "-e"‿c0 %% ⟨0,"¯4␊",⟨⟩⟩
%USE t0 ⋄ Test "-e"‿c1 %% ⟨1,⟨⟩,"Error: Assertion error␊(-e):1:␊  !0␊  ^␊"⟩
%USE t0 ⋄ Test "-e"‿c2 %% ⟨1,⟨⟩,"Error: Undefined identifier␊(-e):1:␊  aa␊  ^^␊"⟩
%USE t0 ⋄ Test "-e"‿c3 %% ⟨0,⟨⟩,⟨⟩⟩

%USE t0 ⋄ c0 Test ⋈"-" %% ⟨0,"¯4␊",⟨⟩⟩
%USE t0 ⋄ c1 Test ⋈"-" %% ⟨1,⟨⟩,"Error: Assertion error␊(stdin):1:␊  !0␊  ^␊"⟩
%USE t0 ⋄ c2 Test ⋈"-" %% ⟨1,⟨⟩,"Error: Undefined identifier␊(stdin):1:␊  aa␊  ^^␊"⟩
%USE t0 ⋄ c3 Test ⋈"-" %% ⟨0,⟨⟩,⟨⟩⟩
%USE test ⋄ "•args" Test "-"‿"--foo" %% ⟨0,⟨⟩,⟨⟩⟩

%USE t0 ⋄ c0 Test ⋈"-s" %% ⟨0,"¯4␊¯4␊",⟨⟩⟩
%USE t0 ⋄ c1 Test ⋈"-s" %% ⟨0,⟨⟩,"Error: Assertion error␊at !0␊   ^␊"⟩
%USE t0 ⋄ c2 Test ⋈"-s" %% ⟨0,⟨⟩,"Error: Undefined identifier␊at aa␊   ^^␊"⟩
%USE t0 ⋄ c3 Test ⋈"-s" %% ⟨0,"1␊",⟨⟩⟩

%USE t0 ⋄ (")e "∾c0) Test ⋈"-s" %% ⟨0," •Show ¯4 ␊       │  ␊ •Show─¯4 ␊╶──┘      ␊",⟨⟩⟩
%USE t0 ⋄ (")e "∾c1) Test ⋈"-s" %% ⟨0," !0 ␊  │ ␊ !0 ␊╶┘  ␊",⟨⟩⟩
%USE t0 ⋄ (")e "∾c2) Test ⋈"-s" %% ⟨0,⟨⟩,"Error: Undefined identifier␊at aa␊   ^^␊"⟩
%USE t0 ⋄ (")e "∾c3) Test ⋈"-s" %% ⟨0," a⇐1 ␊ │   ␊ a─1 ␊╶──┘ ␊",⟨⟩⟩

%USE t0 ⋄ Test ⟨"-e", "•BQN "∾•Repr c0⟩ %% ⟨0,"¯4␊",⟨⟩⟩
%USE t0 ⋄ Test ⟨"-e", "•BQN "∾•Repr c1⟩ %% ⟨1,⟨⟩,"Error: Assertion error␊at !0␊   ^␊(-e):1:␊  •BQN ""!0""␊  ^^^^␊"⟩
%USE t0 ⋄ Test ⟨"-e", "•BQN "∾•Repr c2⟩ %% ⟨1,⟨⟩,"Error: Undefined identifier␊at aa␊   ^^␊(-e):1:␊  •BQN ""aa""␊  ^^^^␊"⟩
%USE t0 ⋄ Test ⟨"-e", "•BQN "∾•Repr c3⟩ %% ⟨0,⟨⟩,⟨⟩⟩

%USE t0 ⋄ Test ⟨"-e", "f←•ReBQN{primitives⇐⋈⟨'!',!∘'e'⟩}"∾lf∾"F "∾•Repr c0⟩ %% ⟨0,"¯4␊",⟨⟩⟩
%USE t0 ⋄ Test ⟨"-e", "f←•ReBQN{primitives⇐⋈⟨'!',!∘'e'⟩}"∾lf∾"F "∾•Repr c1⟩ %% ⟨1,⟨⟩,"Error: 'e'␊at !0␊   ^␊(-e):2:␊  F ""!0""␊  ^␊"⟩
%USE t0 ⋄ Test ⟨"-e", "f←•ReBQN{primitives⇐⋈⟨'!',!∘'e'⟩}"∾lf∾"F "∾•Repr c2⟩ %% ⟨1,⟨⟩,"Error: Undefined identifier␊at aa␊   ^^␊(-e):2:␊  F ""aa""␊  ^␊"⟩
%USE t0 ⋄ Test ⟨"-e", "f←•ReBQN{primitives⇐⋈⟨'!',!∘'e'⟩}"∾lf∾"F "∾•Repr c3⟩ %% ⟨0,⟨⟩,⟨⟩⟩

%USE t0 ⋄ Test ⟨"-e", "f←•ReBQN{primitives⇐⋈⟨'!',!∘'e'⟩,repl⇐""loose""}"∾lf∾"F "∾•Repr c0⟩ %% ⟨0,"¯4␊",⟨⟩⟩
%USE t0 ⋄ Test ⟨"-e", "f←•ReBQN{primitives⇐⋈⟨'!',!∘'e'⟩,repl⇐""loose""}"∾lf∾"F "∾•Repr c1⟩ %% ⟨1,⟨⟩,"Error: 'e'␊at !0␊   ^␊(-e):2:␊  F ""!0""␊  ^␊"⟩
%USE t0 ⋄ Test ⟨"-e", "f←•ReBQN{primitives⇐⋈⟨'!',!∘'e'⟩,repl⇐""loose""}"∾lf∾"F "∾•Repr c2⟩ %% ⟨1,⟨⟩,"Error: Undefined identifier␊at aa␊   ^^␊(-e):2:␊  F ""aa""␊  ^␊"⟩
%USE t0 ⋄ Test ⟨"-e", "f←•ReBQN{primitives⇐⋈⟨'!',!∘'e'⟩,repl⇐""loose""}"∾lf∾"F "∾•Repr c3⟩ %% ⟨1,⟨⟩,"Error: Cannot construct a namespace for a REPL result␊(-e):2:␊  F ""a⇐1""␊  ^␊"⟩

%USE t0 ⋄ Test ⟨"-e", "f←•ReBQN{primitives⇐⋈⟨'!',!∘'e'⟩,repl⇐""strict""}"∾lf∾"F "∾•Repr c0⟩ %% ⟨0,"¯4␊",⟨⟩⟩
%USE t0 ⋄ Test ⟨"-e", "f←•ReBQN{primitives⇐⋈⟨'!',!∘'e'⟩,repl⇐""strict""}"∾lf∾"F "∾•Repr c1⟩ %% ⟨1,⟨⟩,"Error: 'e'␊at !0␊   ^␊(-e):2:␊  F ""!0""␊  ^␊"⟩
%USE t0 ⋄ Test ⟨"-e", "f←•ReBQN{primitives⇐⋈⟨'!',!∘'e'⟩,repl⇐""strict""}"∾lf∾"F "∾•Repr c2⟩ %% ⟨1,⟨⟩,"Error: Undefined identifier␊at aa␊   ^^␊(-e):2:␊  F ""aa""␊  ^␊"⟩
%USE t0 ⋄ Test ⟨"-e", "f←•ReBQN{primitives⇐⋈⟨'!',!∘'e'⟩,repl⇐""strict""}"∾lf∾"F "∾•Repr c3⟩ %% ⟨1,⟨⟩,"Error: Cannot construct a namespace for a REPL result␊(-e):2:␊  F ""a⇐1""␊  ^␊"⟩

%USE test ⋄ Test ⟨"-f"⟩ %% ⟨1,⟨⟩,"(self-bin): -f requires an argument␊"⟩
%USE test ⋄ Test ⟨"-M123"⟩ %% ⟨1,⟨⟩,"(self-bin): Value of -M must be in a separate argument␊"⟩
%USE test ⋄ Test ⟨"-M", 99⥊'9'⟩ %% ⟨1,⟨⟩,"(self-bin): -M: Too large␊"⟩
%USE test ⋄ Test ⟨"-M", "1.5"⟩ %% ⟨1,⟨⟩,"(self-bin): -M: Argument not a number␊"⟩
%USE test ⋄ Test ⟨"-!"⟩ %% ⟨1,⟨⟩,"(self-bin): Unknown option: -!␊"⟩

%USE test ⋄ Test ⟨"--bad-opt"⟩ %% ⟨1,⟨⟩,"(self-bin): Unknown option: --bad-opt␊"⟩
%USE test ⋄ Test ⟨"-Xbad-opt"⟩ %% ⟨1,⟨⟩,"(self-bin): Unknown -X option: -Xbad-opt␊"⟩
%USE test ⋄ Test ⟨"-Xhelp=123"⟩ %% ⟨1,⟨⟩,"(self-bin): Flag -Xhelp must not be given a value␊"⟩
%USE test ⋄ Test ⟨"-Xlog-file"⟩ %% ⟨1,⟨⟩,"(self-bin): Flag -Xlog-file must be given a value␊"⟩

%USE test ⋄ Test ⟨"-Xffi-debug=foo"⟩ %% ⟨1,⟨⟩,"(self-bin): Invalid value for -Xffi-debug␊"⟩
%USE test ⋄ Test ⟨"-Xrepl-history=foo"⟩ %% ⟨1,⟨⟩,"(self-bin): Invalid value for -Xrepl-history␊"⟩
%USE test ⋄ Test ⟨"-Xrand-seed=12a"⟩ %% ⟨1,⟨⟩,"(self-bin): Invalid value for -Xrand-seed␊"⟩
%USE test ⋄ Test ⟨"-Xhash-seed="⟩ %% ⟨1,⟨⟩,"(self-bin): Invalid value for -Xhash-seed␊"⟩
%USE test ⋄ Test ⟨"-Xinternal-seed=x"⟩ %% ⟨1,⟨⟩,"(self-bin): Invalid value for -Xinternal-seed␊"⟩
%USE test ⋄ Test ⟨"-Xinternal-seed=999999999999999999999999999"⟩ %% ⟨1,⟨⟩,"(self-bin): Invalid value for -Xinternal-seed␊"⟩
%USE test ⋄ Test ⟨"-Xrand-seed= 123"⟩ %% ⟨1,⟨⟩,"(self-bin): Invalid value for -Xrand-seed␊"⟩
%USE test ⋄ Test ⟨"-Xprofile=u"⟩ %% ⟨1,⟨⟩,"(self-bin): Invalid value for -Xprofile␊"⟩
%USE test ⋄ Test ⟨"-Xprofile=00"⟩ %% ⟨1,⟨⟩,"Cannot profile with 0hz sampling frequency␊"⟩
