// RUN: %check_clang_tidy %s libcxx-attribute-not-on-first-decl %t
#define HIDDEN __attribute__((visibility("hidden")))

void f();
HIDDEN void f() {}
// CHECK-MESSAGES: :[[@LINE-1]]:{{.*}}: warning: function 'f' was declared without attribute
// CHECK-FIXES: {{^}}HIDDEN void f();{{$}}
// CHECK-FIXES: {{^}}void f() {}

