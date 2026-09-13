# Array

## Apa yang bisa ditulis?

```rupa
[]
[1, 2, 3]
[1 + 2, 3 * 4]
[1, [2, 3], 4]
```

## Typed array

Array dapat diberi type annotation dengan menambahkan `[]` setelah type element:

```rupa
x: number[] = [1, 2, 3]
names: string[] = ["rupa", "language"]
empty: number[] = []
```

Array kosong boleh digunakan sebagai nilai awal typed array karena tidak memiliki element yang perlu diperiksa. Untuk array berisi, semua element harus sesuai dengan type yang dideklarasikan. Contoh berikut menghasilkan error:

```rupa
x: number[] = [1, "a"]
```

Array bertingkat dapat ditulis dengan beberapa `[]`:

```rupa
matrix: number[][] = [[1, 2], [3, 4]]
```

## Method array

Method bawaan array sengaja dibuat minimal — hanya operasi mutasi dasar yang hidup di core. Transformasi seperti `map`, `filter`, `reduce` tetap berupa fungsi di modul `collections`, bukan method, supaya bahasa tidak terasa seperti JS.

### push

Menambah element ke akhir array (in-place). Mengembalikan array itu sendiri.

```rupa
arr = [1, 2]
arr.push(3)
print(arr)
// [1, 2, 3]

arr.push(4, 5) // beberapa nilai sekaligus
```

### pop

Mengambil element terakhir (in-place). Mengembalikan element yang diambil, atau `null` untuk array kosong.

```rupa
arr = [1, 2, 3]
x = arr.pop()
print(x)
// 3
print(arr)
// [1, 2]
```

### length

Property (bukan method) jumlah element:

```rupa
print(arr.length)
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
