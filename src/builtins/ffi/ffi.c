#include "core.h"

#if FFI && !defined(CBQN_EXPORT)
  #error "Expected CBQN_EXPORT if FFI is defined"
#endif

#include "ffiExport.c"

#if FFI
  // split up into files for some semantic separation, but still all merged together to allow for static functions & inlining
  #include "ffiCore.c"
  #include "ffiMain.c" // memory copying, core ffiFn_core
  #include "ffiParser.c" // type parsing, main ffiload_c2
  #include "ptrobj.c" // pointer objects
  
  static void sysffi_init() {
    // for the combined FFICompoundType + foreign->element allocation
    assert(sizeof(FFICompoundType) % sizeof(ffi_type*) == 0);
    assert(sizeof(FFIEnt) % sizeof(ffi_type*) == 0);
    assert(8 % _Alignof(ffi_type) == 0);
    assert(sty_u16==sty_u8+1 && sty_u32==sty_u16+1); // assumption made by code for *u8 via el_i8 & co
  }
  
  STATIC_GLOBAL bool ffiInit;
  DEFINE_NFN ffiloadDesc, foreignFunctionDesc, foreignValueDesc, foreignPointerDesc;
  STATIC_GLOBAL Body* foreign_nsGen;
  STATIC_GLOBAL B nullPointer;
  static void initSysFFIDesc() {
    if (ffiInit) return;
    ffiInit = true;
    ptrobj_init();
    foreignFnDesc = registerNFn(m_c8vec_0("(foreign function)"), c1_bad, c2_bad);
    TIi(t_ffiType,freeO) = ffiType_freeO; TIi(t_ffiScratchMem,freeO) = ffiScratchMem_freeO;
    TIi(t_ffiType,freeF) = ffiType_freeF; TIi(t_ffiScratchMem,freeF) = ffiScratchMem_freeF;
    TIi(t_ffiType,visit) = ffiType_visit; TIi(t_ffiScratchMem,visit) = ffiScratchMem_visit;
    TIi(t_ffiType,print) = ffiType_print;
    TIi(t_unkArr,visit) = noop_visit;
    TIi(t_unkArr,freeO) = tyarr_freeO;
    TIi(t_unkArr,freeF) = tyarr_freeF;
    if (sizeof(size_t) != sizeof(ssize_t)) fatal("unexpected ssize_t size");
    
    ffiloadDesc = registerNFn(m_c32vec_0(U"•FFI"), c1_bad, ffi_c2);
    foreignFunctionDesc = registerNFn(m_c32vec_0(U"•foreign.Function"), foreignFunction_c1, foreignFunction_c2);
    foreignValueDesc = registerNFn(m_c32vec_0(U"•foreign.Value"), c1_bad, foreignValue_c2);
    foreignPointerDesc = registerNFn(m_c32vec_0(U"•foreign.Pointer"), c1_bad, foreignPointer_c2);
    foreign_nsGen = m_nnsDesc("function","value","pointer","null","sizeof","readbytesto0","readcharsto0");
    nullPointer = gc_add(m_ptrobj(0, m_ffiPrim(sty_void, sty_void), 0));
  }
  B getSysFFI(B path, bool namespace) {
    initSysFFIDesc();
    if (!namespace) return m_nfn(ffiloadDesc, inc(path));
    return m_nns(foreign_nsGen,
      m_nfn(foreignFunctionDesc, inc(path)),
      m_nfn(foreignValueDesc, inc(path)),
      m_nfn(foreignPointerDesc, inc(path)),
      incG(nullPointer),
      incG(bi_foreignSizeof),
      incG(bi_foreignReadBytesTo0),
      incG(bi_foreignReadCharsTo0),
    );
  }

#elif !FOR_BUILD
  static void sysffi_init() { }
  B getSysFFI(B path, bool namespace) { fatal("getSysFFI called"); }
#else // whatever build.bqn uses from •FFI
  #include "utils/nfns.h"
  #include "utils/cstr.h"
  #include <unistd.h>
  #include <poll.h>
  typedef struct pollfd pollfd;
  NFnDesc* forbuildDesc;
  B forbuild_c1(B t, B x) {
    i32 id = o2i(nfn_objU(t));
    switch (id) { default: thrM("bad FOR_BUILD •FFI function");
      case 0: {
        char* s = toCStr(x);
        decG(x);
        i32 r = chdir(s);
        freeCStr(s);
        return m_f64(r);
      }
      case 1: {
        dec(x);
        return m_f64(fork());
      }
      case 2: {
        decG(x);
        int vs[2];
        int r = pipe(vs);
        return m_vec2(m_f64(r), m_vec2(m_f64(vs[0]), m_f64(vs[1])));
      }
      case 3: {
        SGet(x)
        int fd =        o2i(Get(x,0));
        Arr* buf = cpyI8Arr(Get(x,1));
        usz maxlen =    o2s(Get(x,2));
        decG(x);
        assert(PIA(buf)==maxlen);
        int res = read(fd, tyarrv_ptr((TyArr*)buf), maxlen);
        return m_vec2(m_f64(res), taga(buf));
      }
      case 4: {
        SGet(x)
        int fd =        o2i(Get(x,0));
        Arr* buf = cpyI8Arr(Get(x,1));
        usz maxlen =    o2s(Get(x,2));
        decG(x);
        int res = write(fd, tyarrv_ptr((TyArr*)buf), maxlen);
        ptr_dec(buf);
        return m_f64(res);
      }
      case 5: {
        return m_f64(close(o2i(x)));
      }
      case 6: {
        SGet(x)
        Arr* buf = cpyI16Arr(Get(x,0)); i16* a = (i16*)tyarrv_ptr((TyArr*)buf);
        int nfds =       o2i(Get(x,1));
        int timeout =    o2s(Get(x,2));
        decG(x);
        
        TALLOC(pollfd, ps, nfds)
        for (i32 i = 0; i < nfds; i++) ps[i] = (pollfd){.fd = a[i*4+0]|a[i*4+1]<<16, .events=a[i*4+2]};
        int res = poll(&ps[0], nfds, timeout);
        for (i32 i = 0; i < nfds; i++) a[i*4+3] = ps[i].revents;
        TFREE(ps);
        
        return m_vec2(m_f64(res), taga(buf));
      }
      case 7: {
        return m_f64(isatty(o2i(x)));
      }
    }
  }
  static B ffi_names;
  B ffiload_c2(B t, B w, B x) {
    B name = IGetU(x, 1);
    i32 id = 0;
    while (id<IA(ffi_names) && !equal(IGetU(ffi_names, id), name)) id++;
    B r = m_nfn(forbuildDesc, m_f64(id));
    decG(x);
    return r;
  }
  
  DEFINE_NFN ffiloadDesc;
  static void sysffi_init() {
    HArr_p a = m_harrUv(8);
    a.a[0] = m_c8vec_0("chdir");
    a.a[1] = m_c8vec_0("fork");
    a.a[2] = m_c8vec_0("pipe");
    a.a[3] = m_c8vec_0("read");
    a.a[4] = m_c8vec_0("write");
    a.a[5] = m_c8vec_0("close");
    a.a[6] = m_c8vec_0("poll");
    a.a[7] = m_c8vec_0("isatty");
    NOGC_E;
    ffi_names = a.b; gc_add(ffi_names);
    forbuildDesc = registerNFn(m_c8vec_0("(function for build)"), forbuild_c1, c2_bad);
    ffiloadDesc = registerNFn(m_c32vec_0(U"•FFI"), c1_bad, ffiload_c2);
  }
  B getSysFFI(B path, bool namespace) {
    if (namespace) thrM("•foreign isn't supported in bootstrap build");
    return m_nfn(ffiloadDesc, inc(path));
  }
#endif // FOR_BUILD

#if !FFI
  B foreignSizeof_c1(B t, B x) { fatal("foreignSizeof_c1 called"); }
  B foreignReadBytesTo0_c1(B t, B x) { fatal("foreignReadBytesTo0_c1 called"); }
  B foreignReadCharsTo0_c1(B t, B x) { fatal("foreignReadCharsTo0_c1 called"); }
  B foreignReadBytesTo0_c2(B t, B w, B x) { fatal("foreignReadBytesTo0_c2 called"); }
  B foreignReadCharsTo0_c2(B t, B w, B x) { fatal("foreignReadCharsTo0_c2 called"); }
#endif

void ffi_init(void) {
  sysffi_init(); // •FFI
  bqnffi_init(); // bqnffi.h
}
