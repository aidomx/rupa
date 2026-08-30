# Array

## Apa yang bisa ditulis?

```rupa
[]
[1, 2, 3]
[1 + 2, 3 * 4]
[1, [2, 3], 4]
```

## Kapan digunakan?

Gunakan array untuk menyimpan koleksi data urutan. Akses element menggunakan subscript notation `[index]`.

## Apa hasilnya?

Array dibuat dengan `[ ]` dan element dipisahkan koma. Index dimulai dari 0.

### Contoh execution

```rupa
arr = [1, 2, 3]
print(arr)
print(arr[0])
print(arr[2])
```

Output:
```
[1, 2, 3]
1
3
```

```rupa
nested = [1, [2, 3], 4]
print(nested)
print(nested[1])
```

Output:
```
[1, [2, 3], 4]
[2, 3]
```

```rupa
mixed = [1 + 2, 3 * 4, "hello"]
print(mixed)
```

Output:
```
[3, 12, hello]
```
