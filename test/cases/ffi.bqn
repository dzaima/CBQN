%DEF var V←•internal.Variation ⋄ LV←•internal.ListVariations ⋄ CLR←•internal.ClearRefs
%DEF tvar %USE var ⋄ _tvar ← {F _𝕣 x: (CLR@) ⊢ {F 𝕩 V x}¨ LV 𝕩; w F _𝕣 x: (CLR@) ⊢ (LV 𝕨) {(𝕨 V w) F 𝕩 V x}⌜ LV 𝕩}
%DEF defs size_t ← "u64" ⋄ size_tw ← 64


# bad •FFI invocation
# generally weird
!"•FFI: 𝕩 must be a list" % @•FFI @
!"•FFI: 𝕩 must be a list" % @•FFI 2‿2⥊<"a"
!"•FFI: Path must be a list of characters" % 1‿2 •FFI ""‿"bqn_init"
!"Fn𝕩: This function can't be called monadically" % •FFI ""‿"bqn_init"
@ •FFI⎊{𝕊: 28↑•CurrentError@} ""‿"bqn_this symbol doesn't exist" %% "FFI: Failed to find symbol: "
!"FFI𝕩: Type must be a string" % @•FFI "hello"
!"FFI: Too many arguments" % @•FFI""‿"bqn_init"∾70000⥊<"i32"
!"Type parser: Unexpected character '?'" % @•FFI""‿"bqn_init"‿"?"
!"Type parser: expected number" % @•FFI""‿"bqn_init"‿"["
!"Type parser: Bad array type" % @•FFI""‿"bqn_init"‿"[0"
!"Type parser: Unexpected end of input" % @•FFI""‿"bqn_init"‿"[1]"
!"Type parser: expected number" % @•FFI""‿"bqn_init"‿"[]i32"
!"Type parser: Unexpected end of input" % @•FFI""‿"bqn_init"‿"{"
!"Type parser: expected number" % @•FFI""‿"bqn_init"‿"i"
!"Type parser: Bad float width" % @•FFI""‿"bqn_init"‿"f80"
!"Type parser: Bad float width" % @•FFI""‿"bqn_init"‿"f128"
!"Type parser: Bad float width" % @•FFI""‿"bqn_init"‿"f16"
!"Type parser: Bad float width" % @•FFI""‿"bqn_init"‿"f1"
!"Type parser: Bad integer width" % @•FFI""‿"bqn_init"‿"u1"
!"Type parser: Bad integer width" % @•FFI""‿"bqn_init"‿"i0"
!"Type parser: Bad integer width" % @•FFI""‿"bqn_init"‿"i10"
!"Type parser: Bad integer width" % @•FFI""‿"bqn_init"‿"u128"
!"Type parser: Type was empty" % @•FFI""‿"bqn_init"‿""
!"Type parser: number literal too large" % @•FFI""‿"bqn_init"‿"u99999999999999"

# >/𝕨/𝕩
!"FFI: Multiple occurrences of '>' within one argument" % @•FFI""‿"bqn_init"‿">>i32"
!"FFI: Multiple occurrences of argument side specified" % @•FFI""‿"bqn_init"‿"𝕨𝕨i32"
!"FFI: Multiple occurrences of argument side specified" % @•FFI""‿"bqn_init"‿"𝕩𝕩i32"
!"FFI: Multiple occurrences of argument side specified" % @•FFI""‿"bqn_init"‿"𝕨𝕩i32"
!"FFI: Multiple arguments on 𝕩 specified, some with '>'" % @•FFI""‿"bqn_init"‿">i32"‿">i32"
!"FFI: Multiple arguments on 𝕨 specified, some with '>'" % @•FFI""‿"bqn_init"‿">𝕨i32"‿">𝕨i32"
!"FFI: Multiple arguments on 𝕩 specified, some with '>'" % @•FFI""‿"bqn_init"‿">i32"‿"i32"
!"FFI: Multiple arguments on 𝕨 specified, some with '>'" % @•FFI""‿"bqn_init"‿">𝕨i32"‿"𝕨i32"

# arrays
!"Type parser: 0-item arrays not supported" % @•FFI""‿"bqn_init"‿"[0]i32"
!"Type parser: number literal too large" % @•FFI""‿"bqn_init"‿"[999999999999]i32"
!"FFI: Cannot return array" % @•FFI"[4]i32"‿"bqn_init"

# structs
!"Type parser: Improper struct separator or end" % @•FFI""‿"bqn_init"‿"{*i32*i32}"
!"Type parser: Improper struct separator or end" % @•FFI""‿"bqn_init"‿"{*i32"
!"Type parser: Unexpected end of input" % @•FFI""‿"bqn_init"‿"{*i32,"
!"Type parser: '&' currently can only be used as the outermost type" % @•FFI""‿"bqn_init"‿"{&i32}"
!"Type parser: '&' currently can only be used as the outermost type" % @•FFI""‿"bqn_init"‿"{[2]&i32}"

# :
!"Type parser: expected number" % @•FFI""‿"bqn_init"‿"i32:"
!"Type parser: Unexpected character after "":""" % f←@•FFI""‿"bqn_init"‿">*u64:q8"
!"Type parser: expected number" % @•FFI""‿"bqn_init"‿"i32:i"
!"Type parser: Unsupported width in :i9" % @•FFI""‿"bqn_init"‿"i32:i9"
!"Type parser: Unsupported width in :u16" % @•FFI""‿"bqn_init"‿"i32:u16"
!"Type parser: Unsupported width in :i64" % @•FFI"i32:i64"‿"bqn_init"
!"Type parser: Unsupported width in :f32" % @•FFI""‿"bqn_init"‿"i32:f32"
!"Type parser: number literal too large" % @•FFI""‿"bqn_init"‿"i32:u9999999999999999"
!"Type parser: Representation wider than the value within" % @•FFI""‿"bqn_init"‿"i32:f64"
!"Type parser: Garbage at end of type" % @•FFI""‿"bqn_init"‿"i32 hello world"

# pointers
!"Type parser: Unexpected end of input" % @•FFI""‿"bqn_init"‿"&"
!"Type parser: '&' currently can only be used as the outermost type" % @•FFI""‿"bqn_init"‿"{&}"
!"Type parser: Unexpected character ':'" % @•FFI""‿"bqn_init"‿"&:i8"

# return value
!"FFI: Function specification must have at least two items" % @•FFI ⟨"&"⟩
!"FFI: Return value is specified as ""&"", but there are 0 mutated values" % @•FFI"&"‿"bqn_init"‿">*i32"
!"FFI: Return value is specified as ""&"", but there are 2 mutated values" % @•FFI"&"‿"bqn_init"‿"&i32"‿"&i32"
!"FFI: Cannot return array" % @•FFI"[4]i32"‿"bqn_init"
!"FFI: Unimplemented result type" % @•FFI"**:i8"‿"bqn_init"
!"FFI: Result type cannot be mutated" % @•FFI"&i32"‿"bqn_init"


# correct elements
%USE defs ⋄ f←@•FFI"&"‿"memcpy"‿"&u8"‿"*i8"‿size_t ⋄ G←{F⟨𝕩,↕0,0⟩} ⋄ %USE tvar ⋄ !∘≡¨⟜⊏ G _tvar 100⥊1‿0
%USE defs ⋄ f←@•FFI"&"‿"memcpy"‿"&u8"‿"*i8"‿size_t ⋄ G←{F⟨𝕩,↕0,0⟩} ⋄ G ↕200 %% ↕200
%USE defs ⋄ f←@•FFI"&"‿"memcpy"‿"&u16"‿"*i8"‿size_t ⋄ G←{F⟨𝕩,↕0,0⟩} ⋄ G ⋈65535 %% ⋈65535
%USE defs ⋄ f←@•FFI"&"‿"memcpy"‿"&u32"‿"*i8"‿size_t ⋄ G←{F⟨𝕩,↕0,0⟩} ⋄ G (2⋆32)|¯5+↕10 %% (2⋆32)|¯5+↕10
%USE defs ⋄ f←@•FFI"&"‿"memcpy"‿"&i16"‿"*i8"‿size_t ⋄ G←{F⟨𝕩,↕0,0⟩} ⋄ G ¯32768‿32767 %% ¯32768‿32767
%USE defs ⋄ f←@•FFI"&"‿"memcpy"‿"&{i16}"‿"*i8"‿size_t ⋄ G←{F⟨𝕩,↕0,0⟩} ⋄ G ⋈¨¯32768‿32767 %% ⋈¨¯32768‿32767
%USE defs ⋄ f←@•FFI"&"‿"memcpy"‿"&f32"‿"*i8"‿size_t ⋄ G←{F⟨𝕩,↕0,0⟩} ⋄ G ⟨∞,¯∞,∞÷∞⟩∾10÷˜↕10 %% ⟨∞,¯∞,∞÷∞⟩∾0‿0.10000000149011612‿0.20000000298023224‿0.30000001192092896‿0.4000000059604645‿0.5‿0.6000000238418579‿0.699999988079071‿0.800000011920929‿0.8999999761581421
%USE defs ⋄ f←@•FFI"&"‿"memcpy"‿"&i32"‿"*i16:u1"‿size_t ⋄ F ⟨⋈5, 32⥊1, 4⟩ %% ⋈¯1
%USE defs ⋄ f←@•FFI⟨"&","memcpy","&i8","*i8",size_t∾":u1"⟩ ⋄ F ⟨¯100+↕10, ↕10, size_tw↑0‿0‿1⟩ %% (↕4)∾4↓¯100+↕10
%USE defs ⋄ f←@•FFI"&"‿"memcpy"‿"&i16"‿"*i16:i32"‿size_t ⋄ F ⟨¯100+↕10, ⟨123+456×2⋆16, ¯2⟩, 8⟩ %% 123‿456‿¯2‿¯1∾4↓¯100+↕10
%USE defs ⋄ f←@•FFI⟨size_t,"strlen","*i8"⟩ ⋄ ⟨⟩ F ⋈1‿2‿3‿0 %% 3
%USE defs ⋄ f←@•FFI⟨"&","memcpy","&i8","[4]i8",size_t⟩ ⋄ F ⟨10⥊0, 3+↕4, 4⟩ %% 3‿4‿5‿6‿0‿0‿0‿0‿0‿0


# bad array elements
!"Type parser: '&' currently can only be used as the outermost type" % %USE defs ⋄ f←@•FFI"&"‿"memcpy"‿"&{&i16}"‿"*i8"‿size_t ⋄ G←{F⟨𝕩,↕0,0⟩} ⋄ G ⋈∘⋈¨¯32768‿32767

!"FFI: Array provided for &u8 contained 299" % f←@•FFI"&"‿"bqn_init"‿">&u8" ⋄ F ↕300
!"FFI: Array provided for &u8 contained ¯99" % f←@•FFI"&"‿"bqn_init"‿">&u8" ⋄ F -↕100
!"FFI: Array provided for &u16 contained 65536" % f←@•FFI"&"‿"bqn_init"‿">&u16" ⋄ F ⋈65536
!"FFI: Array provided for &u32 contained 4294967296" % f←@•FFI"&"‿"bqn_init"‿">&u32" ⋄ F ⋈2⋆32
!"FFI: Array provided for &u16 contained ¯1" % f←@•FFI"&"‿"bqn_init"‿">&u16" ⋄ F ⋈¯1
!"FFI: Array provided for &u32 contained ¯1" % f←@•FFI"&"‿"bqn_init"‿">&u32" ⋄ F ⋈¯1

# bad scalars
!"FFI: improper value for u8" % f←@•FFI""‿"bqn_init"‿">u8" ⋄ F 256
!"FFI: improper value for u16" % f←@•FFI""‿"bqn_init"‿">u16" ⋄ F ¯1
!"FFI: improper value for u32" % f←@•FFI""‿"bqn_init"‿">u32" ⋄ F ¯1
!"FFI: improper value for u64" % f←@•FFI""‿"bqn_init"‿">u64" ⋄ F ¯1
!"FFI: improper value for u32" % f←@•FFI""‿"bqn_init"‿">u32" ⋄ F 2⋆32
!"FFI: improper value for u32" % f←@•FFI""‿"bqn_init"‿">u32" ⋄ F 1e20
!"FFI: u64 argument value ≥ 2⋆53" % f←@•FFI""‿"bqn_init"‿">u64" ⋄ F 2⋆53
!"FFI: i64 argument absolute value ≥ 2⋆53" % f←@•FFI""‿"bqn_init"‿">i64" ⋄ F 2⋆53
!"FFI: i64 argument absolute value ≥ 2⋆53" % f←@•FFI""‿"bqn_init"‿">i64" ⋄ F -2⋆53
!"FFI: improper value for i32" % f←@•FFI""‿"bqn_init"‿">𝕨i32"‿">i64" ⋄ @ F 3
!"FFI: improper value for i32" % f←@•FFI""‿"bqn_init"‿">𝕨i32"‿">i64" ⋄ ⟨1⟩ F 3
!"FFI: improper value for i64" % f←@•FFI""‿"bqn_init"‿"i64" ⋄ F ⟨@⟩


# bad overall argument separation
!"FFI: Expected list 𝕩" % f←@•FFI""‿"bqn_init"‿"i32"‿"i32" ⋄ F @
!"FFI: Expected list 𝕩" % f←@•FFI""‿"bqn_init"‿"i32"‿"i32" ⋄ F ≍⟨↕2, ↕2⟩
!"FFI: Expected list 𝕨" % f←@•FFI""‿"bqn_init"‿"𝕨i32"‿"𝕨i32" ⋄ (≍1‿2) F ⟨⟩
!"FFI: Wrong argument count in 𝕩: expected 2, got 3" % f←@•FFI""‿"bqn_init"‿"i32"‿"i32" ⋄ F ⟨↕2, ↕2, ↕2⟩
!"FFI: Wrong argument count in 𝕩: expected 2, got 1" % f←@•FFI""‿"bqn_init"‿"i32"‿"i32" ⋄ F ⟨↕2⟩
!"FFI: Wrong argument count in 𝕨: expected 2, got 3" % f←@•FFI""‿"bqn_init"‿"i32"‿"𝕨i32"‿"𝕨i32" ⋄ ⟨↕2, ↕2, ↕2⟩ F ⟨4⟩
!"FFI: 𝕩 must be an empty list" % f←@•FFI""‿"bqn_init" ⋄ F ⟨1⟩
!"FFI: 𝕩 must be an empty list" % f←@•FFI""‿"bqn_init" ⋄ F @
!"FFI: 𝕩 must be an empty list" % f←@•FFI""‿"bqn_init"‿"𝕨i32"‿"𝕨i32" ⋄ 1‿2 F @
!"FFI: 𝕨 must either be an empty list, or not be present" % f←@•FFI""‿"bqn_init"‿"*i8" ⋄ ⟨1,2⟩ F 1‿2‿3‿0
!"FFI: 𝕨 must either be an empty list, or not be present" % f←@•FFI""‿"bqn_init"‿"*i8" ⋄ @ F 1‿2‿3‿0
!"FFI: 𝕨 must be present" % f←@•FFI""‿"bqn_init"‿"𝕨i32"‿"𝕨i32" ⋄ F ⟨⟩
!"FFI: 𝕨 must be present" % f←@•FFI""‿"bqn_init"‿"𝕨i32"‿"i32" ⋄ F ⟨3⟩
!"FFI: 𝕨 must be present" % f←@•FFI""‿"bqn_init"‿">𝕨i32"‿">i32" ⋄ F 3



# wrong argument internal structure
!"FFI: Expected array or pointer object corresponding to ""*[2]i32""" % f←@•FFI""‿"bqn_init"‿">*[2]i32" ⋄ F @
!"FFI: Expected array or pointer object corresponding to ""*{...}""" % f←@•FFI""‿"bqn_init"‿">*{i32}" ⋄ F @
!"FFI: Expected array corresponding to ""{...}""" % f←@•FFI""‿"bqn_init"‿">{i32}" ⋄ F 0
!"FFI: Expected array corresponding to ""{...}""" % f←@•FFI""‿"bqn_init"‿">*{i32}" ⋄ F ⟨0⟩
!"FFI: Expected array corresponding to ""[1]i32""" % f←@•FFI""‿"bqn_init"‿">*[1]i32" ⋄ F ⟨0⟩
!"FFI: Incorrect list length corresponding to a struct: expected 1, got 0" % f←@•FFI""‿"bqn_init"‿">*{i32}" ⋄ F ⟨⟨⟩⟩
!"FFI: Incorrect list length corresponding to a struct: expected 1, got 2" % f←@•FFI""‿"bqn_init"‿">*{i32}" ⋄ F ⟨1‿2⟩
!"FFI: Incorrect list length corresponding to an array: expected 2, got 0" % f←@•FFI""‿"bqn_init"‿">*[2]i32" ⋄ F ⟨⟨⟩⟩
!"FFI: Incorrect list length corresponding to an array: expected 2, got 3" % f←@•FFI""‿"bqn_init"‿">*[2]i32" ⋄ F ⟨1‿2‿3⟩

# bad :
!"FFI: Expected array corresponding to ""i32:i8""" % f←@•FFI""‿"bqn_init"‿">i32:i8" ⋄ F @
!"FFI: Expected array or pointer object corresponding to ""*i32:i8""" % f←@•FFI""‿"bqn_init"‿">*i32:i8" ⋄ F @

!"FFI: Bad array corresponding to ""i32:i8"": expected 4 elements, got 10" % f←@•FFI""‿"bqn_init"‿">i32:i8" ⋄ F ↕10
!"FFI: Bad array corresponding to ""u64:u1"": expected 64 elements, got 128" % f←@•FFI""‿"bqn_init"‿">u64:u1" ⋄ F 128⥊1

!"FFI: Array provided for :u1 contained 63" % f←@•FFI""‿"bqn_init"‿">u64:u1" ⋄ F ↕64
!"FFI: Bad array corresponding to ""u64:i8"": expected 8 elements, got 64" % f←@•FFI""‿"bqn_init"‿">u64:i8" ⋄ F ↕64

!"FFI: Expected array or pointer object corresponding to ""*u64:i8""" % f←@•FFI""‿"bqn_init"‿">*u64:i8" ⋄ F @
!"FFI: Array provided for :i8 contained 199" % f←@•FFI""‿"bqn_init"‿">*u64:i8" ⋄ F ↕200
!"FFI: Array provided for :i8 contained 199" % f←@•FFI""‿"bqn_init"‿">&u64:i8" ⋄ F ↕200
!"FFI: Array provided for :u1 contained 63" % f←@•FFI""‿"bqn_init"‿">&u64:u1" ⋄ F ↕64
!"FFI: Bad array corresponding to ""*u64:i8"": expected a multiple of 8 elements, got 2" % f←@•FFI""‿"bqn_init"‿">*u64:i8" ⋄ F ↕2
!"FFI: Bad array corresponding to ""*u32:i16"": expected a multiple of 2 elements, got 1" % f←@•FFI""‿"bqn_init"‿">*u32:i16" ⋄ F ↕1
!"FFI: Bad array corresponding to ""&u64:u1"": expected a multiple of 64 elements, got 32" % f←@•FFI""‿"bqn_init"‿">&u64:u1" ⋄ F 32⥊0

!"FFI: Array provided for &i16 contained 32768" % f←@•FFI"&"‿"bqn_init"‿">&i16" ⋄ F ¯32768‿32768
!"FFI: Array provided for &i16 contained ¯32769" % f←@•FFI"&"‿"bqn_init"‿">&i16" ⋄ F ¯32768‿¯32769

!"FFI: Array provided for &u8 contained array" % f←@•FFI"&"‿"bqn_init"‿">&u8" ⋄ F ⟨↕300⟩
!"FFI: Array provided for &i16 contained character" % f←@•FFI"&"‿"bqn_init"‿">&i16" ⋄ F "hi"
!"FFI: Array provided for &f64 contained namespace" % f←@•FFI"&"‿"bqn_init"‿">&f64" ⋄ F 1‿2‿{⇐}

!"FFI: Array provided for :c8 contained array" % f←@•FFI""‿"bqn_init"‿">*i8:c8" ⋄ F ⋈"hello"
!"FFI: Expected array or pointer object corresponding to ""*i8:c8""" % f←@•FFI""‿"bqn_init"‿">*i8:c8" ⋄ F 4
!"FFI: Array provided for :c8 contained number" % f←@•FFI""‿"bqn_init"‿">*i8:c8" ⋄ F ⋈4
!"FFI: Array provided for :c8 contained number" % f←@•FFI""‿"bqn_init"‿">*i8:c8" ⋄ F "Ah"•internal.Variation⋈4

!"FFI: improper value for i8" %                                    f←@•FFI""‿"bqn_init"‿"*i8:c8"  ‿"i8" ⋄ F "hello"‿1000
!"FFI: Array provided for :c8 contained object of unknown type" %  f←@•FFI""‿"bqn_init"‿"*i8:c8"  ‿"i8" ⋄ F "hello ⍉"‿1000
!"FFI: Array provided for :c8 contained object of unknown type" %  f←@•FFI""‿"bqn_init"‿"*i8:c8"  ‿"i8" ⋄ F "hello 𝕩"‿1000
!"FFI: improper value for i8" %                                    f←@•FFI""‿"bqn_init"‿"*i16:c16"‿"i8" ⋄ F "hello"‿1000
!"FFI: improper value for i8" %                                    f←@•FFI""‿"bqn_init"‿"*i16:c16"‿"i8" ⋄ F "hello ⍉"‿1000
!"FFI: Array provided for :c16 contained object of unknown type" % f←@•FFI""‿"bqn_init"‿"*i16:c16"‿"i8" ⋄ F "hello 𝕩"‿1000
!"FFI: improper value for i8" %                                    f←@•FFI""‿"bqn_init"‿"*i32:c32"‿"i8" ⋄ F "hello"‿1000
!"FFI: improper value for i8" %                                    f←@•FFI""‿"bqn_init"‿"*i32:c32"‿"i8" ⋄ F "hello ⍉"‿1000
!"FFI: improper value for i8" %                                    f←@•FFI""‿"bqn_init"‿"*i32:c32"‿"i8" ⋄ F "hello 𝕩"‿1000

%USE defs ⋄ f←@•FFI"*"‿"memcpy"‿"&i8"‿"*i8:c8"‿size_t ⋄ 1⊑F ⟨↕5, "Ah"•internal.Variation"hello", 5⟩ %% 104‿101‿108‿108‿111



# unimplemented stuff
!"FFI: Unimplemented pointer element type within ""&i64""" % f←@•FFI""‿"bqn_init"‿">&i64" ⋄ F ↕10
!"FFI: Unimplemented pointer element type within ""*i64""" % f←@•FFI""‿"bqn_init"‿">*i64" ⋄ F ↕10
!"FFI: Unimplemented pointer element type within ""&u64""" % f←@•FFI""‿"bqn_init"‿">&u64" ⋄ F ↕10
!"FFI: Pointer element type not implemented" % f←@•FFI""‿"bqn_init"‿">**u64" ⋄ F ⟨↕2⟩



# pointer objects
%DEF mm %USE defs ⋄ Calloc ← {1‿𝕩 {𝕏𝕨} @•FFI⟨"*"∾𝕨, "calloc", size_t, size_t⟩} ⋄ free ← @•FFI""‿"free"‿">*" ⋄ cpy ← @•FFI"*:i8"‿"memcpy"‿"*"‿"*"‿size_t ⋄ CpyA ← +´(2⋆8×↕8)×256|Cpy
%DEF WallocE %USE mm ⋄ {allocs←⟨⟩ ⋄ Clean←{Free¨allocs⋄𝕩} ⋄ Clean 𝕏⎊{𝕊: !•CurrentError Clean @} {{r ⊣ allocs∾↩ r← 𝕨 Calloc 𝕩}}}
%DEF Walloc  %USE mm ⋄ {allocs←⟨⟩ ⋄ Clean←{Free¨allocs⋄𝕩} ⋄ Clean 𝕏                             {{r ⊣ allocs∾↩ r← 𝕨 Calloc 𝕩}}}

# .Field
%USE Walloc {{𝕊al: 1∘("{i32,i32,i32}" Al 99).Field⎊0¨ ¯2+↕8}} %% 0‿0‿1‿1‿1‿0‿0‿0
%USE Walloc {{𝕊al: 1∘("[5]u8"         Al 99).Field⎊0¨ ¯2+↕9}} %% 0‿0‿1‿1‿1‿1‿1‿0‿0
%USE Walloc {{𝕊al: 1∘("[5]u8"         Al 99).Field⎊0¨ ⟨∞,¯∞,0÷0,@,"foo",{⇐}⟩}} %% 6⥊0

# .Write, .Read
%USE Walloc {{𝕊al: p←"{i16,i16,i16,i64}" Al 99 ⋄ p.Write ¯4‿10000‿¯30000‿7e12 ⋄ s←-○{CpyA 0∾˜⋈˜𝕩}´ ⟨p.Field 2, p⟩ ⋄ Cpy ⟨p.Add 1, p, s⟩ ⋄ p.Read¨ ↕2}} %% ⟨¯4‿10000‿¯30000‿7e12, ¯4‿10000‿0‿0⟩
%USE Walloc {{𝕊al: p←"{i32,{i64,i64}}" Al 999 ⋄ {𝕩 p.Write ⟨𝕩×10,𝕩×20‿30⟩}¨ ↕4 ⋄ ((p.Field 1).Field 0).Read¨ ↕4 }} %% 20×↕4
!"Cannot get a field of a pointer to a scalar" % %USE WallocE {{𝕊al: ("i32" Al 99).Field 0}}
!"ptr.Write: Cannot write array to ""*i32""" % %USE WallocE {{𝕊al: ("*i32" Al 99).Write 1‿2‿3}}
!"ptr.Write: Cannot write array to ""*i32:i32""" % %USE WallocE {{𝕊al: ("*i32:i32"Al 99).Write ↕16}}
!"FFI: Cannot read from ""*i32:i32""" % %USE WallocE {{𝕊al: ("*i32:i32"Al 99).Read 0}}
%USE Walloc {{𝕊al: p←("" Al 99).Cast "*:i8" ⋄ a←"i32"Al 99 ⋄ a.Write 12345 ⋄ p.Write ar←Cpy a‿a‿0 ⋄ ! ar ≡ p.Read 0 ⋄ a.Read 0}} %% 12345
%USE Walloc {{𝕊al: p←"i16" Al 99 ⋄ raw ← Cpy 0∾˜⋈˜p.Add 3 ⋄ cpy2←@•FFI"*i16"‿"memcpy"‿"*:i8"‿"*:i8"‿size_t ⋄ p.Sub Cpy2 raw‿raw‿0 }} %% ¯3

# garbage arguments
!"Expected integer, got character" % %USE WallocE {{𝕊al: p←"i8" Al 1 ⋄ p.Add '!'}}
!"(pointer).Sub: Unexpected argument type: character" % %USE WallocE {{𝕊al: p←"i8" Al 1 ⋄ p.Sub '!'}}
!"Expected non-negative integer, got character" % %USE WallocE {{𝕊al: p←"i8" Al 1 ⋄ p.Field '!'}}
!"Type parser𝕩: Pointer type must be a string" % %USE WallocE {{𝕊al: p←"i8" Al 1 ⋄ p.Cast '!'}}
!"Expected integer, got character" % %USE WallocE {{𝕊al: p←"i8" Al 1 ⋄ p.Read '!'}}
!"FFI: improper value for i8" % %USE WallocE {{𝕊al: p←"i8" Al 1 ⋄ p.Write '!'}}

# untyped pointers
!"Cannot get a field of a pointer to a scalar" % %USE WallocE {{𝕊al: ("" Al 99).Field 0}}
# %USE Walloc {{𝕊al: p←"i32" Al 99 ⋄ p.Write 123 ⋄ (((p.Cast "").Add 0).Cast"i32").Read 0}} %% 123
!"Cannot offset an untyped pointer" % %USE WallocE {{𝕊al: ("" Al 99).Add 0}}
!"Cannot offset an untyped pointer" % %USE WallocE {{𝕊al: ("" Al 99).Add 1}}

# .Add, .Sub, .Cast
!"Type parser: Unexpected end of input" % %USE WallocE {{𝕊al: p←"{i32,*}" Al 1 ⋄ p.Cast "{"}}
%USE Walloc {{𝕊al: p←"{i32,*}" Al 1 ⋄ p.Sub p.Cast "{i32,*i32}"}} %% 0
%USE Walloc {{𝕊al: p←"{i32,*}" Al 1 ⋄ p.Sub p.Add 123}} %% ¯123
%USE Walloc {{𝕊al: p←"{i32,*}" Al 1 ⋄ p.Sub p.Sub 123}} %% 123

# ptr1.Sub ptr2
!"(pointer).Sub ptr: Both pointers must be typed" % %USE WallocE {{𝕊al: p←"" Al 1 ⋄ p.Sub p}}
!"(pointer).Sub ptr: Both pointers must be typed" % %USE WallocE {{𝕊al: p←"i8" Al 1 ⋄ (p.Cast "").Sub p.Cast "{{i8,i16},i32}"}}
!"(pointer).Sub ptr: Both pointers must be typed" % %USE WallocE {{𝕊al: p←"i8" Al 1 ⋄ (p.Cast "{{i8,i16},i32}").Sub p.Cast ""}}
!"(pointer).Sub ptr: Arguments must have the same stride" % %USE WallocE {{𝕊al: p←"i8" Al 1 ⋄ ((p.Cast "{{i8,i16},i32}").Field 0).Sub p.Cast "{i8,i16}"}}
!"(pointer).Sub ptr: Arguments must have compatible types" % %USE WallocE {{𝕊al: p←"i8" Al 1 ⋄ p.Sub p.Cast "u8"}}
!"(pointer).Sub ptr: Distance between pointers isn't an exact multiple of stride" % %USE WallocE {{𝕊al: p←"i8" Al 1 ⋄ (p.Cast "i32").Sub (p.Add 6).Cast "i32"}}
!"Type parser: '&' currently can only be used as the outermost type" % @•FFI ""‿"bqn_init"‿"{&i8}"

# type comparison
%USE Walloc {{𝕊al: p←"i8" Al 1 ⋄ t←"{{i8,i16,**i8,[12]i8,*:i32,i64:f64,*,*f64},i32,i64:c8}" ⋄ (p.Cast t).Sub p.Cast t}} %% 0 # ⎊-less test of most passing things to run under heapverify
%DEF TyEq {exp𝕊a‿b: %USE Walloc {{𝕊al: p←"i8" Al 1 ⋄ ! (exp⊑99‿0) ≡ ((p.Cast a).Sub)⎊99 p.Cast b}}}
1 %USE TyEq ⟨"{*i16}", "{*i16}"⟩
0 %USE TyEq ⟨"{*i16}", "{*u16}"⟩
1 %USE TyEq ⟨"{*i16}", "{*}"⟩
1 %USE TyEq ⟨"{*}",    "{*i16}"⟩
1 %USE TyEq ⟨"{*:i8}", "{*i16}"⟩
1 %USE TyEq ⟨"{*:i8}", "{*i16:i8}"⟩
1 %USE TyEq ⟨"{*:i8}", "{*i16:i32}"⟩

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
%USE Walloc {{𝕊al: p ← "i16" Al 99 ⋄ p.Write¨˜ ↕10 ⋄ a ← (p.Add 10).Cast"i16:c8" ⋄ f←@•FFI"*:i8"‿"memcpy"‿"&i16:c8"‿"*i16:c8"‿size_t ⋄ ! a ≡ 1⊑F ⟨a, p.Cast"", 8⟩ ⋄ p.Read¨ ↕20}} %% ∾⟨↕10, ↕4, 6⥊0⟩
%USE Walloc {{𝕊al: p ← "i16" Al 99 ⋄ p.Write¨˜ ↕10 ⋄ Cpy ⟨(p.Add 10).Cast"i16:c8", p.Cast"", 8⟩ ⋄ p.Read¨ ↕20}} %% ∾⟨↕10, ↕4, 6⥊0⟩
%USE Walloc {{𝕊al: p ← "i16" Al 99 ⋄ p.Write¨˜ ↕10 ⋄ a ← (p.Add 10).Cast"i16:c8" ⋄ f←@•FFI"*:i8"‿"memcpy"‿"&i16"   ‿"*:i8"   ‿size_t ⋄ ! a ≡ 1⊑F ⟨a, p.Cast"", 8⟩ ⋄ p.Read¨ ↕20}} %% ∾⟨↕10, ↕4, 6⥊0⟩

# unchecked stuff
# !"FFI: Bad array corresponding to ""i64:c8"": expected 8 elements, got 1" % f←@•FFI""‿"bqn_init"‿">i64:c8" ⋄ F 8⥊2
