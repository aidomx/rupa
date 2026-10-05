use workspace

# alias singkat
projects.rupamod as mod
projects.core as core

projects = bade, core, rupamod

compiler = gcc, clang
flags = Wall, Wextra, MMD, MP
headers = include, build, I.
library.default = ssl, crypto
library.linux = m, pthread
library.macos = m
library.windows = ws2_32
std = gnu11

# B. rupamod — proyek kemasan (archive)
mod.output.binary = false
mod.archive.src = .
mod.archive.name = rupa_modules
mod.archive.with = tar, gz
mod.archive.exclude = build, dist, LICENSE, README.md, self_archive
mod.archive.dir = ../bade/modules

# A. Core engine
core.sources = src
core.exclude = main.c
core.headers = ../bade/include

core.output as coreout
core.library as corelib

coreout.binaryName = rupa
coreout.libraryName = core
coreout.libDir = lib
coreout.libraryShared = true

# C. bade — embed arsip jadi dari rupamod
bade.sources = manager, src
bade.exclude = manager/main.c
bade.output as badeout
bade.library as badelib

badeout.binaryName = bade
badeout.libraryName = bade
badeout.libDir = lib
badeout.libraryShared = true

bade.embedded.modules as bademod

bade.depends_on = rupamod

bademod.file = modules/rupa_modules.tar.gz
bademod.variable = MODULES
bademod.extract = /tmp/rupa-system
bademod.pattern = .rp
bade.headers = ../core/include

# core dan bade saling terikat lewat library (rupa <-> bade). Workspace
# rbot menanganinya dengan DUA FASE: fase 1 (library pass) tiap proyek
# mengemas lib<name>.a/.so tanpa link binary; fase 2 (binary pass) baru
# link bin/rupa dan bin/bade setelah SEMUA library tersedia. Pada link
# .so di fase library, artifact proyek lain otomatis dilewatkan.
badelib.linux = core
corelib.linux = bade

# Packaging
core.pack.files = bin/rupa:bin/rupa, src/prompt/cmd.txt:share/rupa/cmd.txt

core.pack.name = rupa
core.pack.version = 0.2.2
core.pack.output = dist/{name}-v{version}.tar.gz
core.pack.format = deb
core.pack.checksum = sha256
core.pack.deb.install_prefix = /usr/local
core.pack.merge = bade

bade.pack.files = bin/bade:bin/bade
bade.pack.name = bade
bade.pack.version = 0.2.2
bade.pack.output = dist/{name}-v{version}.tar.gz
bade.pack.format = deb
bade.pack.checksum = sha256
bade.pack.deb.install_prefix = /usr/local
