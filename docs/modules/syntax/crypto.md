# Crypto Module

Module `crypto` menyediakan fungsi hashing dan encoding berbasis OpenSSL.

```rupa
import crypto from rupa
```

## Fungsi

### crypto.md5(str)
Hash string menggunakan MD5 (128-bit). Hanya untuk checksum, bukan keamanan.

```rupa
print(crypto.md5("hello"))
```

### crypto.sha1(str)
Hash string menggunakan SHA-1 (160-bit).

```rupa
print(crypto.sha1("hello"))
```

### crypto.sha256(str)
Hash string menggunakan SHA-256 (256-bit).

```rupa
print(crypto.sha256("hello"))
```

### crypto.sha512(str)
Hash string menggunakan SHA-512 (512-bit).

```rupa
print(crypto.sha512("hello"))
```

### crypto.hmac(key, message)
HMAC-SHA256: message authentication code dari `message` dengan `key`.

```rupa
print(crypto.hmac("secret-key", "hello"))
```

### crypto.base64Encode(str)
Encode string ke Base64.

```rupa
print(crypto.base64Encode("Hello Rupa!"))
```

### crypto.base64Decode(str)
Decode Base64 ke string.

```rupa
print(crypto.base64Decode(crypto.base64Encode("Hello Rupa!")))
```

## Ringkasan

| Fungsi | Parameter | Return |
|--------|-----------|--------|
| `md5(str)` | string | string hex |
| `sha1(str)` | string | string hex |
| `sha256(str)` | string | string hex |
| `sha512(str)` | string | string hex |
| `hmac(key, message)` | dua string | string hex |
| `base64Encode(str)` | string | string |
| `base64Decode(str)` | string | string |

## Catatan

- Semua hash mengembalikan string hex lowercase.
- `md5`/`sha1` hanya untuk checksum kompatibilitas — untuk kebutuhan baru gunakan `sha256`/`sha512`.
- `hmac` memakai SHA-256.
