%DEF var V←•internal.Variation ⋄ LV←•internal.ListVariations ⋄ CLR←•internal.ClearRefs
%DEF tvar %USE var ⋄ _tvar ← {F _𝕣 x: (CLR@) ⊢ {F 𝕩 V x}¨ LV 𝕩; w F _𝕣 x: (CLR@) ⊢ (LV 𝕨) {(𝕨 V w) F 𝕩 V x}⌜ LV 𝕩}
%DEF defs size_t ← "u64"


# bad •FFI invocation
# generally weird
@ •FFI⎊9 ""‿"bqn_this symbol doesn't exist" %% 9
!"FFI: Type must be a string" % @•FFI "hello"
!"FFI: Error parsing type: Unexpected character '?'" % @•FFI""‿"bqn_init"‿"?"
!"FFI: Bad array type" % @•FFI""‿"bqn_init"‿"["
!"FFI: Bad array type" % @•FFI""‿"bqn_init"‿"[0"
!"FFI: Error parsing type: Unexpected end of input" % @•FFI""‿"bqn_init"‿"[1]"
!"FFI: Error parsing type: Unexpected end of input" % @•FFI""‿"bqn_init"‿"{"
!"FFI: Bad integer width" % @•FFI""‿"bqn_init"‿"i"
!"FFI: Bad float width" % @•FFI""‿"bqn_init"‿"f80"
!"FFI: Bad float width" % @•FFI""‿"bqn_init"‿"f128"
!"FFI: Bad float width" % @•FFI""‿"bqn_init"‿"f16"
!"FFI: Bad float width" % @•FFI""‿"bqn_init"‿"f1"
!"FFI: Bad integer width" % @•FFI""‿"bqn_init"‿"u1"
!"FFI: Bad integer width" % @•FFI""‿"bqn_init"‿"i0"
!"FFI: Bad integer width" % @•FFI""‿"bqn_init"‿"i10"
!"FFI: Bad integer width" % @•FFI""‿"bqn_init"‿"u128"
!"FFI: number literal too large" % @•FFI""‿"bqn_init"‿"u99999999999999"
!"FFI: Too many arguments" % @•FFI""‿"bqn_init"∾70000⥊<"i32"

# >/𝕨/𝕩
!"FFI: At least one argument should be in 𝕩" % @•FFI""‿"bqn_init"‿"𝕨i32"‿"𝕨i32"
!"FFI: Multiple occurrences of '>' within one argument" % @•FFI""‿"bqn_init"‿">>i32"
!"FFI: Multiple occurrences of argument side specified" % @•FFI""‿"bqn_init"‿"𝕨𝕨i32"
!"FFI: Multiple occurrences of argument side specified" % @•FFI""‿"bqn_init"‿"𝕩𝕩i32"
!"FFI: Multiple occurrences of argument side specified" % @•FFI""‿"bqn_init"‿"𝕨𝕩i32"
!"FFI: Multiple arguments on 𝕩 specified, some with '>'" % @•FFI""‿"bqn_init"‿">i32"‿">i32"
!"FFI: Multiple arguments on 𝕨 specified, some with '>'" % @•FFI""‿"bqn_init"‿">𝕨i32"‿">𝕨i32"
!"FFI: Multiple arguments on 𝕩 specified, some with '>'" % @•FFI""‿"bqn_init"‿">i32"‿"i32"
!"FFI: Multiple arguments on 𝕨 specified, some with '>'" % @•FFI""‿"bqn_init"‿">𝕨i32"‿"𝕨i32"

# arrays
!"FFI: 0-item arrays not supported" % @•FFI""‿"bqn_init"‿"[0]i32"
!"FFI: number literal too large" % @•FFI""‿"bqn_init"‿"[999999999999]i32"
!"FFI: Top-level array too large; limit is 65535 elements" % @•FFI""‿"bqn_init"‿"[65536]i32"
!"FFI: Top-level array too large; limit is 65535 elements" % @•FFI""‿"bqn_init"‿"[4000000000]i32"
!"FFI: Array too large; limit is 65534 bytes" % @•FFI""‿"bqn_init"‿"*[20000]i32"
!"FFI: Cannot return array" % @•FFI"[4]i32"‿"bqn_init"

# structs
!"FFI: Improper struct separator or end" % @•FFI""‿"bqn_init"‿"{*i32*i32}"
!"FFI: Improper struct separator or end" % @•FFI""‿"bqn_init"‿"{*i32"
!"FFI: Error parsing type: Unexpected end of input" % @•FFI""‿"bqn_init"‿"{*i32,"
!"FFI: Structs currently cannot contain mutable references" % @•FFI""‿"bqn_init"‿"{&i32}"

# :
!"FFI: Garbage at end of type" % @•FFI""‿"bqn_init"‿"i32:"
!"Unsupported width in :i0" % @•FFI""‿"bqn_init"‿"i32:i"
!"Unsupported width in :i9" % @•FFI""‿"bqn_init"‿"i32:i9"
!"Unsupported width in :u16" % @•FFI""‿"bqn_init"‿"i32:u16"
!"Unsupported width in :i64" % @•FFI"i32:i64"‿"bqn_init"
!"Unsupported width in :f32" % @•FFI""‿"bqn_init"‿"i32:f32"
!"FFI: number literal too large" % @•FFI""‿"bqn_init"‿"i32:u9999999999999999"
!"FFI: Representation wider than the value for ""i32:f64""" % @•FFI""‿"bqn_init"‿"i32:f64"
!"FFI: Garbage at end of type" % @•FFI""‿"bqn_init"‿"i32 hello world"

# return value
!"FFI: Function specification must have at least two items" % @•FFI ⟨"&"⟩
!"FFI: Return value is specified as ""&"", but there are 0 mutated values" % @•FFI"&"‿"bqn_init"‿">*i32"
!"FFI: Return value is specified as ""&"", but there are 2 mutated values" % @•FFI"&"‿"bqn_init"‿"&i32"‿"&i32"
!"FFI: Cannot return array" % @•FFI"[4]i32"‿"bqn_init"
!"FFI: Unimplemented result type" % @•FFI"*i32"‿"bqn_init"
!"FFI: Unimplemented result type" % @•FFI"&i32"‿"bqn_init"


# correct elements
%USE defs ⋄ f←@•FFI"&"‿"memcpy"‿"&u8"‿"*i8"‿size_t ⋄ G←{F⟨𝕩,↕0,0⟩} ⋄ %USE tvar ⋄ !∘≡¨⟜⊏ G _tvar 100⥊1‿0
%USE defs ⋄ f←@•FFI"&"‿"memcpy"‿"&u8"‿"*i8"‿size_t ⋄ G←{F⟨𝕩,↕0,0⟩} ⋄ G ↕200 %% ↕200
%USE defs ⋄ f←@•FFI"&"‿"memcpy"‿"&u16"‿"*i8"‿size_t ⋄ G←{F⟨𝕩,↕0,0⟩} ⋄ G ⋈65535 %% ⋈65535
%USE defs ⋄ f←@•FFI"&"‿"memcpy"‿"&i16"‿"*i8"‿size_t ⋄ G←{F⟨𝕩,↕0,0⟩} ⋄ G ¯32768‿32767 %% ¯32768‿32767
%USE defs ⋄ f←@•FFI"&"‿"memcpy"‿"&{i16}"‿"*i8"‿size_t ⋄ G←{F⟨𝕩,↕0,0⟩} ⋄ G ⋈¨¯32768‿32767 %% ⋈¨¯32768‿32767



# bad array elements
!"FFI: Structs currently cannot contain mutable references" % %USE defs ⋄ f←@•FFI"&"‿"memcpy"‿"&{&i16}"‿"*i8"‿size_t ⋄ G←{F⟨𝕩,↕0,0⟩} ⋄ G ⋈∘⋈¨¯32768‿32767

!"FFI: Array provided for &u8 contained 299" % f←@•FFI"&"‿"bqn_init"‿">&u8" ⋄ F ↕300
!"FFI: Array provided for &u8 contained ¯99" % f←@•FFI"&"‿"bqn_init"‿">&u8" ⋄ F -↕100
!"FFI: Array provided for &u16 contained 65536" % f←@•FFI"&"‿"bqn_init"‿">&u16" ⋄ F ⋈65536
!"FFI: Array provided for &u32 contained 4294967296" % f←@•FFI"&"‿"bqn_init"‿">&u32" ⋄ F ⋈2⋆32
!"FFI: Array provided for &u16 contained ¯1" % f←@•FFI"&"‿"bqn_init"‿">&u16" ⋄ F ⋈¯1
!"FFI: Array provided for &u32 contained ¯1" % f←@•FFI"&"‿"bqn_init"‿">&u32" ⋄ F ⋈¯1

!"FFI: Array provided for &i16 contained 32768" % f←@•FFI"&"‿"bqn_init"‿">&i16" ⋄ F ¯32768‿32768
!"FFI: Array provided for &i16 contained ¯32769" % f←@•FFI"&"‿"bqn_init"‿">&i16" ⋄ F ¯32768‿¯32769

!"FFI: Array provided for &u8 contained an array" % f←@•FFI"&"‿"bqn_init"‿">&u8" ⋄ F ⟨↕300⟩
!"FFI: Array provided for &i16 contained a character" % f←@•FFI"&"‿"bqn_init"‿">&i16" ⋄ F "hi"
!"FFI: Array provided for &f64 contained a namespace" % f←@•FFI"&"‿"bqn_init"‿">&f64" ⋄ F 1‿2‿{⇐}

# bad scalars
!"FFI: u8 argument not exact" % f←@•FFI""‿"bqn_init"‿">u8" ⋄ F 256
!"FFI: u16 argument not exact" % f←@•FFI""‿"bqn_init"‿">u16" ⋄ F ¯1
!"FFI: u32 argument not exact" % f←@•FFI""‿"bqn_init"‿">u32" ⋄ F ¯1
!"FFI: u64 argument not exact" % f←@•FFI""‿"bqn_init"‿">u64" ⋄ F ¯1
!"FFI: u32 argument not exact" % f←@•FFI""‿"bqn_init"‿">u32" ⋄ F 2⋆32
!"FFI: u32 argument not exact" % f←@•FFI""‿"bqn_init"‿">u32" ⋄ F 1e20
!"FFI: u64 argument value ≥ 2⋆53" % f←@•FFI""‿"bqn_init"‿">u64" ⋄ F 2⋆53
!"FFI: i64 argument absolute value ≥ 2⋆53" % f←@•FFI""‿"bqn_init"‿">i64" ⋄ F 2⋆53
!"FFI: i64 argument absolute value ≥ 2⋆53" % f←@•FFI""‿"bqn_init"‿">i64" ⋄ F -2⋆53



# bad overall argument separation
!"FFI: Expected array 𝕩" % f←@•FFI""‿"bqn_init"‿"i32"‿"i32" ⋄ F @
!"FFI: Unnecessary 𝕨 given" % f←@•FFI""‿"bqn_init"‿"i32"‿"i32" ⋄ @ F 1‿2
!"FFI: Wrong argument count in 𝕩: expected 2, got 3" % f←@•FFI""‿"bqn_init"‿"i32"‿"i32" ⋄ F ⟨↕2, ↕2, ↕2⟩
!"FFI: Wrong argument count in 𝕩: expected 2, got 1" % f←@•FFI""‿"bqn_init"‿"i32"‿"i32" ⋄ F ⟨↕2⟩
!"FFI: Wrong argument count in 𝕨: expected 2, got 3" % f←@•FFI""‿"bqn_init"‿"i32"‿"𝕨i32"‿"𝕨i32" ⋄ ⟨↕2, ↕2, ↕2⟩ F ⟨4⟩



# wrong argument internal structure
!"FFI: Expected array corresponding to ""i32:i8""" % f←@•FFI""‿"bqn_init"‿">i32:i8" ⋄ F @
!"FFI: Bad array corresponding to ""i32:i8"": expected 4 elements, got 10" % f←@•FFI""‿"bqn_init"‿">i32:i8" ⋄ F ↕10

!"FFI: Expected array corresponding to a pointer" % f←@•FFI""‿"bqn_init"‿">*i32:i8" ⋄ F @

!"FFI: Expected array corresponding to *{...}" % f←@•FFI""‿"bqn_init"‿">*{i32}" ⋄ F @
!"FFI: Expected array corresponding to a struct" % f←@•FFI""‿"bqn_init"‿">{i32}" ⋄ F @
!"FFI: Expected array corresponding to a struct" % f←@•FFI""‿"bqn_init"‿">*{i32}" ⋄ F ⟨@⟩
!"FFI: Incorrect list length corresponding to a struct: expected 1, got 0" % f←@•FFI""‿"bqn_init"‿">*{i32}" ⋄ F ⟨⟨⟩⟩
!"FFI: Incorrect list length corresponding to a struct: expected 1, got 2" % f←@•FFI""‿"bqn_init"‿">*{i32}" ⋄ F ⟨1‿2⟩
!"FFI: Incorrect list length corresponding to an array: expected 2, got 0" % f←@•FFI""‿"bqn_init"‿">*[2]i32" ⋄ F ⟨⟨⟩⟩
!"FFI: Incorrect list length corresponding to an array: expected 2, got 3" % f←@•FFI""‿"bqn_init"‿">*[2]i32" ⋄ F ⟨1‿2‿3⟩



# unimplemented stuff
!"FFI: ""*"" unimplemented" % f←@•FFI""‿"bqn_init"‿">*" ⋄ F 1
!"FFI: ""*i64"" argument type not yet implemented" % f←@•FFI""‿"bqn_init"‿">&i64" ⋄ F ↕10
!"FFI: ""*i64"" argument type not yet implemented" % f←@•FFI""‿"bqn_init"‿">*i64" ⋄ F ↕10
!"FFI: ""*u64"" argument type not yet implemented" % f←@•FFI""‿"bqn_init"‿">&u64" ⋄ F ↕10
!"FFI: Pointer element type not implemented" % f←@•FFI""‿"bqn_init"‿">**u64" ⋄ F ⟨↕2⟩


