# JSON

Modul `json` menyediakan konversi antara nilai Rupa dan teks JSON.

```rupa
import json from rupa
```

## `json.parse(text)`

Mengubah teks JSON menjadi nilai Rupa. Nilai yang didukung meliputi `null`, boolean, number, decimal, string, array, dan object bersarang.

```rupa
data = json.parse("{\"name\":\"Rupa\",\"version\":1}")
print(data.name)
print(data.version)
```

JSON tidak valid menghasilkan error.

## `json.stringify(value)`

Mengubah nilai Rupa menjadi teks JSON.

```rupa
data = {
    name: "Rupa",
    active: true,
    tags: ["web", "cli"]
}

text = json.stringify(data)
print(text)
```

String akan di-escape sesuai format JSON. Array dan object dapat berisi nilai bersarang.

## Ringkasan

| Fungsi | Parameter | Return |
|---|---|---|
| `parse(text)` | string JSON | nilai Rupa |
| `stringify(value)` | nilai Rupa | string JSON |
