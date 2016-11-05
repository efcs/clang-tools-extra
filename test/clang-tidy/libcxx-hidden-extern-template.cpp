// RUN: %check_clang_tidy %s libcxx-hidden-extern-template %t
#define HIDDEN __attribute__((visibility("hidden")))
template <class T> HIDDEN void func(T) {}
extern template void func(int);
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: function 'f' is insufficiently awesome [misc-hidden-extern-template]

template <class T>
struct Struct {
  void foo() {}
  HIDDEN void bar() {}
  void baz();
  HIDDEN void barbaz();
};

template <class T>
HIDDEN
void Struct<T>::baz() {}
template <class T>
HIDDEN
void Struct<T>::barbaz() {}
extern template struct Struct<int>;


// FIXME: Verify the applied fix.
//   * Make the CHECK patterns specific enough and try to make verified lines
//     unique to avoid incorrect matches.
//   * Use {{}} for regular expressions.
// CHECK-FIXES: {{^}}void awesome_f();{{$}}


