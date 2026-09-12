# Crypto Module

Module `crypto` menyediakan fungsi hashing dan encoding.

```rupa
import crypto from rupa
```

## Fungsi

### crypto.hash(str)
Hash string menggunakan DJB2 (non-cryptographic, sangat cepat).

```rupa
print(crypto.hash("hello"))  // 310f923099
```

### crypto.fnv1a(str)
Hash string menggunakan FNV-1a.

```rupa
print(crypto.fnv1a("hello"))  // a430d84680aabd0b
```

### crypto.murmur3(str, seed?)
Hash string menggunakan MurmurHash3.

```rupa
print(crypto.murmur3("hello"))     // 248bfa47
print(crypto.murmur3("hello", 42)) // hash dengan seed
```

### crypto.xor(str, key)
XOR cipher — encrypt/decrypt dengan key yang sama.

```rupa
encrypted = crypto.xor("secret", "key")
decrypted = crypto.xor(encrypted, "key")
print(decrypted)  // secret
```

### crypto.base64Encode(str)
Encode string ke Base64.

```rupa
encoded = crypto.base64Encode("Hello Rupa!")
print(encoded)  // SGVsbG8gUnVwYSEA
```

### crypto.base64Decode(str)
Decode Base64 ke string.

```rupa
print(crypto.base64Decode("SGVsbG8gUnVwYSEA"))  // Hello Rupa!
```

| Fungsi | Parameter | Return |
|--------|-----------|--------|
| `hash(str)` | string | string |
| `fnv1a(str)` | string | string |
| `murmur3(str, seed?)` | string, number? | string |
| `xor(str, key)` | dua string | string |
| `base64Encode(str)` | string | string |
| `base64Decode(str)` | string | string |

## Catatan

- `hash`, `fnv1a`, `murmur3` adalah hash non-cryptographic untuk dedup/checksum.
- `xor` adalah XOR cipher sederhana — untuk enkripsi ringan, bukan keamanan.
- `base64Encode`/`base64Decode` untuk encoding data.
