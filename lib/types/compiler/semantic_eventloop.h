#pragma once

#if defined(RUPA_PACKAGE_H)

/**
 * @brief Satu event dalam antrian event loop async.
 */
struct AsyncEvent {
  int handleId;       /* AST node id dari async handle */
  RuntimeValue result; /* Hasil evaluasi request */
  bool done;          /* Status penyelesaian */
  struct AsyncEvent *next;
};

/**
 * @brief Event loop untuk menjalankan operasi async secara serial.
 */
struct EventLoop {
  struct AsyncEvent *head;
  struct AsyncEvent *tail;
  int count;
};

#endif
