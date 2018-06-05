// RUN: %check_clang_tidy %s libcxx-extern-template %t

#define _LIBCPP_INLINE_VISIBILITY __attribute__((__always_inline__))

namespace std {
template <class T>
struct Foo {
  void bar();
};

template <class T>
_LIBCPP_INLINE_VISIBILITY void Foo<T>::bar() {
}

template struct Foo<int>;

} // namespace std

// CHECK-MESSAGES: :[[@LINE-1]]:{{.*}}: waaoeu
// CHECK-FIXES: {{^}}HIDDEN void f();{{$}}
// CHECK-FIXES: {{^}}void f() {}
