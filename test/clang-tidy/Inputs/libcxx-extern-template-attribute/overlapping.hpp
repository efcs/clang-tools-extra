#pragma once

#define _LIBCPP_INLINE_VISIBILITY __attribute__((always_inline))
#define _LIBCPP_EXTERN_TEMPLATE_INLINE_VISIBILITY __attribute__((always_inline))

namespace std {
template <class T>
struct Foo {
  Foo();
};

template <class T>
Foo<T>::Foo() {}

extern template struct Foo<int>;

template <class T> T foo() { return 42; }

extern template int foo();

}
