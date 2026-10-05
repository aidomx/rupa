#pragma once

/**
 * @brief Position range untuk token parsing.
 *
 * Merepresentasikan range token (start..end) untuk menandai posisi
 * sub-ekspresi.
 */
struct Position {
  int start;
  int end;
};

/**
 * @brief Request state untuk parsing.
 *
 * Menyimpan state parsing: node collection, token list, left position, dan
 * right range.
 */
struct Request {
  struct Node *node;
  struct Token *tokens;
  int left;
  struct Position right;
  int programId;
  struct Error *error;
  /* Memo parse per-produksi (caches/syntax/expr.c): span konten-identik
   * → node id. Dibuat lazily oleh memo; hidup di GC arena selama Request. */
  void *syntaxMemo;
};

/**
 * @brief Response hasil parsing.
 *
 * Menyimpan hasil parsing berupa indeks node AST yang dihasilkan.
 */
struct Response {
  int nodeId;
  int leftId;
  int rightId;
  int nextId;
};
