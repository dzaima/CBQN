#pragma once
#include "core.h"
#include <math.h>

#define BQN_PI 3.141592653589793

static CpxVal cpx_add(CpxVal w, CpxVal x) { return (CpxVal){w.re+x.re, w.im+x.im}; }
static CpxVal cpx_sub(CpxVal w, CpxVal x) { return (CpxVal){w.re-x.re, w.im-x.im}; }
static CpxVal cpx_mul(CpxVal w, CpxVal x) { return (CpxVal){w.re*x.re - w.im*x.im, w.re*x.im + w.im*x.re}; }
static CpxVal cpx_div(CpxVal w, CpxVal x) {
  f64 tre, tim, div;
  if (x.im==0) { // make sure that complex division exactly matches real division for real divisor
    tre = w.re;
    tim = w.im;
    div = x.re + 0;
  } else { // TODO handle overflow?
    div = x.re*x.re + x.im*x.im;
    tim = w.im*x.re - w.re*x.im;
    tre = w.re*x.re + w.im*x.im;
  }
  return (CpxVal){tre/div, tim/div};
}

static CpxVal cpx_log(CpxVal x) {
  return (CpxVal){log(hypot(x.re, x.im)), atan2(x.im, x.re)};
}
static CpxVal cpx_exp(CpxVal x) {
  f64 e = exp(x.re);
  return (CpxVal){e*cos(x.im), e*sin(x.im)};
}

static CpxVal cpx_sqrt_re(f64 re) {
  if (!COMPLEX_SUPPORT) return (CpxVal){sqrt(re),0};
  f64 root = sqrt(fabs(re));
  return re<0? (CpxVal){0, root} : (CpxVal){root, 0};
}

static CpxVal cpx_sqrt(CpxVal x) {
  if (x.im == 0) return cpx_sqrt_re(x.re);
  f64 r = hypot(x.re, x.im);
  f64 re = sqrt(0.5*(r+x.re));
  f64 im = sqrt(0.5*(r-x.re));
  return (CpxVal){re, x.im<0? -im : im};
}
