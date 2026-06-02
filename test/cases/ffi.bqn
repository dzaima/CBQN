%DEF var V←•internal.Variation ⋄ LV←•internal.ListVariations ⋄ CLR←•internal.ClearRefs
%DEF tvar %USE var ⋄ _tvar ← {F _𝕣 x: (CLR@) ⊢ {F 𝕩 V x}¨ LV 𝕩; w F _𝕣 x: (CLR@) ⊢ (LV 𝕨) {(𝕨 V w) F 𝕩 V x}⌜ LV 𝕩}
%DEF sizesize 8
%DEF _errFFICore {𝕨 p _𝕣 𝕩: m←(@+10)=s←•CurrentError@ ⋄ "TEST FAIL: expected 2 newlines"!2=+´m ⋄ msg‿l0‿l1 ← m(⊢-˜+`×¬)⊸⊔s ⋄ ! msg∾"; "∾ ∾ (""‿"→"⊏˜" ^"⊐ (' '¨p)∾⁼ l1»' '¨l0) ∾¨ p∾⁼l0}
%DEF ErrFFI @⊸•FFI⎊("at 𝕨 •FFI " %USE _errFFICore)

•Type @•FFI {1↓0∾𝕩}¨   ""‿"bqn_init"‿"*i8" %% 3
•Type @•FFI {1↓0∾𝕩}¨   ⟨⟩‿"bqn_init"‿"*i8" %% 3
•Type @•FFI {1↓0∾𝕩}¨ (↕0)‿"bqn_init"‿"*i8" %% 3

# bad •FFI invocation
# generally weird
!"•FFI: 𝕩 must be a list" % @•FFI @
!"•FFI: 𝕩 must be a list" % @•FFI 2‿2⥊<"a"
!"This function can't be called monadically" % •FFI ""‿"bqn_init"
!"•FFI: Symbol name must be a string" % @•FFI "hello"
!"•FFI: Symbol name must be a string" % @•FFI ""‿1‿"i32"
!"•FFI: Symbol name must be a string" % @•FFI ""‿["ab","cd"]‿"i32"
!"•FFI: Symbol name must be a string" % @•FFI 5⥊@
!"•FFI: Too many arguments" % @•FFI""‿"bqn_init"∾70000⥊<"i32"
!"FFI type: Unexpected character where type is expected: '?' (@+63); ·‿""bqn_init""‿""→?""" % %USE ErrFFI ""‿"bqn_init"‿"?"
!"FFI type: expected number; ·‿""bqn_init""‿""[→""" % %USE ErrFFI ""‿"bqn_init"‿"["
!"FFI type: Expected ']'; ·‿""bqn_init""‿""[0→""" % %USE ErrFFI ""‿"bqn_init"‿"[0"
!"FFI type: Expected type, got end of string; ·‿""bqn_init""‿""[1]→""" % %USE ErrFFI ""‿"bqn_init"‿"[1]"
!"FFI type: expected number; ·‿""bqn_init""‿""[→]i32""" % %USE ErrFFI ""‿"bqn_init"‿"[]i32"
!"FFI type: Expected type, got end of string; ·‿""bqn_init""‿""{→""" % %USE ErrFFI ""‿"bqn_init"‿"{"
!"FFI type: Expected 'i' to be followed by either a number, or the text ""size""; ·‿""bqn_init""‿""→i""" % %USE ErrFFI ""‿"bqn_init"‿"i"
!"FFI type: Unsupported width in f80; ·‿""bqn_init""‿""→f80""" % %USE ErrFFI ""‿"bqn_init"‿"f80"
!"FFI type: Unsupported width in f128; ·‿""bqn_init""‿""→f128""" % %USE ErrFFI ""‿"bqn_init"‿"f128"
!"FFI type: Unsupported width in f16; ·‿""bqn_init""‿""→f16""" % %USE ErrFFI ""‿"bqn_init"‿"f16"
!"FFI type: Unsupported width in f1; ·‿""bqn_init""‿""→f1""" % %USE ErrFFI ""‿"bqn_init"‿"f1"
!"FFI type: Unsupported width in u1; ·‿""bqn_init""‿""→u1""" % %USE ErrFFI ""‿"bqn_init"‿"u1"
!"FFI type: Unsupported width in i0; ·‿""bqn_init""‿""→i0""" % %USE ErrFFI ""‿"bqn_init"‿"i0"
!"FFI type: Unsupported width in i10; ·‿""bqn_init""‿""→i10""" % %USE ErrFFI ""‿"bqn_init"‿"i10"
!"FFI type: Unsupported width in u128; ·‿""bqn_init""‿""→u128""" % %USE ErrFFI ""‿"bqn_init"‿"u128"
!"FFI type: Type string cannot be empty" % @•FFI""‿"bqn_init"‿""
!"FFI type: number literal too large; ·‿""bqn_init""‿""u999999999→99999""" % %USE ErrFFI ""‿"bqn_init"‿"u99999999999999"
!"FFI type: Types must not contain null characters; ·‿""bqn_init""‿""i32→␀""" % %USE ErrFFI ⟨"", "bqn_init", "i32"∾@⟩
!"FFI type: Types must not contain null characters; ·‿""bqn_init""‿""→␀i32""" % %USE ErrFFI ⟨"", "bqn_init", @∾"i32"⟩
!"FFI type: Types must not contain null characters; ·‿""bqn_init""‿""{i32,→␀i32}""" % %USE ErrFFI ⟨"", "bqn_init", "{i32,"∾@∾"i32}"⟩
!"FFI type: Types must not contain null characters; ·‿""bqn_init""‿""i32→␀""""hello""""""" % %USE ErrFFI ⟨"", "bqn_init", "i32"∾@∾"""hello"""⟩
!"•FFI: Symbol name must not contain a null character" % @•FFI ⟨"", "bqn_init"∾@⟩
!"FFI type: Expected 'i' to be followed by either a number, or the text ""size""; ""→invalid""‿""bqn_init""‿·‿·‿·‿·‿·‿·‿·‿·‿·‿·" % %USE ErrFFI ⟨"invalid", "bqn_init", ↑, ↑, ↓, ↓, <, >, <, >, ⌊, ⍉⟩
!"FFI: '>' specified multiple times on the same arg; ·‿""bqn_init""‿·‿"">→>i32""‿·‿·" % %USE ErrFFI ⟨"i32", "bqn_init", "i32", ">>i32", @, 5⟩
!"FFI type: Types must not contain null characters; ""→␀i32""‿""bqn_init""" % %USE ErrFFI ⟨@∾"i32", "bqn_init"⟩
!"FFI type: Unexpected character where type is expected: '.' (@+46); ·‿""bqn_init""‿"">→...""" % %USE ErrFFI ""‿"bqn_init"‿">..."
!"•FFI: Cannot have multiple instances of ""...""" % @•FFI""‿"bqn_init"‿"..."‿"..."

# bad symbols/library
!"•FFI: Path must be a list of characters" % 1‿2 •FFI ""‿"bqn_init"
!"•FFI: Path must be a list of characters" % (2‿2⥊@) •FFI ""‿"bqn_init"
!"•FFI: Path must be a list of characters" % ∘‿2 •FFI ""‿"bqn_init"
!"•FFI: Failed to load library: " % "/dev/file that doesn't exist" •FFI⎊{𝕊: ! 30↑•CurrentError@} ""‿"free"
!"•FFI: Failed to load library: " % ↑‿"library that doesn't exist" •FFI⎊{𝕊: ! 30↑•CurrentError@} ""‿"free"
!"•foreign.Pointer: Failed to load library: " % "/dev/file that doesn't exist" •foreign.Pointer⎊{𝕊: ! 42↑•CurrentError@} "i32"‿"free"

!"↑‿path •FFI 𝕩: Path must be a list of characters" % ↑‿2 •FFI ""‿"bqn_init"
!"↑‿path •FFI 𝕩: Path must be a list of characters" % ⟨↑,'a'‿2⟩ •FFI ""‿"bqn_init"
!"↑‿path •FFI 𝕩: Path must be a list of characters" % ⟨↑,2‿2⥊"a"⟩ •FFI ""‿"bqn_init"
!"↑‿path •FFI 𝕩: Path cannot contain slashes" % ↑‿"foo/bar.so" •FFI ""‿"bqn_init"
!"↑‿path •foreign.Function 𝕩: Path cannot contain slashes" % ↑‿"foo/bar.so" •foreign.Function ""‿"bqn_init"
!"↑‿path •foreign.Pointer 𝕩: Path must be a list of characters" % ↑‿@ •foreign.Pointer "i32"‿"free"

!"•FFI: Failed to find symbol: " % @ •FFI⎊{𝕊: ! 29↑•CurrentError@} ""‿"bqn_this symbol doesn't exist"
!"•foreign.Value: Failed to find symbol: " % @ •foreign.Value⎊{𝕊: ! 39↑•CurrentError@} ""‿"bqn_this symbol doesn't exist"

# wrong usage of •foreign stuff
!"•foreign.Function 𝕩: Second element must be a pointer object, got array" % •foreign.Function ""‿"bqn_init"
!"•foreign.Function 𝕩: Second element must be a pointer object, got some other kind of namespace" % •foreign.Function ""‿{⇐}
!"•foreign.Sizeof: 𝕩 must be a string" % •foreign.Sizeof 'a'
!"FFI type: Unexpected character where type is expected: '?' (@+63); at •foreign.Sizeof ""→???""" % •foreign.Sizeof⎊("" %USE _errFFICore) "???"
!"FFI type: Unexpected character where type is expected: '?' (@+63); at 𝕨 •foreign.Pointer ""→???""‿·" % @ •foreign.Pointer⎊("" %USE _errFFICore) "???"‿"free"
!"FFI type: Unexpected character where type is expected: '?' (@+63); at 𝕨 •foreign.Value ""→???""‿·" % @ •foreign.Value⎊("" %USE _errFFICore) "???"‿"free"
!"•foreign.Value: Second element of 𝕩 must be a string" % @ •foreign.Value @‿"free"
!"•foreign.Value: Symbol name must be a string" % @ •foreign.Value "i32"‿@
!"•foreign.Value: 𝕩 must be a two-item list" % @ •foreign.Value "i32"‿"i32"‿"i32"
!"•foreign.Value: 𝕩 must be a two-item list" % @ •foreign.Value ≍"i32"‿"free"
!"•foreign.Pointer: 𝕩 must be a two-item list" % @ •foreign.Pointer "i32"
!"•foreign.Pointer: 𝕩 must be a two-item list" % @ •foreign.Pointer @
!"FFI type: Type string cannot be empty" % @ •foreign.Value ""‿"free"
!"FFI: Expected list 𝕩, but 𝕩 was an atom" % {𝕏@} •foreign.Function ⟨"i32", •foreign.null⟩
!  "•foreign.ReadBytesTo0 𝕩: 𝕩 must be either an untyped pointer, or point to a size-1 primitive" %   •foreign.ReadBytesTo0 •foreign.null.Cast "*i8:c8"
!  "•foreign.ReadCharsTo0 𝕩: 𝕩 must be either an untyped pointer, or point to a size-1 primitive" %   •foreign.ReadCharsTo0 •foreign.null.Cast "i8:i8"
!"𝕨 •foreign.ReadBytesTo0 𝕩: 𝕩 must be either an untyped pointer, or point to a size-1 primitive" % 0 •foreign.ReadBytesTo0 •foreign.null.Cast "i8:u1"
!"𝕨 •foreign.ReadCharsTo0 𝕩: 𝕩 must be either untyped or have stride 1" % 0 •foreign.ReadCharsTo0 •foreign.null.Cast "i16:c8"
!  "•foreign.ReadBytesTo0 𝕩: 𝕩 must be either untyped or have stride 1" %   •foreign.ReadBytesTo0 (•foreign.null.Cast "{i8,i8}").Field 0


# >/𝕨/𝕩
!"FFI: '>' specified multiple times on the same arg; ·‿""bqn_init""‿"">→>i32""" % %USE ErrFFI ""‿"bqn_init"‿">>i32"
!"FFI: Must have at most one of '𝕨' or '𝕩' on one arg; ·‿""bqn_init""‿""𝕨→𝕨i32""" % %USE ErrFFI ""‿"bqn_init"‿"𝕨𝕨i32"
!"FFI: Must have at most one of '𝕨' or '𝕩' on one arg; ·‿""bqn_init""‿""𝕩→𝕩i32""" % %USE ErrFFI ""‿"bqn_init"‿"𝕩𝕩i32"
!"FFI: Must have at most one of '𝕨' or '𝕩' on one arg; ·‿""bqn_init""‿""𝕨→𝕩i32""" % %USE ErrFFI ""‿"bqn_init"‿"𝕨𝕩i32"
!"•FFI: Cannot use "">𝕩"" with multiple arguments on 𝕩" % @•FFI""‿"bqn_init"‿">i32"‿">i32"
!"•FFI: Cannot use "">𝕨"" with multiple arguments on 𝕨" % @•FFI""‿"bqn_init"‿">𝕨i32"‿">𝕨i32"
!"•FFI: Cannot use "">𝕩"" with multiple arguments on 𝕩" % @•FFI""‿"bqn_init"‿">i32"‿"i32"
!"•FFI: Cannot use "">𝕩"" with multiple arguments on 𝕩" % @•FFI""‿"bqn_init"‿"i32"‿">i32"
!"•FFI: Cannot use "">𝕨"" with multiple arguments on 𝕨" % @•FFI""‿"bqn_init"‿">𝕨i32"‿"𝕨i32"

# arrays
!"FFI type: 0-element arrays not supported; ·‿""bqn_init""‿""[→0]i32""" % %USE ErrFFI ""‿"bqn_init"‿"[0]i32"
!"FFI type: 0-element arrays not supported; ·‿""bqn_init""‿""[000→0]i32""" % %USE ErrFFI ""‿"bqn_init"‿"[0000]i32"
!"FFI type: number literal too large; ·‿""bqn_init""‿""[999999999→999]i32""" % %USE ErrFFI ""‿"bqn_init"‿"[999999999999]i32"

# structs
!"FFI type: Expected ',' or '}' after struct field; ·‿""bqn_init""‿""{*i32→*i32}""" % %USE ErrFFI ""‿"bqn_init"‿"{*i32*i32}"
!"FFI type: Unfinished '{'; ·‿""bqn_init""‿""{*i32→""" % %USE ErrFFI ""‿"bqn_init"‿"{*i32"
!"FFI type: structs must have at least one field; ·‿""bqn_init""‿""{→{}}""" % %USE ErrFFI ""‿"bqn_init"‿"{{}}"
!"FFI type: Expected type, got end of string; ·‿""bqn_init""‿""{*i32,→""" % %USE ErrFFI ""‿"bqn_init"‿"{*i32,"

# :
!"FFI type: Expected type to end; ·‿""bqn_init""‿""a→:u1""" % %USE ErrFFI ""‿"bqn_init"‿"a:u1"
!"FFI type: Unexpected trailing ':'; ·‿""bqn_init""‿""→i32:""" % %USE ErrFFI ""‿"bqn_init"‿"i32:"
!"FFI type: ':' must be followed by one of 'i', 'u', or 'c'; ·‿""bqn_init""‿"">*→u64:q8""" % f←%USE ErrFFI ""‿"bqn_init"‿">*u64:q8"
!"FFI type: expected number; ·‿""bqn_init""‿""i32:i→""" % %USE ErrFFI ""‿"bqn_init"‿"i32:i"
!"FFI type: ':i9' not supported; ·‿""bqn_init""‿""→i32:i9""" % %USE ErrFFI ""‿"bqn_init"‿"i32:i9"
!"FFI type: ':u16' not supported; ·‿""bqn_init""‿""→i32:u16""" % %USE ErrFFI ""‿"bqn_init"‿"i32:u16"
!"FFI type: ':i64' not supported; ""→i32:i64""‿""bqn_init""" % %USE ErrFFI "i32:i64"‿"bqn_init"
!"FFI type: ':' must be followed by one of 'i', 'u', or 'c'; ·‿""bqn_init""‿""→i32:f32""" % %USE ErrFFI ""‿"bqn_init"‿"i32:f32"
!"FFI type: ':' must be followed by one of 'i', 'u', or 'c'; ·‿""bqn_init""‿""→i32:f64""" % %USE ErrFFI ""‿"bqn_init"‿"i32:f64"
!"FFI type: number literal too large; ·‿""bqn_init""‿""i32:u999999999→9999999""" % %USE ErrFFI ""‿"bqn_init"‿"i32:u9999999999999999"
!"FFI type: Expected type to end; ·‿""bqn_init""‿""i32→ hello world""" % %USE ErrFFI ""‿"bqn_init"‿"i32 hello world"

# representation widths
!"FFI type: i8:i32 has a wider representation than its value; ·‿""bqn_init""‿""→i8:i32""" % %USE ErrFFI ""‿"bqn_init"‿"i8:i32"
!"FFI type: i8:i32 has a wider representation than its value; ·‿""bqn_init""‿""[1]→i8:i32""" % %USE ErrFFI ""‿"bqn_init"‿"[1]i8:i32"
!"FFI type: i8:i32 has a wider representation than its value; ·‿""bqn_init""‿""[2]→i8:i32""" % %USE ErrFFI ""‿"bqn_init"‿"[2]i8:i32"
!"FFI type: i8:c32 has a wider representation than its value; ·‿""bqn_init""‿""[15]→i8:c32""" % %USE ErrFFI ""‿"bqn_init"‿"[15]i8:c32"
!"FFI type: i8:i32 has a wider representation than its value; ·‿""bqn_init""‿""[4]{→i8:i32}""" % %USE ErrFFI ""‿"bqn_init"‿"[4]{i8:i32}"
!"FFI type: i8:i32 has a wider representation than its value; ·‿""bqn_init""‿""⥊→i8:i32""" % %USE ErrFFI ""‿"bqn_init"‿"⥊i8:i32"
!"FFI type: i8:i32 has a wider representation than its value; ·‿""bqn_init""‿""[4]→i8:i32""‿·‿·" % %USE ErrFFI ""‿"bqn_init"‿"[4]i8:i32"‿"[100]i8:i32"‿"[100]i8:c32"
!"FFI type: i8:i32 has a wider representation than its value; ·‿""bqn_init""‿""*→i8:i32""" % %USE ErrFFI ""‿"bqn_init"‿"*i8:i32"
!"FFI type: i8:i32 has a wider representation than its value; ·‿""bqn_init""‿""&→i8:i32""" % %USE ErrFFI ""‿"bqn_init"‿"&i8:i32"
!"FFI: Expected 6-element array to be passed to [3]u64:i32, but got array with shape ⟨0⟩" % f ← @•FFI ""‿"bqn_init"‿"[3]u64:i32" ⋄ F ⋈⟨⟩
!"FFI: Expected 24-element array to be passed to [3]u64:i8, but got array with shape ⟨1⟩" % f ← @•FFI ""‿"bqn_init"‿"[3]u64:i8" ⋄ F ⋈⋈1
!"FFI: Expected 48-element array to be passed to [3]i16:u1, but got array with shape ⟨47⟩" % f ← @•FFI ""‿"bqn_init"‿"[3]i16:u1" ⋄ F ⋈47⥊0
!"FFI: A non-negative integer must be passed to ⥊T, but got array" % f ← @•FFI ""‿"bqn_init"‿"⥊i32" ⋄ F ⟨↕10⟩
!"FFI: A non-negative integer must be passed to ⥊T, but got invalid number" % f ← @•FFI ""‿"bqn_init"‿"⥊i32" ⋄ F ⟨¯5⟩
!"FFI: A non-negative integer must be passed to ⥊T, but got invalid number" % f ← @•FFI ""‿"bqn_init"‿">⥊i32" ⋄ F 2.5
!"FFI: A non-negative integer must be passed to ⥊T, but got character" % f ← @•FFI ""‿"bqn_init"‿">⥊i32" ⋄ F @

# pointers
!"FFI type: Expected type, got end of string; ·‿""bqn_init""‿""&→""" % %USE ErrFFI ""‿"bqn_init"‿"&"
!"FFI type: Unexpected character where type is expected: ':' (@+58); ·‿""bqn_init""‿""&→:i8""" % %USE ErrFFI ""‿"bqn_init"‿"&:i8"
!"FFI type: ""&T"" cannot be used here; ·‿""bqn_init""‿""[1]→&i8""" % %USE ErrFFI ""‿"bqn_init"‿"[1]&i8"
!"FFI type: ""&T"" cannot be used here; ·‿""bqn_init""‿""[4]→&i8""" % %USE ErrFFI ""‿"bqn_init"‿"[4]&i8"
!"FFI type: ""&T"" cannot be used here; ·‿""bqn_init""‿""{[4]→&i8}""" % %USE ErrFFI ""‿"bqn_init"‿"{[4]&i8}"
!"FFI type: ""&T"" cannot be used here; ·‿""bqn_init""‿""*→&i8""" % %USE ErrFFI ""‿"bqn_init"‿"*&i8"
!"FFI type: ""⥊T"" cannot be used here; ·‿""bqn_init""‿""*{*i8,→⥊i8,*i8}""" % %USE ErrFFI ""‿"bqn_init"‿"*{*i8,⥊i8,*i8}"
!"FFI type: Cannot do *·T; ·‿""bqn_init""‿""→*·i8""" % %USE ErrFFI ""‿"bqn_init"‿"*·i8"
!"FFI type: Cannot do *·T; ·‿""bqn_init""‿""&{→*·i8}""" % %USE ErrFFI ""‿"bqn_init"‿"&{*·i8}"

# return value
!"•FFI: Function specification must have at least two items" % @•FFI ⟨"&"⟩
!"•FFI: Can only have result type of ""&"" when exactly one object is mutated, but 0 are" % @•FFI"&"‿"bqn_init"‿">*i32"
!"•FFI: Can only have result type of ""&"" when exactly one object is mutated, but 2 are" % @•FFI"&"‿"bqn_init"‿"&i32"‿"&i32"
!"•FFI: Functions cannot return arrays; ""→[4]i32""‿""bqn_init""" % %USE ErrFFI "[4]i32"‿"bqn_init"
!"FFI type: ""&T"" can only be used in function argument types; ""→&i32""‿""bqn_init""" % %USE ErrFFI "&i32"‿"bqn_init"
!"FFI type: ""⥊T"" can only be used in function argument types; ""→⥊i32""‿""bqn_init""" % %USE ErrFFI "⥊i32"‿"bqn_init"
!"FFI type: ""⥊T"" can only be used in function argument types; ""*→⥊i32""‿""bqn_init""" % %USE ErrFFI "*⥊i32"‿"bqn_init"
!"•FFI: Functions cannot return arrays; ""→[4]&i32""‿""bqn_init""" % %USE ErrFFI "[4]&i32"‿"bqn_init"

!"Expected pointer object or array, got number" % f←@•FFI"&"‿"bqn_init"‿">&i32" ⋄ F 0
!"Expected pointer object or array, got character" % f←@•FFI"&"‿"bqn_init"‿"i32"‿"&i32" ⋄ F 0‿@

# special types
!"FFI type: Expected ""bool""; ·‿""bqn_init""‿""{→booL,i32}""" % %USE ErrFFI ""‿"bqn_init"‿"{booL,i32}"
!"FFI type: Expected ""bool""; ·‿""bqn_init""‿""{→b,i32}"""    % %USE ErrFFI ""‿"bqn_init"‿"{b,i32}"
!"FFI type: Expected ""bool""; ·‿""bqn_init""‿""→b"""          % %USE ErrFFI ""‿"bqn_init"‿"b"
!"FFI type: Expected ""bool""; ·‿""bqn_init""‿""→bo"""         % %USE ErrFFI ""‿"bqn_init"‿"bo"
!"FFI type: Expected ""bool""; ·‿""bqn_init""‿""→boo"""        % %USE ErrFFI ""‿"bqn_init"‿"boo"
!"FFI type: Expected type to end; ·‿""bqn_init""‿""*bool→:u1""" % %USE ErrFFI ""‿"bqn_init"‿"*bool:u1"
•Type @•FFI"*bool"‿"bqn_init"‿"bool"‿"{bool}" %% 3

# correct elements
f←@•FFI"&"‿"memcpy"‿"&u8"‿"*i8"‿"usize" ⋄ G←{F⟨𝕩,↕0,0⟩} ⋄ %USE tvar ⋄ !∘≡¨⟜⊏ G _tvar 100⥊1‿0
f←@•FFI"&"‿"memcpy"‿"&u8"‿"*i8"‿"usize" ⋄ G←{F⟨𝕩,↕0,0⟩} ⋄ G ↕200 %% ↕200
f←@•FFI"&"‿"memcpy"‿"&u16"‿"*i8"‿"usize" ⋄ G←{F⟨𝕩,↕0,0⟩} ⋄ G ⋈65535 %% ⋈65535
f←@•FFI"&"‿"memcpy"‿"&u32"‿"*i8"‿"usize" ⋄ G←{F⟨𝕩,↕0,0⟩} ⋄ G (2⋆32)|¯5+↕10 %% (2⋆32)|¯5+↕10
f←@•FFI"&"‿"memcpy"‿"&i16"‿"*i8"‿"usize" ⋄ G←{F⟨𝕩,↕0,0⟩} ⋄ G ¯32768‿32767 %% ¯32768‿32767
f←@•FFI"&"‿"memcpy"‿"&{i16}"‿"*i8"‿"usize" ⋄ G←{F⟨𝕩,↕0,0⟩} ⋄ G ⋈¨¯32768‿32767 %% ⋈¨¯32768‿32767
f←@•FFI"&"‿"memcpy"‿"&f32"‿"*i8"‿"usize" ⋄ G←{F⟨𝕩,↕0,0⟩} ⋄ G ⟨∞,¯∞,∞÷∞⟩∾10÷˜↕10 %% ⟨∞,¯∞,∞÷∞⟩∾0‿0.10000000149011612‿0.20000000298023224‿0.30000001192092896‿0.4000000059604645‿0.5‿0.6000000238418579‿0.699999988079071‿0.800000011920929‿0.8999999761581421
f←@•FFI"&"‿"memcpy"‿"&i32"‿"*i16:u1"‿"usize" ⋄ F ⟨⋈5, 32⥊1, 4⟩ %% ⋈¯1
f←@•FFI⟨"&","memcpy","&i8","*i8","usize:u1"⟩ ⋄ F ⟨¯100+↕10, ↕10, (8×•foreign.Sizeof"usize")↑0‿0‿1⟩ %% (↕4)∾4↓¯100+↕10
f←@•FFI"&"‿"memcpy"‿"&i16"‿"*i32"‿"usize" ⋄ F ⟨¯100+↕10, ⟨123+456×2⋆16, ¯2⟩, 8⟩ %% 123‿456‿¯2‿¯1∾4↓¯100+↕10
f←@•FFI⟨"usize","strlen","*i8"⟩ ⋄ ⟨⟩ F ⋈1‿2‿3‿0 %% 3
f←@•FFI⟨"&","memcpy","&i8","[4]i8","usize"⟩ ⋄ F ⟨10⥊0, 3+↕4, 4⟩ %% 3‿4‿5‿6‿0‿0‿0‿0‿0‿0


# bad array elements
!"FFI: Improper value for u8: 256" % f←@•FFI"&"‿"bqn_init"‿">&u8" ⋄ F ↕300
!"FFI: Improper value for u8: ¯1" % f←@•FFI"&"‿"bqn_init"‿">&u8" ⋄ F -↕100
!"FFI: Improper value for u16: 65536" % f←@•FFI"&"‿"bqn_init"‿">&u16" ⋄ F ⋈65536
!"FFI: Improper value for u32: 4294967296" % f←@•FFI"&"‿"bqn_init"‿">&u32" ⋄ F ⋈2⋆32
!"FFI: Improper value for u16: ¯1" % f←@•FFI"&"‿"bqn_init"‿">&u16" ⋄ F ⋈¯1
!"FFI: Improper value for u32: ¯1" % f←@•FFI"&"‿"bqn_init"‿">&u32" ⋄ F ⋈¯1
!"FFI: Improper value for bool: ¯1" % f←@•FFI""‿"bqn_init"‿">*bool" ⋄ F ⋈¯1
!"FFI: Improper value for bool: 2" % f←@•FFI""‿"bqn_init"‿">&bool" ⋄ F ⋈2

# bad scalars
!"FFI: Improper value for u8: 256" % f←@•FFI""‿"bqn_init"‿">u8" ⋄ F 256
!"FFI: Improper value for u16: ¯1" % f←@•FFI""‿"bqn_init"‿">u16" ⋄ F ¯1
!"FFI: Improper value for u32: ¯1" % f←@•FFI""‿"bqn_init"‿">u32" ⋄ F ¯1
!"FFI: Improper value for u64: ¯1" % f←@•FFI""‿"bqn_init"‿">u64" ⋄ F ¯1
!"FFI: Improper value for u32: 4294967296" % f←@•FFI""‿"bqn_init"‿">u32" ⋄ F 2⋆32
!"FFI: Improper value for u32: 1e20" % f←@•FFI""‿"bqn_init"‿">u32" ⋄ F 1e20
!"FFI: Improper value for u64: 9.007199254740992e15" % f←@•FFI""‿"bqn_init"‿">u64" ⋄ F 2⋆53
!"FFI: Improper value for i64: 9.007199254740992e15" % f←@•FFI""‿"bqn_init"‿">i64" ⋄ F 2⋆53
!"FFI: Improper value for i64: ¯9.007199254740992e15" % f←@•FFI""‿"bqn_init"‿">i64" ⋄ F -2⋆53
!"FFI: Improper value for i32: character" % f←@•FFI""‿"bqn_init"‿">𝕨i32"‿">i64" ⋄ @ F 3
!"FFI: Improper value for i32: array" % f←@•FFI""‿"bqn_init"‿">𝕨i32"‿">i64" ⋄ ⟨1⟩ F 3
!"FFI: Improper value for i64: character" % f←@•FFI""‿"bqn_init"‿"i64" ⋄ F ⟨@⟩
!"FFI: Improper value for bool: character" % f←@•FFI""‿"bqn_init"‿"bool" ⋄ F ⟨@⟩
!"FFI: Improper value for bool: 2" % f←@•FFI""‿"bqn_init"‿"bool" ⋄ F ⟨2⟩


# bad overall argument separation
!"FFI: Expected list 𝕩, but 𝕩 was an atom" % f←@•FFI""‿"bqn_init"‿"i32"‿"i32" ⋄ F @
!"FFI: Wrong value for 𝕩: Expected list with 2 elements, got array with shape 1‿2" % f←@•FFI""‿"bqn_init"‿"i32"‿"i32" ⋄ F ≍⟨↕2, ↕2⟩
!"FFI: Wrong value for 𝕨: Expected list with 2 elements, got array with shape 1‿2" % f←@•FFI""‿"bqn_init"‿"𝕨i32"‿"𝕨i32" ⋄ (≍1‿2) F ⟨⟩
!"FFI: Wrong value for 𝕩: Expected list with 2 elements, got array with shape ⟨3⟩" % f←@•FFI""‿"bqn_init"‿"i32"‿"i32" ⋄ F ⟨↕2, ↕2, ↕2⟩
!"FFI: Wrong value for 𝕩: Expected list with 2 elements, got array with shape ⟨1⟩" % f←@•FFI""‿"bqn_init"‿"i32"‿"i32" ⋄ F ⟨↕2⟩
!"FFI: Wrong value for 𝕨: Expected list with 2 elements, got array with shape ⟨3⟩" % f←@•FFI""‿"bqn_init"‿"i32"‿"𝕨i32"‿"𝕨i32" ⋄ ⟨↕2, ↕2, ↕2⟩ F ⟨4⟩
!"FFI: Wrong value for 𝕩: Expected list with 0 elements, got array with shape ⟨1⟩" % f←@•FFI""‿"bqn_init" ⋄ F ⟨1⟩
!"FFI: Expected list 𝕩, but 𝕩 was an atom" % f←@•FFI""‿"bqn_init" ⋄ F @
!"FFI: Expected list 𝕩, but 𝕩 was an atom" % f←@•FFI""‿"bqn_init"‿"𝕨i32"‿"𝕨i32" ⋄ 1‿2 F @
!"FFI: Wrong value for 𝕩: Expected list with 0 elements, got array with shape ⟨2⟩" % f←@•FFI""‿"bqn_init"‿">𝕨i32" ⋄ 4 F 1‿2
!"FFI: Wrong value for 𝕨: Expected list with 0 elements, got array with shape ⟨2⟩" % f←@•FFI""‿"bqn_init"‿"*i8" ⋄ ⟨1,2⟩ F 1‿2‿3‿0
!"FFI: Expected list 𝕨, but 𝕨 was an atom" % f←@•FFI""‿"bqn_init"‿"*i8" ⋄ @ F 1‿2‿3‿0
!"This function can't be called monadically" % f←@•FFI""‿"bqn_init"‿"𝕨i32"‿"𝕨i32" ⋄ F ⟨⟩
!"This function can't be called monadically" % f←@•FFI""‿"bqn_init"‿"𝕨i32"‿"i32" ⋄ F ⟨3⟩
!"This function can't be called monadically" % f←@•FFI""‿"bqn_init"‿">𝕨i32"‿">i32" ⋄ F 3



# wrong argument internal structure
!"Expected pointer object or array, got character" % f←@•FFI""‿"bqn_init"‿">*[2]i32" ⋄ F @
!"Expected pointer object or array, got character" % f←@•FFI""‿"bqn_init"‿">*{i32}" ⋄ F @
!"FFI: Expected array to be passed to struct, but got number" % f←@•FFI""‿"bqn_init"‿">{i32}" ⋄ F 0
!"FFI: Expected array to be passed to struct, but got number" % f←@•FFI""‿"bqn_init"‿">*{i32}" ⋄ F ⟨0⟩
!"FFI: Expected array to be passed to [1]i32, but got number" % f←@•FFI""‿"bqn_init"‿">*[1]i32" ⋄ F ⟨0⟩
!"FFI: Expected array to be passed to [1]i32, but got namespace" % f←@•FFI""‿"bqn_init"‿">[1]i32" ⋄ F {⇐}
!"FFI: Expected array to be passed to [2]*T, but got character" % f←@•FFI""‿"bqn_init"‿">*[2]****i8" ⋄ F ⟨@⟩
!"FFI: Expected 1-element array to be passed to struct, but got array with shape ⟨0⟩" % f←@•FFI""‿"bqn_init"‿">*{i32}" ⋄ F ⟨⟨⟩⟩
!"FFI: Expected 1-element array to be passed to struct, but got array with shape ⟨2⟩" % f←@•FFI""‿"bqn_init"‿">*{i32}" ⋄ F ⟨1‿2⟩
!"FFI: Expected 2-element array to be passed to [2]i32, but got array with shape ⟨9⟩" % f←@•FFI""‿"bqn_init"‿">[2]i32" ⋄ F ↕9
!"FFI: Expected 2-element array to be passed to [2]i32, but got array with shape ⟨0⟩" % f←@•FFI""‿"bqn_init"‿">*[2]i32" ⋄ F ⟨⟨⟩⟩
!"FFI: Expected 2-element array to be passed to [2]i32, but got array with shape ⟨3⟩" % f←@•FFI""‿"bqn_init"‿">*[2]i32" ⋄ F ⟨1‿2‿3⟩
!"FFI: Expected 2-element array to be passed to [2]{...}, but got array with shape ⟨3⟩" % f←@•FFI""‿"bqn_init"‿">*[2]{i32}" ⋄ F ⟨1‿2‿3⟩

# bad :
!"FFI: Expected array to be passed to i32:i8" % f←@•FFI""‿"bqn_init"‿">i32:i8" ⋄ F @
!"Expected pointer object or array, got character" % f←@•FFI""‿"bqn_init"‿">*i32:i8" ⋄ F @

!"FFI: Expected 4-element array to be passed to i32:i8, but got array with shape ⟨10⟩" % f←@•FFI""‿"bqn_init"‿">i32:i8" ⋄ F ↕10
!"FFI: Expected 64-element array to be passed to u64:u1, but got array with shape ⟨128⟩" % f←@•FFI""‿"bqn_init"‿">u64:u1" ⋄ F 128⥊1

!"FFI: Improper value for u1: 2" % f←@•FFI""‿"bqn_init"‿">u64:u1" ⋄ F ↕64
!"FFI: Expected 8-element array to be passed to u64:i8, but got array with shape ⟨64⟩" % f←@•FFI""‿"bqn_init"‿">u64:i8" ⋄ F ↕64

!"Expected pointer object or array, got character" % f←@•FFI""‿"bqn_init"‿">*u64:i8" ⋄ F @
!"FFI: Improper value for i8: 128" % f←@•FFI""‿"bqn_init"‿">*u64:i8" ⋄ F ↕200
!"FFI: Improper value for i8: 128" % f←@•FFI""‿"bqn_init"‿">&u64:i8" ⋄ F ↕200
!"FFI: Improper value for u1: 2" % f←@•FFI""‿"bqn_init"‿">&u64:u1" ⋄ F ↕64
!"FFI: Array provided for *u64:i8 needs to have a length that's a multiple of 8, but array had shape ⟨2⟩" % f←@•FFI""‿"bqn_init"‿">*u64:i8" ⋄ F ↕2
!"FFI: Array provided for *u32:i16 needs to have a length that's a multiple of 2, but array had shape ⟨1⟩" % f←@•FFI""‿"bqn_init"‿">*u32:i16" ⋄ F ↕1
!"FFI: Array provided for &u64:u1 needs to have a length that's a multiple of 64, but array had shape ⟨32⟩" % f←@•FFI""‿"bqn_init"‿">&u64:u1" ⋄ F 32⥊0

!"FFI: Improper value for i16: 32768" % f←@•FFI"&"‿"bqn_init"‿">&i16" ⋄ F ¯32768‿32768
!"FFI: Improper value for i16: ¯32769" % f←@•FFI"&"‿"bqn_init"‿">&i16" ⋄ F ¯32768‿¯32769

!"FFI: Improper value for u8: array" % f←@•FFI"&"‿"bqn_init"‿">&u8" ⋄ F ⟨↕300⟩
!"FFI: Improper value for i16: character" % f←@•FFI"&"‿"bqn_init"‿">&i16" ⋄ F "hi"
!"FFI: Improper value for f64: namespace" % f←@•FFI"&"‿"bqn_init"‿">&f64" ⋄ F 1‿2‿{⇐}

!"FFI: Improper value for c8: array" % f←@•FFI""‿"bqn_init"‿">*i8:c8" ⋄ F ⋈"hello"
!"Expected pointer object or array, got number" % f←@•FFI""‿"bqn_init"‿">*i8:c8" ⋄ F 4
!"FFI: Improper value for c8: number" % f←@•FFI""‿"bqn_init"‿">*i8:c8" ⋄ F ⋈4
!"FFI: Improper value for c8: number" % f←@•FFI""‿"bqn_init"‿">*i8:c8" ⋄ F "Ah"•internal.Variation⋈4

!"FFI: Improper value for i8: 1000"      % f←@•FFI""‿"bqn_init"‿"*i8:c8"  ‿"i8" ⋄ F "hello"‿1000
!"FFI: Improper value for c8: @+9033"    % f←@•FFI""‿"bqn_init"‿"*i8:c8"  ‿"i8" ⋄ F "hello ⍉"‿1000
!"FFI: Improper value for c8: @+120169"  % f←@•FFI""‿"bqn_init"‿"*i8:c8"  ‿"i8" ⋄ F "hello 𝕩"‿1000
!"FFI: Improper value for i8: 1000"      % f←@•FFI""‿"bqn_init"‿"*i16:c16"‿"i8" ⋄ F "hello"‿1000
!"FFI: Improper value for i8: 1000"      % f←@•FFI""‿"bqn_init"‿"*i16:c16"‿"i8" ⋄ F "hello ⍉"‿1000
!"FFI: Improper value for c16: @+120169" % f←@•FFI""‿"bqn_init"‿"*i16:c16"‿"i8" ⋄ F "hello 𝕩"‿1000
!"FFI: Improper value for i8: 1000"      % f←@•FFI""‿"bqn_init"‿"*i32:c32"‿"i8" ⋄ F "hello"‿1000
!"FFI: Improper value for i8: 1000"      % f←@•FFI""‿"bqn_init"‿"*i32:c32"‿"i8" ⋄ F "hello ⍉"‿1000
!"FFI: Improper value for i8: 1000"      % f←@•FFI""‿"bqn_init"‿"*i32:c32"‿"i8" ⋄ F "hello 𝕩"‿1000

f←@•FFI"*"‿"memcpy"‿"&i8"‿"*i8:c8"‿"usize" ⋄ 1⊑F ⟨↕5, "Ah"•internal.Variation"hello", 5⟩ %% 104‿101‿108‿108‿111



# pointer objects
%DEF mm Calloc ← {1‿𝕩 {𝕏𝕨} @•FFI⟨"*"∾𝕨, "calloc", "usize", "usize"⟩} ⋄ free ← @•FFI""‿"free"‿">*" ⋄ cpy ← @•FFI"*:i8"‿"memcpy"‿"*"‿"*"‿"usize" ⋄ CpyB ← 256|Cpy
%DEF WallocE %USE mm ⋄ {allocs←⟨⟩ ⋄ Clean←{Free¨allocs⋄𝕩} ⋄ Clean 𝕏⎊{𝕊: !•CurrentError Clean @} {{r ⊣ allocs∾↩ r← 𝕨 Calloc 𝕩}}}
%DEF Walloc  %USE mm ⋄ {allocs←⟨⟩ ⋄ Clean←{Free¨allocs⋄𝕩} ⋄ Clean 𝕏                             {{r ⊣ allocs∾↩ r← 𝕨 Calloc 𝕩}}}

# .Field
%USE Walloc {{𝕊al: 1∘("{i32,i32,i32}" Al 99).Field⎊0¨ ¯2+↕8}} %% 0‿0‿1‿1‿1‿0‿0‿0
%USE Walloc {{𝕊al: 1∘("[5]u8"         Al 99).Field⎊0¨ ¯2+↕9}} %% 0‿0‿1‿1‿1‿1‿1‿0‿0
%USE Walloc {{𝕊al: 1∘("[5]u8"         Al 99).Field⎊0¨ ⟨∞,¯∞,0÷0,@,"foo",{⇐}⟩}} %% 6⥊0
!"Cannot get field 11 of a struct with 3 fields" % (•foreign.null.Cast "{i32,i8,i8}").Field 11
!"Cannot get pointer to element 111 of an array with 10 elements" % (•foreign.null.Cast "[10][2]{i64,i8}").Field 111

# .Write, .Read
%USE Walloc {{𝕊al: p←"{i16,i16,i16,i64}" Al 99 ⋄ p.Write ¯4‿10000‿¯30000‿7e12 ⋄ s← +´(2⋆8×↕∘≠)⊸× -○{CpyB 0∾˜⋈˜𝕩}´ ⟨p.Field 2, p⟩ ⋄ Cpy ⟨p.Add 1, p, s⟩ ⋄ p.Read¨ ↕2}} %% ⟨¯4‿10000‿¯30000‿7e12, ¯4‿10000‿0‿0⟩
%USE Walloc {{𝕊al: p←"{i32,{i64,i64}}" Al 999 ⋄ {𝕩 p.Write ⟨𝕩×10,𝕩×20‿30⟩}¨ ↕4 ⋄ ((p.Field 1).Field 0).Read¨ ↕4 }} %% 20×↕4
!"(pointer).Field 𝕩: Pointer type must be either array or struct" % %USE WallocE {{𝕊al: ("i32" Al 99).Field 0}}
!"FFI: Cannot pass an array to *{...} in this context, must use pointer objects" % %USE WallocE {{𝕊al: ("*{i32}"Al 99).Write ↕16}}
!"FFI: Cannot pass an array to *i32 in this context, must use pointer objects" % %USE WallocE {{𝕊al: ("*i32" Al 99).Write 1‿2‿3}}
!"FFI: Cannot pass an array to *i32 in this context, must use pointer objects" % %USE WallocE {{𝕊al: ("*i32" Al 99).WriteList ⋈1‿2‿3}}
!"FFI: Cannot pass an array to *i32:i32 in this context, must use pointer objects" % %USE WallocE {{𝕊al: ("*i32:i32"Al 99).Write ↕16}}
%USE Walloc {{𝕊al: p←"i32" Al 16 ⋄ p.Write¨⟜(10⊸×)˜ ↕4 ⋄ p2 ← "*i32:i8" Al 8 ⋄ p2.Write p ⋄ (p2.Read 0).Read 2}} %% 20‿0‿0‿0
%USE Walloc {{𝕊al: p←("" Al 99).Cast "*:i8" ⋄ a←"i32"Al 99 ⋄ a.Write 12345 ⋄ p.Write ar←Cpy a‿a‿0 ⋄ ! ar ≡ p.Read 0 ⋄ a.Read 0}} %% 12345
%USE Walloc {{𝕊al: p←"i16" Al 99 ⋄ raw ← Cpy 0∾˜⋈˜p.Add 3 ⋄ cpy2←@•FFI"*i16"‿"memcpy"‿"*:i8"‿"*:i8"‿"usize" ⋄ p.Sub Cpy2 raw‿raw‿0 }} %% ¯3

# .WriteList, .ReadList
!"(pointer).WriteList 𝕩: 𝕩 must be a list" % •foreign.null.WriteList 0
!"(pointer).WriteList 𝕩: 𝕩 must be a list" % •foreign.null.WriteList 0‿0⥊0
!"Out of memory" % (•foreign.null.Cast "i8").ReadList 2⋆50

# garbage arguments
!"Expected integer, got character" % %USE WallocE {{𝕊al: p←"i8" Al 1 ⋄ p.Add '!'}}
!"Expected pointer object or integer, got character" % %USE WallocE {{𝕊al: p←"i8" Al 1 ⋄ p.Sub '!'}}
!"Expected non-negative integer, got character" % %USE WallocE {{𝕊al: p←"i8" Al 1 ⋄ p.Field '!'}}
!"(pointer).Cast: 𝕩 must be a string" % %USE WallocE {{𝕊al: p←"i8" Al 1 ⋄ p.Cast '!'}}
!"Expected integer, got character" % %USE WallocE {{𝕊al: p←"i8" Al 1 ⋄ p.Read '!'}}
!"FFI: Improper value for i8: character" % %USE WallocE {{𝕊al: p←"i8" Al 1 ⋄ p.Write '!'}}

!"FFI: Cannot pass an array to *i64 in this context, must use pointer objects" % f←@•FFI""‿"bqn_init"‿"&{*i64}" ⋄   F ⟨   3⥊<⋈↕7⟩
⋄                  f←@•FFI"*i32:u1"‿"memcpy"‿"⥊{*i16}"‿"*{*i64}"‿"usize" ⋄ {((𝕩.Add 1).Cast"i8").Sub 𝕩.Cast"i8"}⚇0 F ⟨3, 3⥊<⋈↕7,     3×•foreign.Sizeof"usize"⟩   %% ⟨4,⟨⟨2⟩,⟨2⟩,⟨2⟩⟩⟩
%USE Walloc {{𝕊al: f←@•FFI"*i32:u1"‿"memcpy"‿"⥊{*i16}"‿"*{*i64}"‿"usize" ⋄ {((𝕩.Add 1).Cast"i8").Sub 𝕩.Cast"i8"}⚇0 F ⟨3, 3⥊<⋈""Al 1, 3×•foreign.Sizeof"usize"⟩}} %% ⟨4,⟨⟨2⟩,⟨2⟩,⟨2⟩⟩⟩
%USE Walloc {{𝕊al: f←@•FFI"*i32:u1"‿"memcpy"‿"⥊{*i16}"‿"&{*i64}"‿"usize" ⋄ {((𝕩.Add 1).Cast"i8").Sub 𝕩.Cast"i8"}⚇0 F ⟨3, 3⥊<⋈""Al 1, 3×•foreign.Sizeof"usize"⟩}} %% ⟨4,⟨⟨2⟩,⟨2⟩,⟨2⟩⟩,⟨⟨8⟩,⟨8⟩,⟨8⟩⟩⟩

# untyped pointers
!"(pointer).Field 𝕩: Pointer type must be either array or struct" % %USE WallocE {{𝕊al: ("" Al 99).Field 0}}
# %USE Walloc {{𝕊al: p←"i32" Al 99 ⋄ p.Write 123 ⋄ (((p.Cast "").Add 0).Cast"i32").Read 0}} %% 123
!"Cannot offset an untyped pointer" % %USE WallocE {{𝕊al: ("" Al 99).Add 0}}
!"Cannot offset an untyped pointer" % %USE WallocE {{𝕊al: ("" Al 99).Add 1}}
!"(pointer).WriteList 𝕩: pointer must not be untyped" % •foreign.null.WriteList ↕10
!"Cannot offset an untyped pointer" % •foreign.null.Write 0
!"Cannot offset an untyped pointer" % •foreign.null.Read 0

# .Add, .Sub, .Cast
!"FFI type: Expected type to end; at (pointer).Cast ""i32→f32""" % %USE WallocE {{𝕊al: p←"i8" Al 1 ⋄ p.Cast "i32f32"}⎊("" %USE _errFFICore)}
!"FFI type: Expected type, got end of string; at (pointer).Cast ""{→""" % %USE WallocE {{𝕊al: p←"{i32,*}" Al 1 ⋄ p.Cast "{"}⎊("" %USE _errFFICore)}
!"FFI type: 0-element arrays not supported; at (pointer).Cast ""{[4]i32,[→0]i32}""" % %USE WallocE {{𝕊al: p←"{i32,*}" Al 1 ⋄ p.Cast "{[4]i32,[0]i32}"}⎊("" %USE _errFFICore)}
!"(pointer).Cast: 𝕩 must be a string" % %USE WallocE {{𝕊al: p←"{i32,*}" Al 1 ⋄ p.Cast @}}
!"(pointer).Cast: 𝕩 must be a string" % %USE WallocE {{𝕊al: p←"{i32,*}" Al 1 ⋄ p.Cast 'i'‿3‿'2'}}
%USE Walloc {{𝕊al: p←"{i32,*}" Al 1 ⋄ p.Sub p.Cast 1↓0∾"{i32,*i32}"}} %% 0
%USE Walloc {{𝕊al: p←"{i32,*}" Al 1 ⋄ p.Sub p.Add 123}} %% ¯123
%USE Walloc {{𝕊al: p←"{i32,*}" Al 1 ⋄ p.Sub p.Sub 123}} %% 123
•Type¨ •foreign.null.Cast¨ ⟨⟨⟩, ↕0⟩ %% 6‿6

# ptr1.Sub ptr2
!"(pointer).Sub ptr: Both pointers must be typed" % %USE WallocE {{𝕊al: p←"" Al 1 ⋄ p.Sub p}}
!"(pointer).Sub ptr: Both pointers must be typed" % %USE WallocE {{𝕊al: p←"i8" Al 1 ⋄ (p.Cast "").Sub p.Cast "{{i8,i16},i32}"}}
!"(pointer).Sub ptr: Both pointers must be typed" % %USE WallocE {{𝕊al: p←"i8" Al 1 ⋄ (p.Cast "{{i8,i16},i32}").Sub p.Cast ""}}
!"(pointer).Sub ptr: Arguments must have the same stride" % %USE WallocE {{𝕊al: p←"i8" Al 1 ⋄ ((p.Cast "{{i8,i16},i32}").Field 0).Sub p.Cast "{i8,i16}"}}
!"(pointer).Sub ptr: Arguments must have compatible types" % %USE WallocE {{𝕊al: p←"i8" Al 1 ⋄ p.Sub p.Cast "u8"}}
!"(pointer).Sub ptr: Arguments must have compatible types" % {(𝕩.Cast "i16").Sub 𝕩.Cast "i8"} •foreign.null
!"(pointer).Sub ptr: Distance between pointers isn't an exact multiple of stride" % %USE WallocE {{𝕊al: p←"i8" Al 1 ⋄ (p.Cast "i32").Sub (p.Add 6).Cast "i32"}}

# type comparison
%USE Walloc {{𝕊al: p←"i8" Al 1 ⋄ t←"{{i8,i16,**i8,[12]i8,*:i32,i64:u1,*,*f64},i32,i64:c8}" ⋄ (p.Cast t).Sub p.Cast t}} %% 0 # catch-less test of most passing things to run under heapverify
%DEF TyEq {exp𝕊a‿b: %USE Walloc {{𝕊al: p←"i8" Al 1 ⋄ ! (exp⊑99‿0) ≡ ((p.Cast a).Sub)⎊99 p.Cast b}}}
1 %USE TyEq ⟨"{*i16}", "{*i16}"⟩
0 %USE TyEq ⟨"{i16}", "{i16,i16}"⟩
0 %USE TyEq ⟨"{i16,i16,i16}", "{i16,i16}"⟩
0 %USE TyEq ⟨"{*i16}", "{*u16}"⟩
1 %USE TyEq ⟨"{*i16}", "{*}"⟩
1 %USE TyEq ⟨"{*}",    "{*i16}"⟩
1 %USE TyEq ⟨"{*:i8}", "{*i16}"⟩
1 %USE TyEq ⟨"{*:i8}", "{*i16:i8}"⟩
1 %USE TyEq ⟨"{*:i8}", "{*usize:c32}"⟩

1 %USE TyEq ⟨"{{i8,i16},i32}", "{{i8,i16},i32}"⟩
0 %USE TyEq ⟨"{{i8,i16},i32}", "{{u8,i16},i32}"⟩
1 %USE TyEq ⟨"{[3]i32,[5]i32}", "{[3]i32,[5]i32}"⟩
1 %USE TyEq ⟨"{[3]i32,[5]i32}", "{[3]i32,[5]i32:i8}"⟩
0 %USE TyEq ⟨"{[3]i32,[5]i32}", "{[3]i32,[5]u32}"⟩
0 %USE TyEq ⟨"{[3]i32,[5]i32}", "{[5]i32,[3]i32}"⟩
0 %USE TyEq ⟨"[4]i32", "[3]i32"⟩
0 %USE TyEq ⟨"*[4]i32", "*[3]i32"⟩
0 %USE TyEq ⟨"{i32}", "i32"⟩
0 %USE TyEq ⟨"i16", "i32"⟩
0 %USE TyEq ⟨"i16", "u16"⟩
1 %USE TyEq ⟨"{i32:i8,i64}", "{i32:i8,i64}"⟩
0 %USE TyEq ⟨"{i32:i8,i64}", "{u32:i8,i64}"⟩
0 %USE TyEq ⟨"{i32:i8,i64}", "{i16:i8,i64}"⟩
0 %USE TyEq ⟨"*{i32:i8}", "*{i16:i8}"⟩
1 %USE TyEq ⟨"{i32:i8,i64}", "{i32:i16,i64}"⟩
1 %USE TyEq ⟨"{i32:i32,i64}","{i32:u1,i64}"⟩
1 %USE TyEq ⟨"{i32:i8,i64}", "{i32,i64}"⟩
1 %USE TyEq ⟨"*i32:i8", "*"⟩
1 %USE TyEq ⟨"*i32:i8", "*:c32"⟩
1 %USE TyEq ⟨"*i32", "*:c32"⟩
0 %USE TyEq ⟨"**", "*i8"⟩
0 %USE TyEq ⟨"*{*:i8}", "*{i8}"⟩
0 %USE TyEq ⟨"*{*}", "*{i8}"⟩
0 %USE TyEq ⟨"*{i8}", "*{*}"⟩

# passing pointer objects as arguments
!"FFI: Pointer object type isn't compatible with argument type" % %USE WallocE {{𝕊al: f ← @•FFI""‿"bqn_init"‿">*i8" ⋄ p←"i16" Al 10 ⋄ F p}}
%USE Walloc {{𝕊al: p ← "i16" Al 99 ⋄ p.Write¨˜ ↕10 ⋄ a ← (p.Add 10).Cast"i16:c8" ⋄ f←@•FFI"*:i8"‿"memcpy"‿ "&i16:c8"‿"*i16:c8"‿"usize" ⋄ ! a ≡ 1⊑F ⟨a, p.Cast"", 8⟩ ⋄ p.ReadList 20}} %% ∾⟨↕10, ↕4, 6⥊0⟩
%USE Walloc {{𝕊al: p ← "i16" Al 99 ⋄ p.Write¨˜ ↕10 ⋄ a ← (p.Add 10).Cast"i16:c8" ⋄ f←@•FFI"*i16"‿"memcpy"‿"&·i16:c8"‿"*i16:c8"‿"usize" ⋄        (F ⟨a, p.Cast"", 8⟩).ReadList 4}} %% ↕4
%USE Walloc {{𝕊al: p ← "i16" Al 99 ⋄ p.Write¨˜ ↕10 ⋄ Cpy ⟨(p.Add 10).Cast"i16:c8", p.Cast"", 8⟩ ⋄ p.ReadList 20}} %% ∾⟨↕10, ↕4, 6⥊0⟩
%USE Walloc {{𝕊al: p ← "i16" Al 99 ⋄ p.Write¨˜ ↕10 ⋄ a ← (p.Add 10).Cast"i16:c8" ⋄ f←@•FFI"*:i8"‿"memcpy"‿"&i16"   ‿"*:i8"   ‿"usize" ⋄ ! a ≡ 1⊑F ⟨a, p.Cast"", 8⟩ ⋄ p.ReadList 20}} %% ∾⟨↕10, ↕4, 6⥊0⟩

# •foreign things
•foreign.Sizeof¨ "bool"‿"i8"‿"u8"‿"i16"‿"u32"‿"i64" %% 1‿1‿1‿2‿4‿8
•foreign.Sizeof¨ "[1000000]i16"‿"[1000][100]i64" %% 2000000‿800000
!"FFI type: Array size too large" % •foreign.Sizeof "[1000000000][1000000000][1000000000]i8"
(! 1=•Type)¨ •foreign.Sizeof¨ "{{{i8}}}"‿"{i32,i8,*i8,[4]i32}"
