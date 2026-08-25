# Case Grammar

## Definition

`case` adalah construct untuk memilih satu aksi berdasarkan hasil pencocokan sebuah expression dengan daftar pattern.

```rupa
case username => {
    "budi": print(true)
    "arman": print(true)
    *: print(false)
}
```

`case` tidak menggunakan fallthrough. Setelah satu pattern cocok dan body selesai dijalankan, evaluasi `case` selesai.

## Grammar Form

```text
case-statement
    = "case" expression "=>" case-block

case-block
    = "{" case-entry* "}"

case-entry
    = case-pattern ":" case-body

case-pattern
    = expression
    | "*"

case-body
    = statement
    | expression
    | block
```

## Components

### Subject

Subject adalah expression setelah `case`.

```rupa
case username => {
    "budi": print(true)
}
```

Subject dievaluasi satu kali sebelum pattern diperiksa.

### Pattern

Pattern berada di sisi kiri `:`.

```rupa
case username => {
    "budi": print(true)
    "arman": print(true)
}
```

Pattern diperiksa sesuai urutan source.

### Body

Body berada di sisi kanan `:`.

Body dapat berupa aksi singkat:

```rupa
"budi": print(true)
```

atau block:

```rupa
"budi": {
    print(true)
    save()
}
```

## Wildcard

`*` adalah wildcard pattern untuk nilai yang tidak cocok dengan pattern sebelumnya.

```rupa
case username => {
    "budi": print("Budi")
    *: print("Unknown")
}
```

Wildcard harus menjadi entry terakhir. Entry setelah wildcard tidak valid karena tidak dapat dicapai.

## Parse Structure

```text
CaseStatement
├── subject
└── entries
    ├── CaseEntry
    │   ├── pattern
    │   └── body
    ├── CaseEntry
    │   ├── pattern
    │   └── body
    └── CaseEntry
        ├── wildcard
        └── body
```

Nama node internal dapat berbeda. Struktur pentingnya adalah subject dan entries yang mempertahankan urutan source.

## Evaluation

```text
1. Evaluate subject.
2. Read entries from top to bottom.
3. Evaluate pattern.
4. Compare pattern with subject.
5. Execute body of the first matching entry.
6. Finish case.
```

```text
Case
 │
 ├── evaluate subject
 │
 ├── pattern 1 matches?
 │      ├── yes → execute body → finish
 │      └── no
 │
 ├── pattern 2 matches?
 │      ├── yes → execute body → finish
 │      └── no
 │
 └── wildcard exists?
        ├── yes → execute body
        └── no  → finish
```

## No Fallthrough

```rupa
case status => {
    200: print("success")
    404: print("not found")
    *: print("unknown")
}
```

Jika `status` cocok dengan `200`, hanya cabang `200` yang dijalankan.

`break` tidak diperlukan untuk menghentikan perpindahan antar entry.

## Control Flow Context

`case` tidak mengubah arti `break` atau `continue`.

```rupa
for i < users.length {
    case users[i].username => {
        "budi": continue
        "arman": break
        *: print(users[i].username)
    }
}
```

```text
continue
→ loop terdekat

break
→ context breakable terdekat
```

`case` hanya memilih cabang. Validitas `break` dan `continue` tetap ditentukan oleh context luar.

## Parser Responsibilities

Parser perlu mengenali:

```text
case
expression
=>
{
pattern
:
body
...
}
```

Parser perlu:

1. Memisahkan subject dari `case-block`.
2. Membaca entry secara berurutan.
3. Mengenali `*` sebagai wildcard khusus di dalam `case`.
4. Memisahkan pattern dan body menggunakan `:`.
5. Menerima body sebagai statement, expression, atau block.
6. Memastikan wildcard tidak diikuti entry lain.
7. Menjaga context `break` dan `continue` dari scope luar.

## Runtime Contract

Grammar tidak mengikat implementasi pada mekanisme internal tertentu.

Kontrak perilakunya:

```text
case subject
→ evaluate subject once

pattern
→ check in source order

first match
→ execute one body

no match
→ execute wildcard when present

body finished
→ case finished
```
