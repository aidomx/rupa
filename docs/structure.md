# Project Structure

```
rupa-v22/
├── bin/                          # Binary output
│   └── rupa                      # Compiled interpreter
│
├── include/                      # Public API header
│   └── rupa.h                    # #include <rupa> — main entry point
│
├── lib/                          # Headers & type definitions
│   ├── intl.h                    # Central include hub (includes all lib/)
│   ├── forward.h                 # Forward declarations (enum, struct)
│   ├── stdlib.h                  # Standard library C headers
│   │
│   ├── core/                     # Enums, constants, macros
│   │   ├── enum.h                # Master enum include
│   │   ├── enum_binary.h         # Binary operator types
│   │   ├── enum_command.h        # REPL command types
│   │   ├── enum_context.h        # Runtime context types
│   │   ├── enum_debug.h          # Debug mode flags
│   │   ├── enum_editor.h         # Editor mode types
│   │   ├── enum_error.h         # Error code types
│   │   ├── enum_except.h         # Exception types
│   │   ├── enum_flag.h           # Runtime flag types
│   │   ├── enum_interpreter.h    # Interpreter state types
│   │   ├── enum_ir.h             # IR opcode & value types
│   │   ├── enum_keyword.h        # Keyword token types
│   │   ├── enum_mod.h            # Module design baru (ModEntryKind, ModType)
│   │   ├── enum_node.h           # AST node types
│   │   ├── enum_program.h        # Program phase types
│   │   ├── enum_token.h          # Token types (termasuk compound +=, -=, dst)
│   │   ├── enum_value.h          # Runtime value types
│   │   ├── enum_variable.h       # Variable scope types
│   │   ├── keys.h                # Keyboard key definitions
│   │   ├── limit.h               # System limits
│   │   ├── macros.h              # Utility macros
│   │   ├── manifest.h            # Version & build info
│   │   └── platform.h            # Platform detection
│   │
│   ├── compiler/                 # Compiler header interfaces
│   │   ├── compiler.h            # Master include compiler
│   │   ├── interpreter/          # Struct + fn decl interpreter
│   │   │   ├── value.h           # RuntimeValue, RuntimeArray, NativeFn
│   │   │   ├── result.h          # InterpreterResult & flow
│   │   │   ├── function.h        # RuntimeFunction (closure AST)
│   │   │   ├── eval.h            # interpretNode & eval API
│   │   │   ├── text.h            # valueTextOf, valueBinaryApply (dipakai IR)
│   │   │   ├── interpreter.h
│   │   │   ├── runtime.h
│   │   │   ├── annotation.h
│   │   │   ├── debug/            # debug_ast.h, print_ast.h, print_ast_shared.h
│   │   │   ├── error/error.h
│   │   │   └── statement/loop.h
│   │   ├── ir/                   # IR pipeline (AST → IR → Execute)
│   │   │   ├── ir.h              # IRModule, IRInstruction, IRValue
│   │   │   ├── rewrite.h         # rewrite() AST → IR
│   │   │   ├── execute.h         # executeIR() mesin IR
│   │   │   └── debug.h           # IR printer
│   │   ├── lexer/lexer.h         # DataToken, LexerState
│   │   ├── parser/
│   │   │   ├── ast/ast.h         # AstNode & semua struct Ast* (nempel fn decl)
│   │   │   ├── node.h, node_type.h
│   │   │   ├── parser.h
│   │   │   ├── grammar/grammar.h, array/array.h
│   │   │   └── token/token.h
│   │   └── semantic/
│   │       ├── symbol.h          # RuntimeEnv, RuntimeBinding
│   │       ├── analyzer.h        # Registry struct + validasi type (struct-first)
│   │       ├── value.h
│   │       └── eventloop.h       # EventLoop async
│   │
│   ├── formatter/formatter.h     # Formatter struct + API
│   ├── runtime/                  # Runtime header interfaces
│   │   ├── runtime.h
│   │   ├── context/context.h
│   │   ├── gc/gc.h               # GC allocator (gcmall, gcstrdup, ...)
│   │   └── io/io.h
│   │
│   ├── editor/                   # Editor header interfaces
│   │   ├── editor.h, editor_type.h
│   │   ├── core/buffer.h, cursor.h, mode.h
│   │   ├── display/drawer.h, refresh.h, terminal.h
│   │   └── operations/editorState.h, history.h, indent.h, reset.h
│   │
│   ├── state/                    # State header interfaces
│   │   ├── state.h
│   │   ├── types/state.h, input.h, flags.h
│   │   └── input/input.h, flags.h
│   │
│   ├── modules/                  # Module headers (per modul native)
│   │   ├── rupa_modules.h        # Master include modul
│   │   ├── crypto.h, datetime.h, io.h, net.h
│   │   ├── os.h, regex.h, string.h, test_helper.h
│   │
├── prompt/prompt.h           # Prompt header
│   ├── repl/repl.h, repl_type.h  # REPL header
│   ├── debug/debug.h, debug_type.h
│   └── utils/                    # Utility headers
│       ├── utils.h
│       ├── atom.h, identifier.h, numbers.h, strings.h
│       └── types/atom.h, posix.h, symbol.h, support.h, system.h
│
├── src/                          # Implementation (C source)
│   ├── main.c                    # Entry point
│   │
│   ├── bootstrap/                # File loading & bootstrapping
│   │   └── loader.c
│   │
│   ├── formatter/                # Code formatter (modular, top-level)
│   │   ├── formatter.c             # Entry points (formatFile/String/Stdin)
│   │   ├── format_helpers.c        # fmtIndent, fmtStr, fmtChar, fmtNewline
│   │   ├── format_node.c           # Atom nodes
│   │   ├── format_expr.c           # Expressions (incl. compound +=, -=, dst)
│   │   ├── format_stmt.c           # Statements (incl. import/export/namespace)
│   │   ├── format_dispatch.c       # fmtNode main switch
│   │   └── format_comment.c        # Comment formatting
│   │
│   ├── compiler/                 # Compiler pipeline
│   │   ├── lexer/                # Tokenizer
│   │   │   ├── lexer.c, factory.c, operations.c, support.c
│   │   │   └── processor/
│   │   │       ├── processor.c
│   │   │       ├── construct/            # Token construction
│   │   │       │   ├── construct.c       # Main construct processor
│   │   │       │   ├── delimiter.c       # Delimiters (, ; :)
│   │   │       │   ├── identifier.c      # Identifiers
│   │   │       │   ├── literal.c         # Literals (number, string, bool)
│   │   │       │   └── operator.c        # Operators (+, -, =, etc.)
│   │   │       └── keyword/              # Keyword detection
│   │   │           ├── check.c, keyword.c, lists.c
│   │   │
│   │   ├── parser/               # Parser (tokens → AST)
│   │   │   ├── ast/              # AST node creation
│   │   │   │   ├── ast.c, processor.c
│   │   │   │   ├── parse_array.c, parse_atom.c, parse_binary.c
│   │   │   │   ├── parse_expression.c, parse_factor.c
│   │   │   │   ├── parse_statement.c, parse_subscript.c
│   │   │   │
│   │   │   ├── grammar/          # Grammar rules (tokens → AST nodes)
│   │   │   │   ├── grammar.c               # Main dispatcher
│   │   │   │   ├── grammar_shared.c        # Shared utilities
│   │   │   │   ├── grammar_annotation.c    # x: type
│   │   │   │   ├── grammar_assignment.c    # x = expr
│   │   │   │   ├── grammar_async.c         # async handler
│   │   │   │   ├── grammar_block.c         # { ... }
│   │   │   │   ├── grammar_call.c          # fn()
│   │   │   │   ├── grammar_case.c          # case/when
│   │   │   │   ├── grammar_control.c       # break, continue
│   │   │   │   ├── grammar_expression.c    # Expression parser
│   │   │   │   ├── grammar_function.c      # fn() {}
│   │   │   │   ├── grammar_if.c            # if/else
│   │   │   │   ├── grammar_loop.c          # while
│   │   │   │   ├── grammar_module.c        # Module dispatcher
│   │   │   │   ├── grammar_module_export.c # export
│   │   │   │   ├── grammar_module_import.c # import
│   │   │   │   ├── grammar_module_utils.c  # Module utilities
│   │   │   │   ├── grammar_object.c        # { key: value }
│   │   │   │   ├── grammar_postfix.c       # postfix ops
│   │   │   │   ├── grammar_print.c         # print()
│   │   │   │   ├── grammar_return.c        # return
│   │   │   │   ├── grammar_struct.c        # struct
│   │   │   │   ├── grammar_update.c        # ++, -- dan compound +=, -=, *=, /=, %=
│   │   │   │   └── array/array.c           # Array grammar
│   │   │   │
│   │   │   ├── node/             # AST node utilities
│   │   │   │   ├── cleaner.c, create.c, factory.c
│   │   │   │   └── factory_nodes.c  # Statements, module (incl. NODE_MOD factories), async, case
│   │   │   │
│   │   │   ├── token/            # Token utilities
│   │   │   │   ├── check.c, error.c, lookup.c, posix.c
│   │   │   │   ├── save.c, symbol.c, type.c
│   │   │   │
│   │   │   └── parser.h          # Parser entry
│   │   │
│   │   ├── ir/                   # IR pipeline (AST → IR → Execute)
│   │   │   ├── ir.c              # IR builders & module
│   │   │   ├── rewrite.c         # rewrite() AST → IR
│   │   │   ├── execute.c         # executeIR() mesin IR (stack machine)
│   │   │   └── debug.c           # IR printer (--test-ir)
│   │   │
│   │   ├── interpreter/          # AST interpreter
│   │   │   ├── interpreter.c             # Main dispatch
│   │   │   ├── annotation/annotation.c   # Type annotation check
│   │   │   ├── control/result.c          # Flow control (return, break)
│   │   │   ├── debug/                    # AST debug printing
│   │   │   │   ├── debug_ast.c, print_ast.c, print_ast_basic.c
│   │   │   │   ├── print_ast_control.c, print_ast_shared.c
│   │   │   │   └── print_ast_structural.c
│   │   │   ├── environment/              # Variable environment
│   │   │   ├── error/create.c            # Error creation
│   │   │   ├── expression/               # Expression evaluation
│   │   │   │   ├── expression.c          # Main expression dispatch
│   │   │   │   ├── array.c, async.c, await.c
│   │   │   │   ├── binary.c              # Binary operators (+, -, etc.)
│   │   │   │   ├── identifier.c          # Variable lookup
│   │   │   │   ├── literal.c             # Literal values
│   │   │   │   ├── member.c              # obj.prop
│   │   │   │   ├── object.c              # Object literal
│   │   │   │   ├── string_interp.c       # String interpolation
│   │   │   │   ├── subscript.c           # arr[i]
│   │   │   │   └── update.c              # ++, -- dan compound +=, -=, dst
│   │   │   ├── function/                 # Function handling
│   │   │   │   ├── call.c                # Function call
│   │   │   │   └── declaration.c         # Function declaration
│   │   │   ├── modules/                  # Module system
│   │   │   │   ├── dispatch.c, loader.c, module.h
│   │   │   ├── statement/                # Statement execution
│   │   │   │   ├── statement.c           # Main statement dispatch
│   │   │   │   ├── case.c, struct.c
│   │   │   │   ├── loop.c, loop_for.c, loop_helper.c, loop_rev.c
│   │   │   └── value/value.c             # Value operations
│   │   │
│   │   └── semantic/             # Semantic analysis
│   │       ├── eventloop.c       # Async event loop
│   │       ├── symbol.c          # Symbol table
│   │       └── analyzer.c        # Registry struct + validasi assignment/annotation
│   │                             # + shorthand object literal {name, health}
│   │
│   ├── editor/                   # TUI editor
│   │   ├── editor.c
│   │   ├── core/
│   │   │   ├── buffer.c          # Text buffer management
│   │   │   ├── cursor.c          # Cursor positioning
│   │   │   └── mode.c            # Editor modes
│   │   ├── display/
│   │   │   ├── drawer.c          # Screen drawing
│   │   │   ├── refresh.c         # Display refresh
│   │   │   └── terminal.c        # Terminal handling
│   │   └── operations/
│   │       ├── history.c         # Undo/redo
│   │       ├── indent.c          # Auto-indentation
│   │       ├── reset.c           # Reset state
│   │       └── state.c           # State management
│   │
│   ├── prompt/                   # Command prompt
│   │   ├── prompt.c, runner.c, test.c
│   │
│   ├── repl/                     # REPL mode
│   │   ├── repl.c, repl_command.c, repl_input.c
│   │
│   ├── runtime/                  # Runtime system
│   │   ├── context/
│   │   │   ├── context.c, create.c
│   │   ├── gc/gc.c              # Garbage collector
│   │   ├── input/
│   │   │   ├── cleaner.c, create.c, flags.c, input.c
│   │   ├── io/readfile.c         # File I/O
│   │   ├── keyword/create.c      # Keyword registration
│   │   └── validation/           # Runtime validation
│   │
│   ├── state/state.c             # Global state
│   │
│   ├── stdlib/                   # Standard library (C functions)
│   │   ├── stdlib.c              # Module registration
│   │   ├── http_client.c         # HTTP client (curl)
│   │   ├── http_server.c         # HTTP server (POSIX sockets)
│   │   ├── io.c                  # input(), toNumber()
│   │   ├── json.c                # JSON stringify/parse
│   │   ├── json_parser.c         # JSON parser
│   │   ├── math.c                # Math functions
│   │   ├── os.c                  # OS functions
│   │   ├── string.c              # String functions
│   │   ├── thread.c              # Thread functions
│   │   ├── loader.c              # Stdlib loader
│   │   ├── manifest.c            # Module manifest
│   │   ├── package.c             # Package management
│   │   ├── install.c             # Package installer
│   │   └── test_helper.c         # Test utilities
│   │
│   ├── debug/debug.c             # Debug utilities
│   └── utils/strings.c           # String utilities
│
├── tests/                        # Test files
│   ├── syntax/                   # Syntax tests (46 files)
│   │   ├── annotation.rp, array.rp, assignment.rp, async.rp, ...
│   │   └── modules/              # Test module files (a.rp, b.rp, ...)
│   │
│   ├── ast/                      # AST structure tests (8 files)
│   │   ├── async.rp, binary_chain.rp, complex_array.rp, ...
│   │
│   ├── execution/                # Execution tests (29 files)
│   │   ├── array_ops.rp, basic_arithmetic.rp, ...
│   │   └── repl_*.rp             # REPL-specific tests (18 files)
│   │
│   ├── semantics/                # Semantic analysis tests (6 files)
│   │   ├── function_param_types.rp, type_annotation_basic.rp, ...
│   │
│   ├── formatter/                # Formatter tests
│   ├── modules/                  # Module tests (20 files)
│   │   ├── math/, json/, collections/, strings/, thread/
│   │   └── dbtest/, crypto/, datetime/, net/, regex/
│   │
│   └── stress/stress tests
│
├── modules/                      # Rupa standard modules (packaged)
│   └── rupa_modules.tar.gz       # Archive of stdlib modules
│
├── docs/                         # Documentation
│   ├── index.md                  # Docs index
│   ├── README.md                 # Docs readme
│   ├── structure.md              # This file
│   ├── TODO.md                   # Project status & roadmap
│   ├── about.md, mission.md, vision.md
│   ├── instruction.md
│   │
│   ├── syntax/                   # Language syntax docs (28 files)
│   │   ├── module.md             # → modules/syntax/
│   │   ├── annotation.md, array.md, assignment.md, async.md, ...
│   │
│   ├── grammar/                  # Grammar/AST docs (28 files)
│   │   ├── module.md             # → modules/grammar/
│   │   ├── annotation.md, array.md, assignment.md, async.md, ...
│   │
│   └── modules/                  # Module documentation
│       ├── syntax/               # Module syntax docs
│       │   ├── math.md, os.md, io.md, json.md
│       │   ├── string.md, thread.md, http.md
│       └── grammar/              # Module grammar docs
│           ├── math.md, os.md, io.md, json.md
│           ├── string.md, thread.md, http.md
│
├── commands/                     # Build & dev scripts
│   ├── main.sh                   # Script dispatcher
│   ├── build.sh, build_test.sh   # Build scripts
│   ├── run.sh                    # Run script
│   ├── test.sh                   # Test runner
│   ├── debug.sh                  # Debug build
│   ├── release.sh                # Release build
│   ├── bootstrap.sh              # Module bootstrap
│   └── lib/                      # Script utilities
│       ├── colors.sh, compiler.sh, fs.sh
│       ├── logging.sh, os.sh, paths.sh
│       ├── progress.sh, table.sh
│
├── examples/                     # Example projects
│   └── calculator/
│       ├── index.rp
│       └── modules/op.rp
│
├── .github/                      # GitHub config
├── build/                        # Build artifacts (.o files)
├── .cache/                       # Build cache
├── .logs/                        # Log files
│
├── Makefile                      # Build system
├── build.sh                      # Top-level build entry
├── compile_commands.json         # LSP compile database
├── LICENSE
└── README.md
```

## Architecture Overview

```
┌─────────────────────────────────────────────────┐
│                    Entry Point                   │
│  src/main.c → prompt/runner.c → processInput()  │
└─────────────┬───────────────────┬───────────────┘
              │                   │
              ▼                   ▼
   ┌──────────────────┐  ┌──────────────────┐
   │   REPL Mode      │  │   File Mode      │
   │  src/repl/*.c    │  │  src/prompt/*.c  │
   └────────┬─────────┘  └────────┬─────────┘
            │                     │
            └──────────┬──────────┘
                       ▼
            ┌─────────────────────┐
            │   Lexer (Tokenizer) │
            │  src/compiler/lexer │
            └──────────┬──────────┘
                       ▼
            ┌─────────────────────┐
            │   Grammar Parser    │
            │ src/compiler/parser │
            │  grammar/ + ast/    │
            └──────────┬──────────┘
                       ▼
            ┌─────────────────────┐
            │   AST Interpreter   │
            │src/compiler/interpr │
            │  expression/        │
            │  statement/         │
            │  function/          │
            │  modules/           │
            └──────────┬──────────┘
                       ▼
            ┌─────────────────────┐
            │    IR Pipeline      │
            │ src/compiler/ir/    │
            │  rewrite.c (AST→IR) │
            │  execute.c (mesin)  │
            └──────────┬──────────┘
                       ▼
            ┌─────────────────────┐
            │   Runtime System    │
            │  src/runtime/       │
            │  GC, Context, IO    │
            └──────────┬──────────┘
                       ▼
            ┌─────────────────────┐
            │   Standard Library  │
            │  src/stdlib/        │
            │  http, json, math,  │
            │  os, thread, string │
            └─────────────────────┘
```

## Key Directories

| Directory       | Purpose                                                  |
| --------------- | -------------------------------------------------------- |
| `include/`      | Public API (`#include <rupa>`)                           |
| `lib/`          | Header files & type definitions                          |
| `lib/intl.h`    | Central include hub                                      |
| `lib/core/`     | Enums, constants, macros                                 |
| `lib/compiler/` | Struct types menempel header fn decl compiler            |
| `lib/modules/`  | Module system header                                     |
| `src/`          | Implementation files                                     |
| `src/compiler/` | Lexer → Parser → Interpreter + IR pipeline               |
| `src/compiler/ir/` | rewrite (AST→IR) & execute (mesin IR)                 |
| `src/formatter/` | Formatter modular (top-level)                           |
| `src/stdlib/`   | C-implemented standard library modules                   |
| `src/runtime/`  | GC, context, input, validation                           |
| `src/editor/`   | TUI editor (buffer, cursor, display)                     |
| `src/repl/`     | Interactive REPL mode                                    |
| `tests/`        | Test suites (syntax, ast, execution, semantics, modules) |
| `modules/`      | Packaged Rupa modules (tar.gz)                           |
| `docs/`         | Documentation (syntax, grammar, modules)                 |

> **Struktur lib (rencana):** struct types sekarang menempel langsung
> pada header fn decl masing-masing (contoh: `RuntimeValue` di
> `lib/compiler/interpreter/value.h`) — `lib/types/` sudah tidak ada.
> Langkah berikutnya memisahkan ke `lib/compiler/enums/*.h` (enum dari
> `lib/core/`) dan `lib/compiler/types/*.h` (types dari header fn decl).
| `commands/`     | Build & development scripts                              |
