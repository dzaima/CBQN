#include "../core.h"
#include "../utils/talloc.h"
#include "../utils/mut.h"
#include "../builtins.h"

extern B ud_c1(B, B);
extern B ne_c2(B, B, B);
extern B slash_c1(B, B);
extern B slash_c2(B, B, B);
extern B select_c2(B, B, B);
extern B take_c2(B, B, B);
extern B drop_c2(B, B, B);
extern B join_c2(B, B, B);

static Arr* arr_shChangeLen(Arr* a, ur r, usz* xsh, usz len) {
  assert(r > 1);
  usz* sh = a->sh = m_shArr(r)->a;
  SPRNK(a,r);
  sh[0] = len;
  shcpy(sh+1, xsh+1, r-1);
  return a;
}
static B m_shChangeLen(u8 xt, ur xr, usz* xsh, usz l, usz cw, usz csz) {
  return taga(arr_shChangeLen(m_arr(offsetof(TyArr, a)+l*cw, xt, l*csz), xr, xsh, l));
}
static void allocGroups(B* rp, usz ria, B z, u8 xt, ur xr, usz* xsh, i32* len, usz width, usz csz) {
  if (xr==1) for (usz j = 0; j < ria; j++) { usz l=len[j]; if (!l) rp[j] = inc(z); else m_tyarrv(rp+j, width, l, xt); }
  else       for (usz j = 0; j < ria; j++) { usz l=len[j]; rp[j] = !l ? inc(z) : m_shChangeLen(xt, xr, xsh, l, width, csz); }
}

extern B rt_group;
B group_c2(B t, B w, B x) {
  if (!isArr(x)) thrM("⊔: 𝕩 must be an array");
  ur xr = RNK(x);
  if (isArr(w) && RNK(w)==1 && xr>=1 && depth(w)==1) {
    usz wia = IA(w);
    usz* xsh = SH(x);
    usz xia = *xsh;
    if (wia-xia > 1) thrF("⊔: ≠𝕨 must be either ≠𝕩 or one bigger (%s≡≠𝕨, %s≡≠𝕩)", wia, xia);
    u8 we = TI(w,elType);
    if (elInt(we)) {
      if (we==el_bit) w = taga(cpyI8Arr(w));
      i64 ria = 0;
      bool bad = false, sort = true;
      usz neg = 0, change = 0;
      void *wp0 = tyany_ptr(w);
      #define CASE(T) case el_##T: { \
        T max = -1, prev = -1;                            \
        for (usz i = 0; i < xia; i++) {                   \
          T n = ((T*)wp0)[i];                             \
          if (n>max) max = n;                             \
          bad |= n < -1;                                  \
          neg += n == -1;                                 \
          sort &= prev <= n;                              \
          change += prev != n;                            \
          prev = n;                                       \
        }                                                 \
        if (wia>xia) { ria=((T*)wp0)[xia]; bad|=ria<-1; } \
        i64 m=(i64)max+1; if (m>ria) ria=m;               \
        break; }
      switch (we) { default:UD; case el_bit: CASE(i8) CASE(i16) CASE(i32) }
      #undef CASE
      if (bad) thrM("⊔: 𝕨 can't contain elements less than ¯1");
      if (ria > (i64)(USZ_MAX)) thrOOM();
      
      Arr* r = arr_shVec(m_fillarrp(ria)); fillarr_setFill(r, m_f64(0));
      B* rp = fillarr_ptr(r);
      for (usz i = 0; i < ria; i++) rp[i] = m_f64(0); // don't break if allocation errors
      B xf = getFillQ(x);
      
      Arr* rf = m_fillarrp(0); if (xr==1) arr_shVec(rf); else arr_shChangeLen(rf, xr, xsh, 0);
      fillarr_setFill(rf, m_f64(0));
      B z = taga(rf);
      fillarr_setFill(r, z);
      
      // Both cases needed to make sure wia>0 for ip[wia-1] below
      if (ria==0) goto setfill_dec_ret;
      if (neg==xia) {
        for (usz i = 0; i < ria; i++) rp[i] = inc(z);
        goto setfill_dec_ret;
      }
      TALLOC(i32, pos, 2*ria+1); i32* len = pos+ria+1;
      
      bool notB = TI(x,elType) != el_B;
      u8 xt = arrNewType(TY(x));
      u8 xl = arrTypeBitsLog(TY(x));
      bool bits = xl == 0;
      u64 width = bits ? 1 : 1<<(xl-3); // cell width in bits if bits==1, bytes otherwise
      usz csz = 1;
      if (RARE(xr>1)) {
        width *= csz = arr_csz(x);
        xl += CTZ(csz);
        if (bits && xl>=3) { bits=0; width>>=3; }
        if ((csz & (csz-1)) || xl>7) xl = 7;
      }
      if (xia>64 && notB && !bits && change<(xia*width)/32) {
        #define C1(F,X  ) F##_c1(m_f64(0),X  )
        #define C2(F,X,W) F##_c2(m_f64(0),X,W)
        if (wia>xia) w = C2(take, m_f64(xia), w);
        B c = C2(ne, C2(drop, m_f64(-1), inc(w)),
                     C2(drop, m_f64( 1), inc(w)));
        B ind = C1(slash, C2(join, m_f64(-1!=IGetU(w,0).f), c));
        w = C2(select, inc(ind), w);
        #undef C1
        #undef C2
        if (TI(ind,elType)!=el_i32) ind = taga(cpyI32Arr(ind));
        if (TI(w  ,elType)!=el_i32) w   = taga(cpyI32Arr(w  ));
        wia = IA(ind);
        
        i32* ip = i32any_ptr(ind);
        i32* wp = i32any_ptr(w);
        usz i0 = ip[0];
        for (usz i=0; i<wia-1; i++) ip[i] = ip[i+1]-ip[i];
        ip[wia-1] = xia-ip[wia-1];
        for (usz i = 0; i < ria; i++) len[i] = pos[i] = 0;
        for (usz i = 0; i < wia; i++) len[wp[i]]+=ip[i];
        
        void* xp = tyany_ptr(x);
        
        allocGroups(rp, ria, z, xt, xr, xsh, len, width, csz);
        for (u64 i=0, k=i0*width; i<wia; i++) {
          u64 k0 = k;
          u64 l = ip[i]*width; k += l;
          i32 n = wp[i]; if (n<0) continue;
          memcpy((u8*)tyarr_ptr(rp[n])+pos[n], (u8*)xp+k0, l);
          pos[n] += l;
        }
        decG(ind);
      } else {
        if (xia>32 && neg>xia/4+xia/8) {
          if (wia>xia) w = take_c2(m_f64(0), m_f64(xia), w);
          B m = ne_c2(m_f64(0), m_f64(-1), inc(w));
          w = slash_c2(m_f64(0), inc(m), w);
          x = slash_c2(m_f64(0), m, x); xia = IA(x);
          neg = 0;
        }
        if (TI(w,elType)!=el_i32) w = taga(cpyI32Arr(w));
        i32* wp = i32any_ptr(w);
        for (usz i = 0; i < ria; i++) len[i] = pos[i] = 0;
        for (usz i = 0; i < xia; i++) len[wp[i]]++; // overallocation makes this safe after n<-1 check
        
        u8 xk = xl - 3;
        if (notB && !bits && sort) {
          void* xp = tyany_ptr(x);
          u64 i=neg*width;
          #define GROUP_SORT(ALLOC) \
            for (usz j=0; j<ria; j++) {                \
              usz l = len[j];                          \
              if (!l) { rp[j]=inc(z); continue; }      \
              ALLOC;                                   \
              u64 lw = l*width;                        \
              memcpy(tyarr_ptr(rp[j]), (u8*)xp+i, lw); \
              i += lw;                                 \
            }
          if (xr==1) GROUP_SORT(m_tyarrv(rp+j, width, l, xt))
          else       GROUP_SORT(rp[j] = m_shChangeLen(xt, xr, xsh, l, width, csz))
          #undef GROUP_SORT
        } else if (notB && xk <= 3) {
          void* xp = tyany_ptr(x);
          allocGroups(rp, ria, z, xt, xr, xsh, len, width, csz);
          switch(xk) { default: UD;
            case 0: for (usz i = 0; i < xia; i++) { i32 n = wp[i]; if (n>=0) ((u8* )tyarr_ptr(rp[n]))[pos[n]++] = ((u8* )xp)[i]; } break;
            case 1: for (usz i = 0; i < xia; i++) { i32 n = wp[i]; if (n>=0) ((u16*)tyarr_ptr(rp[n]))[pos[n]++] = ((u16*)xp)[i]; } break;
            case 2: for (usz i = 0; i < xia; i++) { i32 n = wp[i]; if (n>=0) ((u32*)tyarr_ptr(rp[n]))[pos[n]++] = ((u32*)xp)[i]; } break;
            case 3: for (usz i = 0; i < xia; i++) { i32 n = wp[i]; if (n>=0) ((u64*)tyarr_ptr(rp[n]))[pos[n]++] = ((u64*)xp)[i]; } break;
          }
        } else {
          for (usz i = 0; i < ria; i++) {
            usz l = len[i];
            Arr* c = m_fillarrp(l*csz);
            c->ia = 0;
            fillarr_setFill(c, inc(xf));
            if (xr==1) arr_shVec(c); else arr_shChangeLen(c, xr, xsh, l);
            rp[i] = taga(c);
          }
          SLOW2("𝕨⊔𝕩", w, x);
          SGet(x)
          if (csz == 1) {
            for (usz i = 0; i < xia; i++) {
              i32 n = wp[i];
              if (n>=0) fillarr_ptr(a(rp[n]))[pos[n]++] = Get(x, i);
            }
          } else {
            for (usz i = 0; i < xia; i++) {
              i32 n = wp[i];
              if (n<0) continue;
              usz p = (pos[n]++)*csz;
              B* rnp = fillarr_ptr(a(rp[n])) + p;
              for (usz j = 0; j < csz; j++) rnp[j] = Get(x, i*csz + j);
            }
          }
          for (usz i = 0; i < ria; i++) a(rp[i])->ia = len[i]*csz;
        }
      }
      TFREE(pos);
      setfill_dec_ret:
      fillarr_setFill(rf, xf);
      decG(w); decG(x);
      return taga(r);
    } else if (xr==1) {
      SLOW2("𝕨⊔𝕩", w, x);
      SGetU(w)
      i64 ria = wia==xia? 0 : o2i64(GetU(w, xia));
      if (ria<-1) thrM("⊔: 𝕨 can't contain elements less than ¯1");
      ria--;
      for (usz i = 0; i < xia; i++) {
        B cw = GetU(w, i);
        if (!q_i64(cw)) goto base;
        i64 c = o2i64G(cw);
        if (c>ria) ria = c;
        if (c<-1) thrM("⊔: 𝕨 can't contain elements less than ¯1");
      }
      if (ria > (i64)(USZ_MAX-1)) thrOOM();
      ria++;
      TALLOC(i32, lenO, ria+1); i32* len = lenO+1;
      TALLOC(i32, pos, ria);
      for (usz i = 0; i < ria; i++) len[i] = pos[i] = 0;
      for (usz i = 0; i < xia; i++) len[o2i64G(GetU(w, i))]++;
      
      Arr* r = arr_shVec(m_fillarrp(ria)); fillarr_setFill(r, m_f64(0));
      B* rp = fillarr_ptr(r);
      for (usz i = 0; i < ria; i++) rp[i] = m_f64(0); // don't break if allocation errors
      B xf = getFillQ(x);
      
      for (usz i = 0; i < ria; i++) {
        Arr* c = m_fillarrp(len[i]);
        c->ia = 0;
        fillarr_setFill(c, inc(xf));
        arr_shVec(c);
        rp[i] = taga(c);
      }
      Arr* rf = m_fillarrp(0); arr_shVec(rf);
      fillarr_setFill(rf, xf);
      fillarr_setFill(r, taga(rf));
      SGet(x)
      for (usz i = 0; i < xia; i++) {
        i64 n = o2i64G(GetU(w, i));
        if (n>=0) fillarr_ptr(a(rp[n]))[pos[n]++] = Get(x, i);
      }
      for (usz i = 0; i < ria; i++) a(rp[i])->ia = len[i];
      decG(w); decG(x); TFREE(lenO); TFREE(pos);
      return taga(r);
    }
  }
  base:
  return c2(rt_group, w, x);
}
B group_c1(B t, B x) {
  if (isArr(x) && RNK(x)==1 && TI(x,arrD1)) {
    usz ia = IA(x);
    B range = ud_c1(t, m_f64(ia));
    return group_c2(m_f64(0), x, range);
  }
  return c1(rt_group, x);
}
