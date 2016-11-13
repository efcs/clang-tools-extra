// RUN: %check_clang_tidy %s libcxx-reserved-names %t
namespace std {
// FIXME: Add something that triggers the check here.
void f();

void bar(int i) { int x; static int y; }
// CHECK-MESSAGES: :[[@LINE-1]]:{{.*}}: warning: use of non-reserved name 'i'
// CHECK-MESSAGES: :[[@LINE-2]]:{{.*}}: warning: use of non-reserved name 'x'
// CHECK-MESSAGES: :[[@LINE-3]]:{{.*}}: warning: use of non-reserved name 'y'

void baz(int i);
// CHECK-MESSAGES: :[[@LINE-1]]:{{.*}}: warning: use of non-reserved name 'i'
// CHECK-FIXES: {{^}}void baz(int __i);{{$}}

struct T {
  static int x;
  int y;
  template <class T> void bar(int i) { int y; static int zz;}
  // CHECK-MESSAGES: :[[@LINE-1]]:{{.*}}: warning: use of non-reserved name 'T'
  // CHECK-MESSAGES: :[[@LINE-2]]:{{.*}}: warning: use of non-reserved name 'i'
  // CHECK-MESSAGES: :[[@LINE-3]]:{{.*}}: warning: use of non-reserved name 'y'
  // CHECK-MESSAGES: :[[@LINE-4]]:{{.*}}: warning: use of non-reserved name 'zz'
  using z = int;
private:
  static int xx;
  // CHECK-MESSAGES: :[[@LINE-1]]:{{.*}}: warning: use of non-reserved name 'xx'
  int yy;
  // CHECK-MESSAGES: :[[@LINE-1]]:{{.*}}: warning: use of non-reserved name 'yy'
  void baz() { int y; }
  // CHECK-MESSAGES: :[[@LINE-1]]:{{.*}}: warning: use of non-reserved name 'baz'
  // CHECK-MESSAGES: :[[@LINE-2]]:{{.*}}: warning: use of non-reserved name 'y'
};
}