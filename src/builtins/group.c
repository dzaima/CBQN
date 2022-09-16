#include "../core.h"
#include "../utils/talloc.h"
#include "../utils/mut.h"
#include "../builtins.h"

extern B ne_c2(B, B, B);
extern B slash_c1(B, B);
extern B slash_c2(B, B, B);
extern B select_c2(B, B, B);
extern B take_c2(B, B, B);
extern B drop_c2(B, B, B);
extern B join_c2(B, B, B);

extern B rt_group;
B group_c2(B t, B w, B x) {
  if (isArr(w)&isArr(x) && RNK(w)==1 && RNK(x)==1 && depth(w)==1) {
    usz wia = IA(w);
    usz xia = IA(x);
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

      Arr* rf = arr_shVec(m_fillarrp(0)); fillarr_setFill(rf, m_f64(0));
      B z = taga(rf);
      fillarr_setFill(r, z);

      TALLOC(i32, pos, 2*ria+1); i32* len = pos+ria+1;
      // Both cases needed to make sure wia>0 for ip[wia-1] below
      if (ria==0) goto intvec_ret;
      if (neg==xia) {
        for (usz i = 0; i < ria; i++) rp[i] = inc(z);
        goto intvec_ret;
      }

      u8 xe = TI(x,elType);
      u8 width = elWidth(xe);
      u64 xw;
      if (xia>64 && (xw=(u64)xia*width)<=I32_MAX && change<xw/32 && xe!=el_bit && xe!=el_B) {
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
        u8 xt = el2t(xe);

        for (usz j = 0; j < ria; j++) {
          usz l=len[j];
          if (!l) rp[j]=inc(z); else m_tyarrv(rp+j, width, l, xt);
        }
        for (usz i=0, k=i0*width; i<wia; i++) {
          usz k0 = k;
          usz l = ip[i]*width; k += l;
          i32 n = wp[i]; if (n<0) continue;
          memcpy((u8*)tyarr_ptr(rp[n])+pos[n], (u8*)xp+k0, l);
          pos[n] += l;
        }
        decG(ind); goto intvec_ret;
      }
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
      
      switch (xe) { default: UD;
        case el_i8: case el_c8:
        case el_i16: case el_c16:
        case el_i32: case el_c32: case el_f64: {
          void* xp = tyany_ptr(x);
          u8 xt = el2t(xe);
          if (sort) {
            for (usz j=0, i=neg*width; j<ria; j++) {
              usz l = len[j];
              if (!l) { rp[j]=inc(z); continue; }
              m_tyarrv(rp+j, width, l, xt);
              usz lw = l*width;
              memcpy(tyarr_ptr(rp[j]), (u8*)xp+i, lw);
              i += lw;
            }
            break;
          }
          
          for (usz j = 0; j < ria; j++) {
            usz l=len[j];
            if (!l) rp[j]=inc(z); else m_tyarrv(rp+j, width, l, xt);
          }
          switch(width) { default: UD;
            case 1: for (usz i = 0; i < xia; i++) { i32 n = wp[i]; if (n>=0) ((u8* )tyarr_ptr(rp[n]))[pos[n]++] = ((u8* )xp)[i]; } break;
            case 2: for (usz i = 0; i < xia; i++) { i32 n = wp[i]; if (n>=0) ((u16*)tyarr_ptr(rp[n]))[pos[n]++] = ((u16*)xp)[i]; } break;
            case 4: for (usz i = 0; i < xia; i++) { i32 n = wp[i]; if (n>=0) ((u32*)tyarr_ptr(rp[n]))[pos[n]++] = ((u32*)xp)[i]; } break;
            case 8: for (usz i = 0; i < xia; i++) { i32 n = wp[i]; if (n>=0) ((f64*)tyarr_ptr(rp[n]))[pos[n]++] = ((f64*)xp)[i]; } break;
          }
          break;
        }
        case el_bit: case el_B: {
          for (usz i = 0; i < ria; i++) {
            Arr* c = m_fillarrp(len[i]);
            c->ia = 0;
            fillarr_setFill(c, inc(xf));
            arr_shVec(c);
            rp[i] = taga(c);
          }
          SLOW2("𝕨⊔𝕩", w, x);
          SGet(x)
          for (usz i = 0; i < xia; i++) {
            i32 n = wp[i];
            if (n>=0) fillarr_ptr(a(rp[n]))[pos[n]++] = Get(x, i);
          }
          for (usz i = 0; i < ria; i++) a(rp[i])->ia = len[i];
          break;
        }
      }
      intvec_ret:
      fillarr_setFill(rf, xf);
      decG(w); decG(x); TFREE(pos);
      return taga(r);
    } else {
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
B ud_c1(B, B);
B group_c1(B t, B x) {
  if (isArr(x) && RNK(x)==1 && TI(x,arrD1)) {
    usz ia = IA(x);
    B range = ud_c1(t, m_f64(ia));
    return group_c2(m_f64(0), x, range);
  }
  return c1(rt_group, x);
}

