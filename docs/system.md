# Supported standard system values

See [the BQN specification](https://mlochbaum.github.io/BQN/spec/system.html) for full descriptions of the following values.

| function      | notes |
|---------------|-------|
| `•BQN`        | |
| `•ReBQN`      | Supported options: `repl`, `primitives` |
| `•primitives` | |
| `•Import`     | |
| `•FFI`        | see [FFI](#ffi) |
| `•state`      | |
| `•args`       | |
| `•path`       | |
| `•name`       | |
| `•wdpath`     | |
| `•Exit`       | |
| `•file`       | Fields: `path`, `At`, `List`, `Bytes`, `Chars`, `Lines`, `Type`, `Exists`, `Name`, `Parent`, `MapBytes`, `CreateDir`, `Rename`, `Remove` |
| `•FChars`     | |
| `•FBytes`     | |
| `•FLines`     | |
| `•Out`        | |
| `•Show`       | |
| `•Repr`       | |
| `•Fmt`        | |
| `•term`       | Fields: `Flush`, `RawMode`, `CharB`, `CharN`; has extensions |
| `•SH`         | Left argument can be `{stdin⇐"input"}` |
| `•Type`       | |
| `•Glyph`      | |
| `•Decompose`  | |
| `•UnixTime`   | second-level precision |
| `•MonoTime`   | up to nanosecond level precision, depending on system support |
| `•Delay`      | |
| `•_timed`     | |
| `•math`       | Fields: `Sin`, `Cos`, `Tan`, `Asin`, `Acos`, `Atan`; `⁼` supported |
| `•MakeRand`   | uses wyhash, **not** cryptographically secure; Result fields: `Range`, `Deal`, `Subset` |
| `•rand`       | seeds with system time (can be hard-coded by setting the C macro `RANDSEED`), same algorithm as `•MakeRand` |
| `•bit`        | Fields: `_cast`; casting an sNaN bit pattern to a float is undefined behavior |

# CBQN-specific system values and extensions

## `•term`

`•term.OutRaw` and `•term.ErrRaw` output the given bytes directly to the specific stream, without any trailing newline. May be removed once a proper interface for stream I/O has been made.

## `•_while_`

While `𝕨𝔾𝕩`, execute `𝕩↩𝕨𝔽𝕩`. Equivalent to `{𝕨𝔾𝕩? 𝕨 𝕊 𝕨𝔽𝕩; 𝕩}`.

## `•GetLine`

Ignores its argument and returns one line of stdin.

Might be removed, moved, or renamed in the future.

## `•Hash`

Get the hash of `𝕩`.

Monadically, use the global secret value which will differ between CBQN sessions.

Dyadically, use the integer left argument as the secret. Will be the same between multiple CBQN sessions, but may change between versions.

## `•PrimInd`

Return the primitive index of the argument.

Might be removed in the future.

## `•Cmp`

Compare arguments. Equal to `((⊑⍋-⍒)⋈)`.

## `•FromUTF8`

Convert the argument UTF-8 byte array (or character array consisting of codepoints 0-255) to a string.

May be removed in the future.

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
| `•internal.HeapDump`       | Create a heap dump file |
| `•internal.Squeeze`        | Try to convert the argument to its most compact representation |
| `•internal.DeepSqueeze`    | Try to convert the argument and all its subarrays to its most compact representation; won't squeeze namespace fields |
| `•internal.ListVariations` | List the possible type variations of the argument array |
| `•internal.Variation`      | Convert `𝕩` to the variation specified in `𝕨` |
| `•internal.ClearRefs`      | Clear references `•internal.Variation` made for `*Inc` variations |
| `•internal.Unshare`        | Get a unique, reference count 1 version of the argument; recursively unshares array items, doesn't touch namespaces |
| `•internal.EEqual`         | exactly equal (NaN equals NaN); 0 and ¯0 aren't equal, but can be made so with the C compile-time flag `-DEEQUAL_NEGZERO` |
| `•internal.Temp`           | place to test new features or temporarily expose some internal function |

# FFI

Currently, there is no support for structs, nested pointers, or constant-length arrays, aka the only supported types are scalars (e.g. `i8`, `u64`, `*`), pointers to those (e.g. `*i8`, `&u64`), and conversions of either of those (e.g. `u64:i32`, `**:c8`).

Additionally, the `a` type maps to `BQNV` from [bqnffi.h](../include/bqnffi.h) (example usage in [FFI tests](../test/ffi/)).