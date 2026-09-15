#pragma once

#if defined(RUPA_PACKAGE_H)

struct Atom {
  const char *content;
  // position cursor
  int cursor;
  // depth for nested
  int depth;
  int depth_brace;
  int depth_bracket;
  int depth_paren;
  // mengatur apakah ada char selanjutnya
  bool has_next;
  // ex: 1.0 next = 0
  // fungsinya untuk melihat char setelahnya
  char next;
  // char adalah number
  bool number;
  // ex: 1.0 prev = 1
  // fungsinya untuk melihat char sebelumnya
  char prev;
  // char adalah " or '
  bool quote;
  // char adalah string
  bool string;
  // char adalah underscore
  bool underscore;
};

#endif
