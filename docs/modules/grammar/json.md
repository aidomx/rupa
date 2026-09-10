# JSON Module Grammar

Module `json` digunakan melalui import:

```rupa
import json from rupa
```

Member yang dikenali runtime:

```text
json.parse(text)
json.stringify(value)
```

Kedua bentuk tersebut diparse sebagai `member expression` yang menjadi callee
pada `call expression`.

| Member | Arity | Return |
|---|---:|---|
| `parse` | 1 | nilai Rupa |
| `stringify` | 1 | string |
