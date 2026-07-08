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
  re_compFn, re_rt, re_glyphs, re_sysNames, re_sysVals, re_remap, // compiling info
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
B bqn_explain(B code, B vars); // consumes code,vars
HArr* defaultUnknownState();
B bqn_exec(B code, HArr* state); // consumes code,state
B bqn_execFile(B path, B args); // consumes both
B bqn_execFileRe(B path, B args, B re); // consumes path,args
Block* bqn_comp   (B source, HArr* state, B re, Scope* sc, u8 kind, bool loose, bool noNS); // consumes source,state; noNS: fail to compile if result would be a namespace
B      rebqn_exec (B source, HArr* state, B re); // consumes source,state; runs in a new environment
B      rerepl_exec(B source, B state0, B re); // consumes source,state0; uses re_mode and re_scope
NOINLINE HArr* m_state(B path, B name, B args);
HArr* prep_state(B w, B path0, char* name); // check & expand state; consumes w, returns ⟨path,name,args⟩
void init_comp(B* new_re, B* prev_re, B prim, B sys); // doesn't consume; writes re_* compiling info into new_re

void cbqn_init(void);
NORETURN void bqn_exit(i32 code);
