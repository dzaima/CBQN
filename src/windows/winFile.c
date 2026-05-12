#include "core.h"
#include "utils/cstr.h"
#include <direct.h>
#include <pathcch.h>
#include "windows/utf16.h"

typedef WCHAR* OsStr;
#define OS_C(C) L##C
#define toOsStr     toWStr
#define freeOsStr   freeWStr
#define OsStrDecode0 utf16Decode0
#define DIR       _WDIR
#define dirent    _wdirent
#define opendir   _wopendir
#define readdir   _wreaddir
#define closedir  _wclosedir
#define mkdir(P, IGNORE) _wmkdir(P)
#define access _waccess
#define rename _wrename
#define unlink _wunlink

#define PREFERRED_SEP '\\'
static bool isPathSep(uint32_t c) {
  return c=='/' || c=='\\';
}

static WCHAR* realpath(const WCHAR*__restrict path, WCHAR*__restrict resolved_path) {
  return _wfullpath(NULL, path, 0);
}

// ⟨absoluteFollows, prefixEnd↑x⟩ is one of:
// A. 0 ⋄ "", for fully-relative paths (always empty)
// B. 0 ⋄ "C:", for known-drive-relative paths (always 2 chars)
// C. 1 ⋄ "/", for unknown-drive-absolute paths (always 1 slash char)
// D. 1 ⋄ "C:/" or "//?/foo/bar/" or "//server/share/" etc, for full absolute paths (or without the ending slash if there was none, or with missing components for "//…" if there were none; but always ≥2 chars)
typedef struct WinPathInfo {
  ux prefixEnd;
  bool absoluteFollows;
} WinPathInfo;
static NOINLINE WinPathInfo winPathInfo(B x) {
  ux ia = IA(x);
  if (ia == 0) return (WinPathInfo){0, false};
  SGetU(x);
  u32 c0 = o2cG(GetU(x,0));
  bool c0slash = isPathSep(c0);
  if (ia == 1) return (WinPathInfo){c0slash, c0slash};
  u32 c1 = o2cG(GetU(x,1));
  if (c0slash) { // "/·…"
    if (isPathSep(c1)) { // "//…" - UNC absolute path
      ux i = 2;
      ux n = 0;
      while (i < ia && n < 2) {
        if (isPathSep(o2cG(GetU(x,i)))) n++;
        i++;
      }
      return (WinPathInfo){i, true};
    } else { // "/[^/]…" - absolute current-drive-relative path
      return (WinPathInfo){1, true};
    }
  } else if (c1 == ':') { // "·:…" - regular drive
    if (ia >= 3 && isPathSep(o2cG(GetU(x,2)))) return (WinPathInfo){3, true}; // "·:/…" - absolute drive path
    return (WinPathInfo){2, false}; // "·:" or "·:…" - specific-drive-relative path
  } else {
    return (WinPathInfo){0, false};
  }
}

static FILE* file_open_impl(B path, char* desc, char* mode) {
  WCHAR wmode[8] = {0};
  u64 len = strlen(mode);
  assert(len<(sizeof(wmode)/sizeof(WCHAR)));
  for (u64 i = 0; i<len; ++i) wmode[i] = (WCHAR)mode[i];
  WCHAR *p = toWStr(path);
  FILE* f = _wfopen(p, wmode);
  freeWStr(p);
  return f;
}

B path_rel(B base, B rel, char* name) {
  assert(isArr(base) || q_N(base));
  if (!isStr(rel)) thrF("%U: Path must be a list of characters", name);
  
  WinPathInfo rpi = winPathInfo(rel);
  if (!rpi.absoluteFollows && rpi.prefixEnd > 0) thrF("%U: Drive-specific relative paths not supported", name); // rel case B, "C:foo/bar"
  if (rpi.absoluteFollows && rpi.prefixEnd != 1) return rel; // rel case D, "C:/foo/bar" etc
  if (q_N(base)) thrF("%U: Using relative path with no absolute base path known", name);
  ux bia = IA(base);
  if (bia==0) return rel;
  WinPathInfo bpi = winPathInfo(base);
  
  B basePre;
  if (rpi.absoluteFollows) { // rel case C, "/foo/bar"; cares about format of base
    if (bpi.prefixEnd < 2) return rel; // base cases B & D - no drive known, rel is absolute enough
    // else, base cases A & C; need to take the drive prefix
    ux prefix = bpi.prefixEnd;
    if (isPathSep(o2cG(IGetU(base, bpi.prefixEnd-1)))) prefix--; // remove trailing slash, as the rel joining will add one
    incG(base);
    basePre = taga(arr_shVec(TI(base,slice)(base, 0, prefix)));
  } else { // rel case A, "foo/bar"; just need to append to base, potentially with a slash in between
    basePre = incG(base);
    SGetU(base);
    if (!(bia==2 && o2cG(GetU(base,1))==':') && !isPathSep(o2cG(GetU(base, bia-1)))) basePre = vec_addN(basePre, m_c32('\\')); // insert a slash if one is not already there, except not when base is a plain "C:" where adding a slash significantly changes meaning
  }
  return vec_join(basePre, rel);
}
