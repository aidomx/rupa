# Thread Module

Modul `thread` menyediakan fungsi untuk menjalankan kode secara paralel menggunakan system thread.

```rupa
import thread from rupa
```

## `thread.sleep(ms)`

Menghentikan thread selama beberapa milidetik.

```rupa
import thread from rupa

print("Start")
thread.sleep(1000)
print("After 1 second")
```

| Parameter | Type | Deskripsi |
|-----------|------|-----------|
| `ms` | number | Milidetik yang harus ditunggu |

## `thread.id()`

Mengembalikan ID dari thread yang sedang berjalan.

```rupa
import thread from rupa

id = thread.id()
print("Thread ID: " + id)
```

| Return | Type | Deskripsi |
|--------|------|-----------|
| - | number | Hash dari thread ID saat ini |

## `thread.count()`

Mengembalikan jumlah thread yang masih aktif (belum selesai).

```rupa
import thread from rupa

print("Active threads: " + thread.count())
```

| Return | Type | Deskripsi |
|--------|------|-----------|
| - | number | Jumlah thread aktif |

## `thread.create(fn)`

Membuat thread baru yang menjalankan fungsi `fn`. Mengembalikan handle thread.

```rupa
import thread from rupa

myTask() {
  print("Running in background\n")
  thread.sleep(500)
  print("Done!\n")
}

handle = thread.create(myTask)
thread.join(handle)
print("Thread finished\n")
```

| Parameter | Type | Deskripsi |
|-----------|------|-----------|
| `fn` | function | Fungsi native yang akan dijalankan di thread baru |

| Return | Type | Deskripsi |
|--------|------|-----------|
| - | number | Handle thread (untuk digunakan dengan `thread.join`) |

## `thread.join(handle)`

Menunggu thread selesai dan mengembalikan hasilnya.

```rupa
import thread from rupa

compute() {
  return 42
}

handle = thread.create(compute)
result = thread.join(handle)
print("Result: " + result + "\n")
```

| Parameter | Type | Deskripsi |
|-----------|------|-----------|
| `handle` | number | Handle dari `thread.create()` |

| Return | Type | Deskripsi |
|--------|------|-----------|
| - | any | Nilai yang dikembalikan oleh fungsi thread |

## Ringkasan

| Fungsi | Parameter | Return | Deskripsi |
|--------|-----------|--------|-----------|
| `sleep(ms)` | number | - | Tidur selama ms milidetik |
| `id()` | - | number | ID thread saat ini |
| `count()` | - | number | Jumlah thread aktif |
| `create(fn)` | function | number | Buat thread baru |
| `join(handle)` | number | any | Tunggu thread selesai |

## Contoh: Parallel Processing

```rupa
import thread from rupa

taskA() {
  thread.sleep(500)
  return "Task A done"
}

taskB() {
  thread.sleep(300)
  return "Task B done"
}

a = thread.create(taskA)
b = thread.create(taskB)

resultA = thread.join(a)
resultB = thread.join(b)

print(resultA + "\n")
print(resultB + "\n")
print("Both tasks completed\n")
```
