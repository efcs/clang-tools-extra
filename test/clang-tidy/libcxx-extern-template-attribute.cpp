// RUN: %check_clang_tidy %s libcxx-extern-template %t

#define _LIBCPP_INLINE_VISIBILITY __attribute__((__always_inline__))
#define _LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY __attribute__((__always_inline__))
namespace std {
template <class T>
struct Foo {
  _LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY inline void bar();
  _LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY inline void baz();
  _LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY inline void foobar();
  _LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY void bobo() {}
  template <class U>
  void dummy1() {}
  template <class U>
  void dummy2();
};

template <class T>
void Foo<T>::bar() {
}

template <class T>
void Foo<T>::baz() {
}

template <class T>
void Foo<T>::foobar() {}

template <class T>
template <class U>
void Foo<T>::dummy2() {}

template struct Foo<int>;

} // namespace std

// CHECK-MESSAGES: :[[@LINE-1]]:{{.*}}: waaoeu
// CHECK-FIXES: {{^}}HIDDEN void f();{{$}}
// CHECK-FIXES: {{^}}void f() {}
