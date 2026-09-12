# Crypto Module Grammar

## AST Structure

### `crypto.hash(str)`

```text
Call:
  Callee:
    Member:
      Object:
        Identifier: crypto
      Member:
        Identifier: hash
  Arg 1:
    String: "hello"
```

## Module Structure

```text
crypto
├── hash(str)           — DJB2 hash
├── fnv1a(str)          — FNV-1a hash
├── murmur3(str, seed?) — MurmurHash3
├── xor(str, key)       — XOR cipher
├── base64Encode(str)   — Base64 encode
└── base64Decode(str)   — Base64 decode
```
