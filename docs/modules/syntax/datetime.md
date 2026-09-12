# DateTime Module

Module `datetime` menyediakan fungsi untuk manipulasi tanggal dan waktu.

```rupa
import datetime from rupa
```

## Fungsi

### datetime.now()
Mengembalikan Unix timestamp saat ini (detik).

```rupa
ts = datetime.now()
print(ts)
```

### datetime.nowMs()
Mengembalikan Unix timestamp dalam milidetik.

```rupa
ms = datetime.nowMs()
```

### datetime.format(timestamp, fmt?)
Format timestamp ke string. Default: `"YYYY-MM-DD HH:MM:SS"`.

```rupa
ts = datetime.now()
print(datetime.format(ts))
print(datetime.format(ts, "%d/%m/%Y %H:%M"))
```

| Format | Arti |
|--------|------|
| `%Y` | Tahun (4 digit) |
| `%m` | Bulan (01-12) |
| `%d` | Hari (01-31) |
| `%H` | Jam (00-23) |
| `%M` | Menit (00-59) |
| `%S` | Detik (00-59) |

### datetime.parse(str, fmt?)
Parse string tanggal ke timestamp.

```rupa
ts = datetime.parse("2026-01-15 10:30:00")
```

### datetime.diff(ts1, ts2)
Selisih dua timestamp dalam detik.

```rupa
diff = datetime.diff(ts1, ts2)
```

### datetime.add(timestamp, seconds)
Tambah detik ke timestamp.

```rupa
future = datetime.add(ts, 3600)  // +1 jam
```

### datetime.year(ts?), datetime.month(ts?), datetime.day(ts?)
### datetime.hour(ts?), datetime.minute(ts?), datetime.second(ts?)
Ekstrak komponen dari timestamp.

```rupa
ts = datetime.now()
print(datetime.year(ts))
print(datetime.month(ts))
print(datetime.day(ts))
```

| Fungsi | Parameter | Return |
|--------|-----------|--------|
| `now()` | - | number |
| `nowMs()` | - | number |
| `format(ts, fmt?)` | timestamp, string? | string |
| `parse(str, fmt?)` | string, string? | number |
| `diff(ts1, ts2)` | dua timestamp | number |
| `add(ts, seconds)` | timestamp, number | number |
| `year(ts?)` | timestamp? | number |
| `month(ts?)` | timestamp? | number |
| `day(ts?)` | timestamp? | number |
| `hour(ts?)` | timestamp? | number |
| `minute(ts?)` | timestamp? | number |
| `second(ts?)` | timestamp? | number |
