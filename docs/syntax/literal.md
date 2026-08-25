# Literal

## Apa yang bisa ditulis?

Number:

```rupa
0
1
100
```

Decimal:

```rupa
1.0
3.14
0.5
```

Boolean dan null:

```rupa
true
false
null
```

String:

```rupa
"hello"
'hello'
```

## Kapan digunakan?

Gunakan literal ketika value ditulis langsung di source tanpa mengambilnya dari identifier atau hasil expression lain.

## Apa hasilnya?

Source menghasilkan value dasar seperti number, decimal, boolean, null, atau string. Decimal tidak langsung menentukan `float` atau `double`; precision dapat ditentukan kemudian melalui annotation atau inference.
