# Enum Grammar

Grammar enum membentuk node `Enum Declaration` dari nama enum dan block member.
Member dibangun eksplisit oleh grammar (bukan statement acak): setiap member
adalah `Annotation` (Name, Type opsional, Value opsional) atau `Identifier`
bare untuk member tanpa nilai.

## Enum dengan member auto-increment

Source:

```rupa
enum Color {
  RED
  GREEN
}
```

AST:

```text
Program:
  Enum Declaration:
    Identifier: Color
    Block:
      Identifier: RED
      Identifier: GREEN
```

Member tanpa nilai disimpan sebagai `Identifier` bare; konstanta numeriknya
(0, 1, 2, ...) ditentukan saat runtime, bukan saat parsing.

## Member dengan nilai eksplisit

Source:

```rupa
enum Color {
  RED
  GREEN = 5
  BLUE
}
```

AST:

```text
Program:
  Enum Declaration:
    Identifier: Color
    Block:
      Identifier: RED
      Annotation:
        Name:
          Identifier: GREEN
        Value:
          Number: 5
      Identifier: BLUE
```

## Member bertipe

Source:

```rupa
enum TokenType {
  IDENTIFIER: string = "id"
  ASSIGN = 0
}
```

AST:

```text
Program:
  Enum Declaration:
    Identifier: TokenType
    Block:
      Annotation:
        Name:
          Identifier: IDENTIFIER
        Type:
          Identifier: string
        Value:
          String: "id"
      Annotation:
        Name:
          Identifier: ASSIGN
        Value:
          Number: 0
```

## Bentuk satu baris

Body boleh ditulis dalam satu baris; pemisah antar member adalah whitespace
(newline), koma, atau titik koma:

```rupa
enum Color { RED GREEN = 5 BLUE }
```

Bentuk AST-nya sama dengan versi multiline.

## Runtime binding

Saat eksekusi (jalur interpreter dan IR trampoline sama):

- Konstanta member di-bind ke environment (`RED`, `GREEN`, ...).
- Nama enum di-bind sebagai object berisi seluruh member sehingga
  `Color.RED` terbaca lewat member access.
- Nilai eksplisit dievaluasi sebagai expression biasa; counter auto-increment
  berlanjut dari angka sebelumnya + 1 dan kembali ke 0 setelah member non-number.
