// An object read through a pointer that has no tangent has none either: in
// vector mode a copy of it, a reference to it, or a pointer to it is left
// without one rather than given a vector of zeros of its type.
// RUN: %cladclang %s -I%S/../../include -oVectorModeNoTangent.out \
// RUN:   -Xclang -verify
// RUN: ./VectorModeNoTangent.out | %filecheck_exec %s

#include "clad/Differentiator/Differentiator.h"

#include <cstdio>

struct Inner { double a; };
struct Outer { Inner in; };
Outer outer{{2}};
Outer* outerPtr = &outer;

double objectNoTangent(double x, double y) {
  // expected-warning@+1 {{'in' is accessed through an expression without a derivative; its derivative is treated as 0}}
  Inner copy = outerPtr->in;
  // expected-warning@+1 {{'in' is accessed through an expression without a derivative; its derivative is treated as 0}}
  const Inner& ref = outerPtr->in;
  Outer* ptr = outerPtr;
  return x * y;
}

int main() {
  double dx = 0, dy = 0;
  auto d = clad::differentiate<clad::opts::vector_mode>(objectNoTangent, "x,y");
  d.execute(1, 2, &dx, &dy);
  printf("{%.2f, %.2f}\n", dx, dy); // CHECK-EXEC: {2.00, 1.00}
}
