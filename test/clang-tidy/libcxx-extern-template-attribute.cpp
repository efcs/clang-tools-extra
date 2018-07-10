// RUN: %check_clang_tidy %s libcxx-extern-template %t

#define _LIBCPP_INLINE_VISIBILITY __attribute__((__always_inline__))
#define _LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY __attribute__((__always_inline__))
#define _LIBCPP_NORETURN [[noreturn]]

namespace std {

template <bool>
struct ThrowBase {
  _LIBCPP_INLINE_VISIBILITY ThrowBase() {}
};


extern template struct ThrowBase<true>;
extern template struct ThrowBase<false>;
template struct ThrowBase<true>;
template struct ThrowBase<false>;

template <class T>
struct Foo {
  // CHECK-MESSAGES: :[[@LINE+2]]:{{.*}}: warning: explicitly instantiated function 'bar' is missing inline [libcxx-extern-template]
  // CHECK-MESSAGES: :[[@LINE+1]]:{{.*}}: warning: function 'bar' is missing visibility declaration [libcxx-extern-template]
  void bar();
  // CHECK-FIXES: inline _LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY void bar();

  // CHECK-MESSAGES: :[[@LINE+2]]:{{.*}}: warning: explicitly instantiated function 'baz' is missing inline [libcxx-extern-template]
  // CHECK-MESSAGES: :[[@LINE+1]]:{{.*}}: warning: function 'baz' is missing visibility declaration [libcxx-extern-template]
  void baz();
  // CHECK-FIXES: inline _LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY void baz();

  // CHECK-MESSAGES: :[[@LINE+2]]:{{.*}}: warning: explicitly instantiated function 'foobar' is missing inline [libcxx-extern-template]
  // CHECK-MESSAGES: :[[@LINE+1]]:{{.*}}: warning: function 'foobar' is missing visibility declaration [libcxx-extern-template]
  void foobar();
  // CHECK-FIXES: inline _LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY void foobar();

  // CHECK-MESSAGES: :[[@LINE+1]]:{{.*}}: warning: function 'bobo' is missing visibility declaration [libcxx-extern-template]
  void bobo() {}
  // CHECK-FIXES:_LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY void bobo() {}

  // CHECK-MESSAGES: :[[@LINE+2]]:{{.*}}: warning: explicitly instantiated function 'bababa' is missing inline [libcxx-extern-template]
  // CHECK-MESSAGES: :[[@LINE+1]]:{{.*}}: warning: function 'bababa' is missing visibility declaration [libcxx-extern-template]
  void bababa();
  // CHECK-FIXES: inline _LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY void bababa();

  // CHECK-MESSAGES-NOT: dummy
  template <class U> void dummy1() {}
  template <class U> void dummy2();
  // CHECK-FIXES:{{^}}  template <class U> void dummy1() {}
  // CHECK-FIXES:{{^}}  template <class U> void dummy2();

  // CHECK-MESSAGES: :[[@LINE+1]]:{{.*}}: warning: explicitly instantiated function '~Foo<T>' is missing inline [libcxx-extern-template]
  ~Foo();
  // CHECK-FIXES:{{^}}  inline ~Foo();
};

template <class T>
Foo<T>::~Foo() {}

// CHECK-MESSAGES: :[[@LINE+2]]:{{.*}}: warning: visibility declaration does not occur on the first declaration of 'bar' [libcxx-extern-template]
template <class T>
_LIBCPP_INLINE_VISIBILITY void Foo<T>::bar() {}
// CHECK-FIXES:{{^}}void Foo<T>::bar() {}


template <class T>
void Foo<T>::baz() {}
// CHECK-FIXES:{{^}}template <class T>
// CHECK-FIXES-NEXT:{{^}}void Foo<T>::baz() {}


// CHECK-MESSAGES: :[[@LINE+2]]:{{.*}}: warning: visibility declaration does not occur on the first declaration of 'foobar' [libcxx-extern-template]
template <class T>
_LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY
void Foo<T>::foobar() {}
// CHECK-FIXES:{{^}}template <class T>
// CHECK-FIXES-NEXT:{{^}}void Foo<T>::foobar() {}


// CHECK-MESSAGES: :[[@LINE+2]]:{{.*}}: warning: visibility declaration does not occur on the first declaration of 'bababa' [libcxx-extern-template]
template <class T>
inline _LIBCPP_INLINE_VISIBILITY void Foo<T>::bababa() {}
// CHECK-FIXES:{{^}}inline void Foo<T>::bababa() {}


template <class T>
template <class U>
void Foo<T>::dummy2() {}
// CHECK-FIXES: template <class T>
// CHECK-FIXES-NEXT: template <class U>
// CHECK-FIXES-NEXT:{{^}}void Foo<T>::dummy2() {}

template struct Foo<int>;

} // namespace std
