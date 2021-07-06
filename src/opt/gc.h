#pragma once

extern u64 gc_depth;
static void gc_disable() { gc_depth++; }
static void gc_enable() { gc_depth--; }
void gc_addFn(vfn f);
void gc_add(B x);


extern u8 gc_tagCurr; // if no gc is running, this is what all objects will have
extern u8 gc_tagNew;
#ifdef LOG_GC
extern u64 gc_visitBytes, gc_visitCount, gc_freedBytes, gc_freedCount;
#endif

static void mm_visit(B x) {
  #ifdef HEAP_VERIFY
    if(heapVerify_visit(x)) return;
  #endif
  
  if (!isVal(x)) return;
  Value* vx = v(x);
  u8 p = vx->mmInfo;
  if ((p&0x80)==gc_tagNew) return;
  vx->mmInfo = p^0x80;
  #ifdef LOG_GC
    gc_visitBytes+= mm_size(vx); gc_visitCount++;
  #endif
  TI(x,visit)(vx);
}
static void mm_visitP(void* xp) {
  #ifdef HEAP_VERIFY
    if(heapVerify_visitP(xp)) return;
  #endif
  
  Value* x = (Value*)xp;
  u8 p = x->mmInfo;
  if ((p&0x80)==gc_tagNew) return;
  x->mmInfo = p^0x80;
  #ifdef LOG_GC
    gc_visitBytes+= mm_size(x); gc_visitCount++;
  #endif
  TIv(x,visit)(x);
}
