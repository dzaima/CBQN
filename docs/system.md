# Supported standard system values

See [the BQN specification](https://mlochbaum.github.io/BQN/spec/system.html) for full descriptions of the following values.

| function      | notes |
|---------------|-------|
| `•BQN`        | |
| `•ReBQN`      | Supported options: `repl`; `primitives`; `system` that's not `"safe"` |
| `•primitives` | |
| `•Import`     | |
| `•FFI`        | see [FFI](#ffi) |
| `•state`      | |
| `•args`       | |
| `•path`       | |
| `•name`       | |
| `•wdpath`     | |
| `•Exit`       | |
| `•file`       | Fields: `path`, `At`, `List`, `Bytes`, `Chars`, `Lines`, `Type`, `Exists`, `Name`, `Parent`, `MapBytes`, `CreateDir`, `Rename`, `Remove`, `Created`, `Modified`, `Accessed`, `Size` |
| `•FChars`     | |
| `•FBytes`     | |
| `•FLines`     | |
| `•Out`        | |
| `•Show`       | |
| `•Repr`       | |
| `•ParseFloat` | Should exactly round floats with up to 17 significant digits, but won't necessarily round correctly with more |
| `•Fmt`        | |
| `•term`       | Fields: `Flush`, `RawMode`, `CharB`, `CharN`; has extensions |
| `•SH`         | See [•SH](#sh) |
| `•Type`       | |
| `•Glyph`      | |
| `•Decompose`  | |
| `•UnixTime`   | second-level precision |
| `•MonoTime`   | up to nanosecond level precision, depending on system support |
| `•Delay`      | |
| `•_timed`     | |
| `•math`       | Fields: `Acos`, `Acosh`, `Asin`, `Asinh`, `Atan`, `Atan2`, `Atanh`, `Cbrt`, `Comb`, `Cos`, `Cosh`, `Erf`, `ErfC`, `Expm1`, `Fact`, `GCD`, `Hypot`, `LCM`, `Log10`, `Log1p`, `Log2`, `LogFact`, `Sin`, `Sinh`, `Sum`, `Tan`, `Tanh`; `⁼` supported for trigonometry functions and between `Expm1` & `Log1p` |
| `•MakeRand`   | uses wyhash, **not** cryptographically secure; Result fields: `Range`, `Deal`, `Subset` |
| `•rand`       | seeds with system time (can be hard-coded by setting the C macro `RANDSEED`), same algorithm as `•MakeRand` |
| `•bit`        | Fields: `_cast`; casting an sNaN bit pattern to a float is undefined behavior |

# CBQN-specific system values and extensions

## `•term`

`•term.OutRaw` and `•term.ErrRaw` output the given bytes directly to the specific stream, without any trailing newline. May be removed once a proper interface for stream I/O has been made.

## `•ns`

`ns •ns.Get "abc"`, `ns •ns.Has "abc"`, and `•ns.Keys ns` for reading namespace information. Order of `•ns.Keys` result is undefined; `Get` & `Has` currently expect the key name to be normalized (i.e. lowercase & without underscores).

## `•_while_`

While `𝕨𝔾𝕩`, execute `𝕩↩𝕨𝔽𝕩`. Equivalent to `{𝕨𝔾𝕩? 𝕨 𝕊 𝕨𝔽𝕩; 𝕩}`.

## `•GetLine`

Ignores its argument and returns one line of stdin.

Might be removed, moved, or renamed in the future.

## `•Hash`

Get the hash of `𝕩`.

Monadically, use the global secret value which will differ between CBQN sessions.

Dyadically, use the integer left argument as the seed. Will be the same between multiple CBQN sessions, but may change between CBQN versions.

## `•PrimInd`

Return the primitive index of the argument.

Might be removed in the future.

## `•Cmp`

Compare arguments. Equal to `((⊑⍋-⍒)⋈)`.

## `•FromUTF8`

Convert the argument UTF-8 byte array (or character array consisting of codepoints 0-255) to a string.

May be removed or renamed in the future.

## `•ToUTF8`

Convert the argument character list to a UTF-8-encoded byte array, represented as a list of characters with codepoints 0-255.

May be removed or renamed in the future.

## `•CurrentError`

Get the current error message while within the catch side of `⎊`. Dynamically-scoped.

Might return a more informative object in the future (e.g. whether the error came from an `!`, the compiler or a builtin, maybe stacktrace reading, etc; such a format is TBD).

## `•internal`

Namespace of various internal functions. May change at any time.

| name                       | description |
|----------------------------|-------------|
| `•internal.Type`           | Name of the internal type for an object |
| `•internal.ElType`         | Element size type identifier; see `enum ElType` |
| `•internal.Refc`           | Reference count of the argument, if it's heap-allocated |
| `•internal.IsPure`         | Whether the vm considers the argument pure (i.e. it can execute it safely for computing fills) |
| `•internal.Info`           | General internal info about the object; a left argument of `1` gives more details |
| `•internal.HeapDump`       | Create a heap dump file; saves to `•wdpath`-relative path `𝕩` or `CBQNHeapDump` if `𝕩` isn't an array |
| `•internal.Squeeze`        | Try to convert the argument to its most compact representation |
| `•internal.DeepSqueeze`    | Try to convert the argument and all its subarrays to its most compact representation; won't squeeze namespace fields |
| `•internal.ListVariations` | List the possible type variations of the argument array |
| `•internal.Variation`      | Convert `𝕩` to the variation specified in `𝕨` |
| `•internal.ClearRefs`      | Clear references `•internal.Variation` made for `*Inc` variations |
| `•internal.Unshare`        | Get a unique, reference count 1 version of the argument; recursively unshares array items, doesn't touch namespaces |
| `•internal.EEqual`         | exactly equal (NaN equals NaN); 0 and ¯0 aren't equal, but can be made so with the C compile-time flag `-DEEQUAL_NEGZERO` |
| `•internal.Temp`           | place to test new features or temporarily expose some internal function |

# FFI

Currently, there is no support for nested pointers or constant-length arrays, and limited support of structs.

That is, the supported types are:
- scalars (e.g. `i8`, `u64`);
- pointers to scalars (e.g. `*i8`, `&u64`);
- conversions of either scalars, pointers to scalars, or opaque pointers (e.g. `u64:i32`, `*i64:i32`, `*:i8`, `**:c8`);
- structs of any of the above (except `&`-pointers) or other structs (e.g. `{*i8,*{*u32:i8,u64:i32}}`), except structs that are within `&` themselves cannot contain any pointers other than converted opaque pointers (e.g. `*{*i32,u64}`, `&{*:i32,u64}`, and `&{i32,u64}` are fine, but `&{*i32,u64}` is not);
- the `a` type, which maps to `BQNV` from [bqnffi.h](../include/bqnffi.h) (example usage in [FFI tests](../test/ffi/)).

# •SH

The left argument can be a namespace, providing additional options.

Accepted fields:

- `stdin⇐"abcd"` passes in stdin to the program; by default, it will be UTF-8 encoded.
- `raw⇐1` will make stdout/stderr return raw bytes (represented as characters with codepoints 0 to 255), and, if given, interpret stdin as such too.

```bqn
   ¯1↓1⊑{stdin⇐@+20‿100‿200‿250,raw⇐1} •SH ⟨"xxd"⟩
"00000000: 1464 c8fa                                .d.."
   @-˜ 1⊑{raw⇐1} •SH ⟨"printf", "\x00\x80\xff\xfe\xee"⟩
⟨ 0 128 255 254 238 ⟩
```


These `•SH` extensions may change in the future if a different interface is standardized.