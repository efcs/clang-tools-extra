// RUN: %check_clang_tidy %s libcxx-template-missing-inline %t

// FIXME: Add something that triggers the check here.
template <class T> inline void f(T);
template <class T> void f(T) {}

template <class T> void g(T) {}
// CHECK-MESSAGES: :[[@LINE-1]]:{{.*}}: warning: function template 'g' is not inline
// CHECK-FIXES: {{^}}template <class T> inline void g(T){{.*}}
template <class T>
struct Foo {
  void bar() {}
  void baz();
  // CHECK-MESSAGES: :[[@LINE-1]]:{{.*}}: warning: function template 'baz' is not inline
  // CHECK-FIXES: inline void baz();
  template <class U>
  inline void foo(U);

  static int compare();
};

template <class T>
void Foo<T>::baz() {}

template <class T>
int Foo<T>::compare() {return 0;}

template <class T>
template <class U>
void Foo<T>::foo(U) {}
