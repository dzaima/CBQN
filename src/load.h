#pragma once
extern HArr* comps_curr; // global-ish state, valid during compilation; comps_max elements
enum {
  comps_path,
  comps_args,
  comps_src,
  comps_re, // HArr of ⟨REPL mode ⋄ scope ⋄ compiler ⋄ runtime ⋄ glyphs ⋄ sysval names ⋄ sysval values⟩
  comps_envPos,
  comps_max
};
#define COMPS_REF(O,N) O->a[comps_##N]
#define COMPS_CREF(N) COMPS_REF(comps_curr, N)

extern B def_sysNames, def_sysVals;
B comps_getPrimitives(void);
void comps_getSysvals(B* res);
