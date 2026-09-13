# Crypto Module Grammar

## AST Structure

### `crypto.sha256(str)`

```text
Call:
  Callee:
    Member:
      Object:
        Identifier: crypto
      Member:
        Identifier: sha256
  Arg 1:
    String: "hello"
```

## Module Structure

```text
crypto
├── md5(str)            — MD5 (checksum)
├── sha1(str)           — SHA-1 (checksum)
├── sha256(str)         — SHA-256
├── sha512(str)         — SHA-512
├── hmac(key, message)  — HMAC-SHA256
├── base64Encode(str)   — Base64 encode
└── base64Decode(str)   — Base64 decode
```

## Implementation

Native module (`src/stdlib/crypto.c`, linked against OpenSSL EVP API):

- `md5`, `sha1`, `sha256`, `sha512` → satu jalur generik `evpHash(algo, input)`.
- `hmac` → `HMAC(EVP_sha256(), ...)`.
- `base64Encode`/`base64Decode` → encoder/decoder Base64 standar.
- Semua hash dan HMAC mengembalikan string hex lowercase.

Registry dibangun di `stdCryptoInit` via `valueObjectSet` pada satu object modul.
