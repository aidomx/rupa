#pragma once
#if defined(RUPA_PACKAGE_H)

/**
 * @brief Satu binding variabel dalam scope environment.
 * Menyimpan nama, tipe, dan nilai runtime dari sebuah variabel.
 */
struct RuntimeBinding {
  char *name;
  char *type;
  RuntimeValue value;
  struct RuntimeBinding *next;
};

/**
 * @brief Environment scope — linked list ke parent scope.
 * Digunakan oleh semantic layer untuk mengelola deklarasi dan
 * pencarian variabel selama interpretasi.
 */
struct RuntimeEnv {
  struct RuntimeEnv *parent;
  struct RuntimeBinding *bindings;
};

#endif
