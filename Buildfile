root: .

clean:
  - buildDir: false
  - compileCommands: true

sources:
  - src

flags:
  - Wall
  - Wextra
  - O2

std: gnu11

headers:
  - include
  - I.

library:
  - ssl
  - crypto
  
  - linux:
    - m
    - pthread

  - macos:
    - m

  - windows:
    - ws2_32

compiler:
  - gcc
  - clang

progress:
  bar: true
  error: always

embedded:
  - modules:
    - src: stdlib
    - extract: /tmp/rupa-system
    - pattern: .rp
    - archive:
      - dir: modules
      - name: rupa_modules
      - with:
        - tar: true
        - ext: gz

output:
  - binaryName: rupa
  - binaryDir: bin
  - buildDir: build
  - compileCommands: auto # compile_commands.json
