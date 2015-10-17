// RUN: %python %S/check_clang_tidy.py %s libcxx-reserved-name %t

namespace std {
inline namespace __version {

template <class T, int N>
void foo(T x = N) {
  // CHECK-MESSAGES: :[[@LINE-2]]:11: warning: non-reserved name 'T' used as identifier
  // CHECK-MESSAGES: :[[@LINE-3]]:20: warning: non-reserved name 'N' used as identifier
  // CHECK-MESSAGES: :[[@LINE-3]]:10: warning: non-reserved name 'x' used as identifier

  int y;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: non-reserved name 'y' used as identifier
}

int z; // OK

namespace experimental {
inline namespace v1 {

template <class T, int N>
void foo(T x = N) {
 // CHECK-MESSAGES: :[[@LINE-2]]:11: warning: non-reserved name 'T' used as identifier
 // CHECK-MESSAGES: :[[@LINE-3]]:20: warning: non-reserved name 'N' used as identifier
 // CHECK-MESSAGES: :[[@LINE-3]]:10: warning: non-reserved name 'x' used as identifier

  int y;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: non-reserved name 'y' used as identifier
}

} // inline namespace v1
} // namespace experimental

} // inline namespace __version
} // namespace std


// Check that the same code doesn't emit warnings when it's not in
// namespace std
namespace not_std {
template <class T, int N>
void foo(T x = N) {
  int y;
}
} // namespace not_std


