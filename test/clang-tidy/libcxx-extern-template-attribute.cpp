// RUN: %check_clang_tidy %s libcxx-extern-template %t

#define _LIBCPP_INLINE_VISIBILITY __attribute__((__always_inline__))
#define _LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY __attribute__((__always_inline__))
namespace std {
template <class T>
struct Foo {
  // CHECK-MESSAGES: :[[@LINE-2]]:{{.*}}: warning: explicitly instantiated function 'bar' is missing inline [libcxx-extern-template]
  // CHECK-MESSAGES: :[[@LINE-1]]:{{.*}}: warning: function 'bar' is missing visibility declaration [libcxx-extern-template]
  void bar();

  // CHECK-MESSAGES: :[[@LINE-2]]:{{.*}}: warning: explicitly instantiated function 'baz' is missing inline [libcxx-extern-template]
  // CHECK-MESSAGES: :[[@LINE-1]]:{{.*}}: warning: function 'baz' is missing visibility declaration [libcxx-extern-template]
  void baz();

  // CHECK-MESSAGES: :[[@LINE-2]]:{{.*}}: warning: explicitly instantiated function 'foobar' is missing inline [libcxx-extern-template]
  // CHECK-MESSAGES: :[[@LINE-1]]:{{.*}}: warning: function 'foobar' is missing visibility declaration [libcxx-extern-template]
  void foobar();

  // CHECK-MESSAGES: :[[@LINE-1]]:{{.*}}: warning: function 'bobo' is missing visibility declaration [libcxx-extern-template]
  void bobo() {}

  // CHECK-MESSAGES: :[[@LINE-2]]:{{.*}}: warning: explicitly instantiated function 'bababa' is missing inline [libcxx-extern-template]
  // CHECK-MESSAGES: :[[@LINE-1]]:{{.*}}: warning: function 'bababa' is missing visibility declaration [libcxx-extern-template]
  void bababa();

  // CHECK-MESSAGES-NOT: dummy
  template <class U>
  void dummy1() {}
  template <class U>
  void dummy2();

  ~Foo();
};

template <class T>
Foo<T>::~Foo() {}

template <class T>
_LIBCPP_INLINE_VISIBILITY void Foo<T>::bar() {
}

template <class T>
void Foo<T>::baz() {
}

template <class T>
_LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY
void Foo<T>::foobar() {}

template <class T>
inline _LIBCPP_INLINE_VISIBILITY void Foo<T>::bababa() {}

template <class T>
template <class U>
void Foo<T>::dummy2() {}

template struct Foo<int>;

} // namespace std

// CHECK-FIXES: {{^}}HIDDEN void f();{{$}}
// CHECK-FIXES: {{^}}void f() {}
