#ifndef RUPA_CTYPES_H
#define RUPA_CTYPES_H

// Grup karakter
#define isarray(c) ((c) == '[' || (c) == ']')
#define isopenarray(c) ((c) == '[')
#define isclosearray(c) ((c) == ']')
#define isassign(c) ((c) == '=')
#define isbacktick(c) ((c) == '`')
#define isblock(c) ((c) == '{' || (c) == '}')
#define iscolon(c) ((c) == ':')
#define iscomma(c) ((c) == ',')
#define isdollar(c) ((c) == '$')
#define isdot(c) ((c) == '.')
#define isfn(c) ((c) == '(' || (c) == ')')
#define ishash(c) ((c) == '#')
#define isint(c) ((c) >= '0' && (c) <= '9')
#define isminus(c) ((c) == '-')
#define ismodules(c) ((c) == '%')
#define ismultiply(c) ((c) == '*')
#define isnewline(c) ((c) == '\n' || (c) == '\r' || (c) == '\x1F')
#define isparen(c) ((c) == '(' || (c) == ')')
#define isplus(c) ((c) == '+')
#define isquote(c) ((c) == '"' || (c) == '\'')
#define issemicolon(c) ((c) == ';')
#define isslash(c) ((c) == '/')
#define isunderscore(c) ((c) == '_')
#define iswhitespace(c) ((c) == ' ' || (c) == '\t')
#define isstr(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))

// Operator umum
#define isoperator(c)                                                          \
  ((c) == '+' || (c) == '-' || (c) == '/' || (c) == '*' || (c) == '%' ||       \
   (c) == '!' || (c) == '<' || (c) == '>' || (c) == '&' || (c) == '|' ||       \
   (c) == '^' || (c) == '~' || (c) == '?')

// Simbol yang diperbolehkan di awal identifier
#define issymallow(c) (isunderscore(c))

// Simbol yang dilarang di awal identifier
#define issymdenied(c)                                                         \
  (isassign(c) || isarray(c) || isblock(c) || isdollar(c) || ishash(c) ||      \
   isparen(c) || isoperator(c) || isdot(c) || iscomma(c) || iscolon(c) ||      \
   issemicolon(c) || isslash(c) || isquote(c) || isbacktick(c) ||              \
   (c) == '\\' || (c) == '@')

// Semua simbol umum (gabungan semua is* kecuali issymallow dan issymdenied)
#define issymbol(c)                                                            \
  (isarray(c) || isassign(c) || isbacktick(c) || isblock(c) || iscomma(c) ||   \
   isdot(c) || ishash(c) || isminus(c) || ismodules(c) || ismultiply(c) ||     \
   isparen(c) || isplus(c) || isquote(c) || issemicolon(c) || isslash(c) ||    \
   isoperator(c))

#define issymvalue(c)                                                          \
  (isarray(c) || isblock(c) || isparen(c) || isoperator(c) || iscomma(c))

#endif
