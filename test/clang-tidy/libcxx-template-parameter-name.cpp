// RUN: %python %S/check_clang_tidy.py %s llvm-template-parameter-name %t

template <class T, int N>
void foo(T x = N);
 // CHECK-MESSAGES: :[[@LINE-2]]:11: warning: non-reserved name 'T' used as identifier
 // CHECK-MESSAGES: :[[@LINE-3]]:20: warning: non-reserved name 'N' used as identifier
 // CHECK-MESSAGES: :[[@LINE-3]]:10: warning: non-reserved name 'x' used as identifier

void bar() {
  int y;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: non-reserved name 'y' used as identifier
}

int z; // OK
