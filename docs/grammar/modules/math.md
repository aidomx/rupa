# Math Module Grammar

Module `math` diimpor sebagai object module dari root namespace Rupa:

```rupa
import math from rupa
```

Untuk syntax ini, identifier setelah `import` adalah nama package yang dicari
oleh module loader. Resolver memprioritaskan archive project, archive global,
lalu archive system yang tertanam di binary. `tests/modules` hanya digunakan
sebagai source pengembangan sebelum archive system dibuat.

Member dari package Rupa:

```text
math.add(a, b)
math.sub(a, b)
math.mul(a, b)
math.div(a, b)
math.mod(a, b)
math.clamp(value, min, max)
math.lerp(a, b, t)
math.isEven(value)
math.isOdd(value)
math.factorial(value)
```

Archive system dibuat dari source Rupa di `tests/modules/math/` dan dikemas
sebagai `modules/rupa_modules.tar.gz` sebelum binary dibangun.
