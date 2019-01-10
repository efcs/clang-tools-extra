// RUN: %check_clang_tidy %s google-adl-call %t

#define BEGIN_DISABLE_ADL _Pragma("clang attribute push ([[clang::no_adl]], apply_to = function(is_non_member))")
#define END_DISABLE_ADL _Pragma("clang attribute pop")


BEGIN_DISABLE_ADL
namespace NS {
struct T {};
void f(T) {}
void f(T, T) {}

[[clang::allow_adl]]
void g(T) {}
}
END_DISABLE_ADL

void test() {
  NS::T t;
  f(t);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: call to function should not use ADL [google-adl-call]
  // CHECK-FIXES: {{^}}  NS::f(t);
  f(t, t);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: call to function should not use ADL [google-adl-call]
  // CHECK-FIXES: {{^}}  NS::f(t, t);
  g(t);
}


BEGIN_DISABLE_ADL

namespace NestedTest {
namespace {
  namespace N1 { inline namespace V1 { namespace NN1 { namespace {
    struct T {};
    void u(T);
  }}}}

  namespace N2 {
    void test() {
      N1::NN1::T t;
      u(t);
      // CHECK-MESSAGES: :[[@LINE-1]]:7: warning: call to function should not use ADL [google-adl-call]
      // CHECK-FIXES: {{^}}      N1::NN1::u(t);
    }
  }
}
}
END_DISABLE_ADL