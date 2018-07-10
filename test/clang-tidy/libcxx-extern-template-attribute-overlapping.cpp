// RUN: rm -f %T/instantiate_overlapping.cpp overlapping.hpp %T/test.cpp
// RUN: cp %S/Inputs/libcxx-extern-template-attribute/overlapping.hpp %T/overlapping.hpp
// RUN: cp %S/Inputs/libcxx-extern-template-attribute/instantiate_overlapping.cpp %T/instantiate_overlapping.cpp
// RUN: cp %s %T/test.cpp
// RUN: clang-tidy -checks=-*,libcxx-extern-template -header-filter=.* \
// RUN:    %T/test.cpp %T/instantiate_overlapping.cpp -fix -- -std=c++14 -I%T
// RUN: FileCheck --input-file %T/overlapping.hpp %s

// Test that the implicit instantiation in this file does not cause the
// functions it uses to be re-considered and re-fixed.

#include "overlapping.hpp"
// CHECK: struct Foo {
// CHECK-NEXT: inline _LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY Foo();

// CHECK: template <class T> inline _LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY T foo() { return 42; }

namespace std {
Foo<int> x;
auto y = foo<int>();
}
