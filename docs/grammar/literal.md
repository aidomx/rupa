# Literal and Atomic Nodes

Literal dan atom menjadi leaf node untuk grammar expression.

Test saat ini mencakup:

```rupa
0
1
10
100

1.0
3.14
0.5

true
false
null

"hello"
'hello'
```

AST menggunakan node seperti:

```text
Number
Decimal
Boolean
Nullable
String
Identifier / Literal ID
```

Ketika literal berdiri sebagai statement, grammar expression statement membungkus
hasil expression ke dalam `Return` node.
