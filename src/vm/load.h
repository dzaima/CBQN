#pragma once
extern GLOBAL HArr* comps_curr; // global-ish state, valid during compilation; comps_max elements
enum {
  comps_path,
  comps_name,
  comps_args,
  comps_kind, // vm.h COMP_*
  comps_src,
  comps_re, // HArr of re_* values (re_mode & re_scope have unspecified values)
  comps_envPos,
  comps_max
};
enum {
  re_compFn, re_rt, re_glyphs, re_sysNames, re_sysVals, // compiling info
  re_map, // •HashMap of cached import results
  re_mode, re_scope, // only for rerepl_exec
  re_max
};
#define COMPS_REF(O,N) O->a[comps_##N]
#define COMPS_CREF(N) COMPS_REF(comps_curr, N)
#define COMPS_ACTIVE() (comps_curr!=NULL)

extern GLOBAL B def_sysNames, def_sysVals, def_re;
B comps_getPrimitives(void);
void comps_getSysvals(B* res);

typedef struct Block Block;
typedef struct Scope Scope;
NOINLINE B load_fullpath(B path, B name); // doesn't consume
B bqn_explain(B str, B vars); // consumes str & vars
B bqn_execFile(B path, B args); // consumes both
B bqn_execFileRe(B path, B args, B re); // consumes path,args
Block* bqn_comp   (B str, B state, B re, Scope* sc, u8 kind, bool loose, bool noNS); // consumes str,state; noNS: fail to compile if result would be a namespace
B      rebqn_exec (B str, B state, B re); // consumes str,state; runs in a new environment
B      rerepl_exec(B str, B state, B re); // consumes str,state; uses re_mode and re_scope
void init_comp(B* new_re, B* prev_re, B prim, B sys); // doesn't consume; writes re_* compiling info into new_re
