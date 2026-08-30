# Loop

## Apa yang bisa ditulis?

```rupa
i = 10
for i: print(i)
```

With condition:

```rupa
for i < 10: print(i)
```

With block:

```rupa
for i < 10 {
    print(i)
}
```

Reverse loop:

```rupa
rev i > 0 {
    print(i)
}
```

While loop:

```rupa
x = 0
while x < 10 {
    print(x)
    x++
}
```

## Kapan digunakan?

Gunakan `for` untuk iterasi maju, `rev` untuk iterasi mundur, dan `while` untuk iterasi berdasarkan condition.

## Apa hasilnya?

Loop menjalankan body berulang kali sampai condition tidak terpenuhi. Gunakan `break` untuk keluar dan `continue` untuk skip iterasi.

### Contoh execution

```rupa
for i < 5 {
    print(i)
}
```

Output:
```
0
1
2
3
4
```

```rupa
x = 0
while x < 5 {
    print(x)
    x++
}
```

Output:
```
0
1
2
3
4
```

```rupa
for i < 10 {
    if i == 3: continue
    if i == 7: break
    print(i)
}
```

Output:
```
0
1
2
4
5
6
```
