# String

Modul `string` menyediakan operasi string dalam bentuk fungsi. String juga mendukung akses method langsung seperti `name.upper()`; lihat [String Methods](../string.md).

```rupa
import string from rupa
```

## Fungsi

```rupa
name = " Rupa "

print(string.length(name))
print(string.upper(name))
print(string.lower(name))
print(string.trim(name))
print(string.contains(name, "up"))
print(string.startsWith(name, " Ru"))
print(string.endsWith(name, "pa "))
print(string.replace(name, "Rupa", "Rupa Language"))
```

| Fungsi | Parameter | Return |
|---|---|---|
| `length(value)` | string | number |
| `upper(value)` | string | string |
| `lower(value)` | string | string |
| `trim(value)` | string | string |
| `contains(value, search)` | dua string | boolean |
| `startsWith(value, prefix)` | dua string | boolean |
| `endsWith(value, suffix)` | dua string | boolean |
| `replace(value, old, new)` | tiga string | string |

## Method langsung

API module di atas juga tersedia melalui receiver string:

```rupa
name = " Rupa "

print(name.length)
print(name.trim().upper())
print(name.contains("up"))
print(name.replace("Rupa", "Rupa Language"))
```
