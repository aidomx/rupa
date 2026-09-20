root: .

clean:
  - buildDir: false
  - compileCommands: false

sources:
  - src

flags:
  - Wall
  - Wextra

std: gnu11

headers:
  - include
  - I.

library:
  - ssl
  - crypto
  - lm
  - pthread

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
