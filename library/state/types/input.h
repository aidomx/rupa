#pragma once
/**
 * Struktur untuk input
 */
struct Input {
  /*char *buffer;*/
  char *content;
  int capacity;
  int cursor;
  int length;
  int line;
  int row;
  // struct layer
  struct Context *context;
  struct Flags *flags;
  struct Input *next;
  struct Keyword *keyword;
  /*struct ValidationInput *validation;*/
};
