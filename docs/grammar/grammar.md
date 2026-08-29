# Rupa Grammar

Dokumentasi ini menjelaskan grammar Rupa dari sudut pandang parser dan AST.

`docs/syntax.md` menjelaskan cara memakai syntax. Sebaliknya, direktori ini
menjelaskan bagaimana bentuk source yang sudah dikenali parser direpresentasikan
sebagai node dan hubungan antar-node.

Alur umum:

```text
source
  ↓
token
  ↓
grammarParseStatement()
  ↓
grammarParseExpr()
  ↓
AST node
```

## Status dokumentasi

Dokumentasi ini mengikuti source dan test yang tersedia saat ini. Bentuk yang
belum diuji atau belum direpresentasikan oleh AST tidak dianggap sebagai grammar
yang sudah dikunci.

## Peta grammar

```text
statement
├── annotation
├── assignment
│   ├── normal assignment
│   └── conditional assignment
├── function / call
├── struct
├── if / elseif / else
├── loop
├── print
├── return
├── break / continue
├── module statement
├── update
└── expression statement

expression
├── literal / identifier
├── binary expression
├── then
├── fallback
├── array literal
├── object literal
├── call expression
└── member expression
```

`?=`, `->`, dan `|` tetap dipandang sebagai grammar yang berbeda. Mereka dapat
muncul dalam satu AST karena grammar expression dapat dikomposisikan.
