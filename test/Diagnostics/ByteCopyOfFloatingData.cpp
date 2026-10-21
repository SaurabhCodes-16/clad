// RUN: %cladclang %s -I%S/../../include -fsyntax-only -Xclang -verify 2>&1
//
// A byte copy of floating-point data moves values clad has to differentiate,
// through `void*`. Reduced from ImpactX's SmallMatrix, which holds its elements
// in a C array.

#include "clad/Differentiator/Differentiator.h"
#include <array>
#include <cstring>

// A copy of integers has a zero derivative. It comes first: the memcpy
// request it makes must not hide the copies below.
struct I { int m[2]; }; // expected-warning 2 {{attempted differentiation of function '__builtin_memcpy' without definition}} \
                        // expected-note 2 {{considering '__builtin_memcpy' as 0}}

double int_copy(double k) {
  I a{}, b{};
  b = a;
  return k * b.m[0];
}

double int_memcpy(double k) {
  int a[2] = {1, 2}, b[2];
  __builtin_memcpy(b, a, sizeof a);
  return k * b[0];
}

struct V { double m[2]; };

double copy_assign(double k) {
  V a{};
  a.m[0] = k;
  V b{};
  b = a; // expected-error {{cannot differentiate the defaulted copy assignment of 'V': it copies 'double[2]' with '__builtin_memcpy', which has no derivative}} \
         // expected-note {{provide custom 'operator_equal_reverse_forw' and 'operator_equal_pullback' for 'V'}}
  return b.m[0];
}

// std::array's assignment copies its elements the same way.
double std_array(double k) {
  std::array<double, 2> a{k, 2 * k}, b{};
  b = a; // expected-error {{cannot differentiate the defaulted copy assignment of 'array<double, 2>'}} \
         // expected-note {{provide custom 'operator_equal_reverse_forw' and 'operator_equal_pullback' for 'array<double, 2>'}}
  return b[0] * b[1];
}

double builtin_memcpy(double k) {
  double a[2] = {k, 2 * k}, b[2];
  __builtin_memcpy(b, a, sizeof a); // expected-error {{cannot differentiate '__builtin_memcpy' copying 'double[2]', which holds floating-point values; its derivative would be zero}} \
                                    // expected-note {{copy the values with assignments, which clad can differentiate}}
  return b[0] + b[1];
}

struct P { double x, y; };

double user_memcpy(double k) {
  P a[2] = {{k, k}, {k, k}};
  P b[2];
  std::memcpy(b, a, sizeof(a)); // expected-error {{cannot differentiate 'memcpy' copying 'P[2]'}} \
                                // expected-note {{copy the values with assignments}}
  return b[1].y;
}

// The floating-point end of a copy may be the source.
double through_bytes(double x) {
  unsigned char buf[sizeof(double)];
  std::memcpy(buf, &x, sizeof x); // expected-error {{cannot differentiate 'memcpy' copying 'double'}} \
                                  // expected-note {{copy the values with assignments}}
  double y;
  std::memcpy(&y, buf, sizeof y); // expected-error {{cannot differentiate 'memcpy' copying 'double'}} \
                                  // expected-note {{copy the values with assignments}}
  return y;
}

int main() {
  clad::gradient(int_copy);
  clad::gradient(int_memcpy);
  clad::gradient(copy_assign);
  clad::gradient(std_array);
  clad::gradient(builtin_memcpy);
  clad::gradient(user_memcpy);
  clad::gradient(through_bytes);
}
