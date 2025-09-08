# Instruction

## Dependencies

Sebelum menggunakan Rupa, pastikan paket berikut terpasang:

- **GCC** → compiler utama
- **Make** → untuk build otomatis
- **Bear** → menghasilkan `compile_commands.json`
- **Clangd** (>=21) → language server

## Cara Install (Linux/Ubuntu)

```bash
sudo apt update
sudo apt install gcc make bear clangd-21
```

## Cara Install (Arch Linux)

```bash
sudo pacman -S gcc make bear clang
```

## Cara Install (MacOS, via Homebrew)

```bash
brew install gcc make bear llvm
```
