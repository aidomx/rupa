#pragma once

#if defined(RUPA_PACKAGE_H)

/**
 * @brief State mesin untuk async event.
 */
enum AsyncEventState {
  ASYNC_PENDING,  /* Belum diproses */
  ASYNC_AWAIT,    /* Handler dipanggil dengan status AWAIT */
  ASYNC_SUCCESS,  /* Request selesai, handler dipanggil dengan SUCCESS */
  ASYNC_ERROR,    /* Error atau timeout, handler dipanggil dengan ERROR */
  ASYNC_DONE      /* Semua handler invocation selesai */
};

/**
 * @brief Satu event dalam antrian event loop async.
 */
struct AsyncEvent {
  int handleId;        /* AST node id dari async handle */
  int requestId;       /* AST node id dari request expression */
  int handlerId;       /* AST node id dari handler (-1 jika tidak ada) */
  int timeoutMs;       /* Timeout dalam milidetik (-1 jika tidak ada) */
  int loaderId;        /* AST node id dari loader identifier (-1 jika tidak ada) */
  int timeoutId;       /* AST node id dari timeout identifier (-1 jika tidak ada) */
  long startTime;      /* Waktu mulai (epoch ms) */
  RuntimeValue result; /* Hasil evaluasi request */
  enum AsyncEventState state; /* State mesin async */
  int pollCount;       /* Berapa kali handler sudah dipanggil */
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
