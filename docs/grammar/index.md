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

## Navigasi

### Dasar
- [Grammar](grammar.md) - Overview grammar
- [Literal](literal.md) - Nilai konstan
- [Expression](expression.md) - Kombinasi value dan operator
- [Assignment](assignment.md) - Assignment grammar
- [Update](update.md) - Update operator
- [Fallback](fallback.md) - Conditional assignment

### Data
- [Array](array.md) - Array grammar
- [Object](object.md) - Object grammar
- [Struct](struct.md) - Struct grammar
- [Member](member.md) - Member access

### Function
- [Function](function.md) - Function grammar
- [Call](call.md) - Function call
- [Return](return.md) - Return statement

### Control Flow
- [If](if.md) - If statement
- [Case](case.md) - Pattern matching
- [Loop](loop.md) - Loop statement
- [Control](control.md) - Break/continue
- [Block](block.md) - Block statement

### Asynchronous
- [Async](async.md) - Async statement
- [Then](then.md) - Then expression

### Program
- [Module](module.md) - Module import
- [Annotation](annotation.md) - Type annotation
- [Print](print.md) - Print statement
