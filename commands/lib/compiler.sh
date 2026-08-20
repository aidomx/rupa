#!/usr/bin/env bash
set_default_compiler() {
  # Compiler flags
  export WALL_FLAGS="-Wall -Wextra"
  export INCLUDE_FLAGS="-Iinclude -I."
  export CLANGD_FLAGS="-xc -std=gnu11 -D_GNU_SOURCE -D_DEFAULT_SOURCE -fPIE"

  # Build flags - FIXED untuk aarch64
  export DEBUG_FLAGS="-g -O0"
  export RELEASE_FLAGS="-g -O2 -DDEBUG"
  export LEAK_FLAGS="-fsanitize=leak"
  export UB_FLAGS="-fsanitize=undefined"

  export CC="gcc"

  if $compdb && command -v ccache >/dev/null 2>&1; then
    export CC="ccache gcc"
  fi

  export CFLAGS="${INCLUDE_FLAGS} ${WALL_FLAGS} ${CLANGD_FLAGS}"
  export LDFLAGS=""
}

set_default_header() {
  # Header
  export BUILD_HEADER="> Build artifacts"
  export CLEAN_HEADER="> Cleaning build artifacts"
  export DEBUG_HEADER="> Build artifacts with debugging"
}

# Source files
set_default_source() {
  SRC=()
  OBJ=()

  while IFS= read -r src; do
    SRC+=("$src")
    OBJ+=("$src:$SRC_DIR/%.c=$BUILD_DIR/%.o")
  done < <(find "$SRC_DIR" -name "*.c")

  export SRC
  export OBJ
}

set_default_target() {
  export TARGET_NAME="${APP_NAME:-rupa}"
  export TARGET_RELEASE_VERSION="1.0"
  export TARGET="${BINARY_DIR}/${TARGET_NAME}"
}

set_default_tracking() {
  # Progress tracking
  export TOTAL_FILES=0
  export COMPILED_FILES=0
  export COMPILED_SYNC="$COMPDB_FILE"
  export PERCENT=0
  export DEBUGGING=false
  export ERROR_TOTAL=0
  export UNUSED_TOTAL=0

  for _ in "${SRC[@]}"; do TOTAL_FILES=$((TOTAL_FILES + 1)); done
}
