// RUN: %check_clang_tidy %s libcxx-reserved-names %t
namespace std {
// FIXME: Add something that triggers the check here.
void f();

void bar(int i) { int x; static int y; }
// CHECK-MESSAGES: :[[@LINE-1]]:{{.*}}: warning: use of non-reserved name 'i'
// CHECK-MESSAGES: :[[@LINE-2]]:{{.*}}: warning: use of non-reserved name 'x'
// CHECK-MESSAGES: :[[@LINE-3]]:{{.*}}: warning: use of non-reserved name 'y'
// CHECK-FIXES: {{^}}void bar(int __i) { int __x; static int __y; }{{$}}

template <class... _Tp>
void __eat(_Tp &&...) {}

struct T {
  static int x;
  int y;
  template <class T, class... U, int N, int... NPack>
  void bar(int i, T __t, U...) { T y = N; static int zz; __eat(__t, NPack..., __t); }
  // CHECK-MESSAGES: :[[@LINE-2]]:{{.*}}: warning: use of non-reserved name 'T'
  // CHECK-MESSAGES: :[[@LINE-3]]:{{.*}}: warning: use of non-reserved name 'U'
  // CHECK-MESSAGES: :[[@LINE-4]]:{{.*}}: warning: use of non-reserved name 'N'
  // CHECK-MESSAGES: :[[@LINE-5]]:{{.*}}: warning: use of non-reserved name 'NPack'
  // CHECK-MESSAGES: :[[@LINE-5]]:{{.*}}: warning: use of non-reserved name 'i'
  // CHECK-MESSAGES: :[[@LINE-6]]:{{.*}}: warning: use of non-reserved name 'y'
  // CHECK-MESSAGES: :[[@LINE-7]]:{{.*}}: warning: use of non-reserved name 'zz'
  // CHECK-FIXES: void bar(int __i, _Tp __t, _Up...) { _Tp __y = _Np; static int __zz; __eat(__t, _NPack..., __t); }{{$}}
  using z = int;
  template <class...>
  void test_templ2() {}
  template <class... U>
  void test_templ() { test_templ2<U...>(); }
  // CHECK-MESSAGES: :[[@LINE-2]]:{{.*}}: warning: use of non-reserved name 'U'
  // CHECK-FIXES: template <class... _Up>
  // CHECK-FIXES-NEXT: void test_templ() { test_templ2<_Up...>(); }
private:
  static int xx;
  // CHECK-MESSAGES: :[[@LINE-1]]:{{.*}}: warning: use of non-reserved name 'xx'
  // CHECK-FIXES: static int __xx;
  int yy;
  // CHECK-MESSAGES: :[[@LINE-1]]:{{.*}}: warning: use of non-reserved name 'yy'
  // CHECK-FIXES: int __yy_;
  void baz() { int y = yy; }
  // CHECK-MESSAGES: :[[@LINE-1]]:{{.*}}: warning: use of non-reserved name 'baz'
  // CHECK-MESSAGES: :[[@LINE-2]]:{{.*}}: warning: use of non-reserved name 'y'
  // CHECK-FIXES: void __baz() { int __y = __yy_; }
  void __bar_baz() { baz(); }
  // CHECK-FIXES: {{^}}  void __bar_baz() { __baz(); }{{$}}
  static int value;
  typedef int type;
};
}