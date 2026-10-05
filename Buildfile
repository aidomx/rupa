use alias
# ============================================
# Buildfile — Bahasa Rupa (Ultimate Version)
# ============================================

# --- Aliases (self-documenting) ---
clean as c              # c = clean
library as lib          # lib = library
embedded as e           # e = embedded
output as o             # o = output
e.modules as mod        # mod = embedded.modules
mod.archive as archive  # archive = embedded.modules.archive

# --- Config ---
root = .

c.build = false
c.compdb = true

sources = src
headers = include, I.
exclude = main.c

flags = Wall, Wextra, O2, MMD, MP
std = gnu11
compiler = gcc, clang

lib.default = ssl, crypto
lib.linux = m, pthread
lib.macos = m
lib.windows = ws2_32

progress.bar = true
progress.error = always

# Embedded modules (pakai alias "mod")
mod.src = stdlib
mod.extract = /tmp/rupa-system
mod.pattern = .rp

# Archive (pakai alias "archive" yang di-chain dari "mod.archive")
archive.dir = modules
archive.name = rupa_modules
archive.with.tar = true
archive.with.ext = gz

# Output (pakai alias "o")
o.binaryName = rupa
o.binaryDir = bin
o.buildDir = build
o.compileCommands = auto
o.libraryName = rupa
o.libDir = lib
o.libraryShared = true
