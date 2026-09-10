# IO Module Grammar

Module `io` diekspos sebagai object module melalui deklarasi import:

```rupa
import io from rupa
```

Member module yang tersedia:

```text
io.input(prompt?)
io.toNumber(value)
```

| Member | Arity | Return |
|---|---:|---|
| `input` | 0–1 | string/null |
| `toNumber` | 1 | number/decimal/null |

Parser membentuk akses `io.input` dan `io.toNumber` sebagai member expression,
kemudian pemanggilannya menghasilkan call expression.
