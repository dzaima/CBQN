B m_cai32(usz ia, i32* a);
B m_caf64(usz sz, f64* a);


I8Arr*  cpyI8Arr (B x); // consumes
I16Arr* cpyI16Arr(B x); // consumes
I32Arr* cpyI32Arr(B x); // consumes
F64Arr* cpyF64Arr(B x); // consumes

// all consume x
static I8Arr*  toI8Arr (B x) { return v(x)->type==t_i8arr ? c(I8Arr, x) : cpyI8Arr (x); }
static I16Arr* toI16Arr(B x) { return v(x)->type==t_i16arr? c(I16Arr,x) : cpyI16Arr(x); }
static I32Arr* toI32Arr(B x) { return v(x)->type==t_i32arr? c(I32Arr,x) : cpyI32Arr(x); }
static F64Arr* toF64Arr(B x) { return v(x)->type==t_f64arr? c(F64Arr,x) : cpyF64Arr(x); }

static B toI8Any (B x) { u8 t=v(x)->type; return t==t_i8arr  || t==t_i8slice ? x : taga(cpyI8Arr (x)); }
static B toI16Any(B x) { u8 t=v(x)->type; return t==t_i16arr || t==t_i16slice? x : taga(cpyI16Arr(x)); }
static B toI32Any(B x) { u8 t=v(x)->type; return t==t_i32arr || t==t_i32slice? x : taga(cpyI32Arr(x)); }
static B toF64Any(B x) { u8 t=v(x)->type; return t==t_f64arr || t==t_f64slice? x : taga(cpyF64Arr(x)); }