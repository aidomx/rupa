# Built-in Functions

Fungsi bawaan yang tersedia tanpa import.

## type(value)

Mengembalikan tipe data sebagai string.

```rupa
print(type(5))        // number
print(type(3.14))     // decimal
print(type("hello"))  // string
print(type(true))     // boolean
print(type(null))     // null
print(type([1,2]))    // array
print(type({a: 1}))   // object
print(type(fn() {}))  // function
```

| Tipe | Return |
|------|--------|
| `5` | `"number"` |
| `3.14` | `"decimal"` |
| `"hello"` | `"string"` |
| `true` | `"boolean"` |
| `null` | `"null"` |
| `[1,2]` | `"array"` |
| `{a: 1}` | `"object"` |
| `fn() {}` | `"function"` |

## len(value)

Mengembalikan panjang string, array, atau jumlah key object.

```rupa
print(len("hello"))     // 5
print(len([1, 2, 3]))   // 3
print(len({a: 1, b: 2})) // 2
```

## isNull(value)

Mengembalikan `true` jika value null.

```rupa
print(isNull(null))    // true
print(isNull(0))       // false
print(isNull(""))      // false
```

## toNumber(value)

Konversi value ke number.

```rupa
print(toNumber("42"))    // 42
print(toNumber("3.14"))  // 3
print(toNumber(true))    // 1
print(toNumber(null))    // 0
```

## toString(value)

Konversi value ke string.

```rupa
print(toString(42))     // "42"
print(toString(true))   // "true"
print(toString(null))   // "null"
```
