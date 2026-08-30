# Control

## Apa yang bisa ditulis?

```rupa
break
continue
```

## Kapan digunakan?

Gunakan `break` untuk keluar dari loop. Gunakan `continue` untuk skip iterasi saat ini.

## Apa hasilnya?

`break` menghentikan loop dan melanjutkan ke statement berikutnya. `continue` melompat ke iterasi berikutnya.

### Contoh execution

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
