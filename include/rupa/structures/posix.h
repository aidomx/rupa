#pragma once

#include "rupa/package.h"

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
  Node *node;
  struct Token *tokens;
  int left;
  Position right;
  int programId;
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
