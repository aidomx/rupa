#!/usr/bin/env bash
set_default_path() {
  export APP_ROOT=${APP_ROOT:-$(realpath .)}
  export BINARY_DIR="${APP_ROOT}/bin"
  export BUILD_DIR="${APP_ROOT}/build"
  export CMD_DIR="${APP_ROOT}/commands"
  export LOG_DIR="${APP_ROOT}/.logs"
  export SRC_DIR="${APP_ROOT}/src"
  export TARGET_EXT="${TARGET_EXT:-}"
  export TARGET_PLATFORM="${TARGET_PLATFORM:-unix}"
}

# Setup target-specific paths for cross-compilation
set_target_path() {
  local target=${1:-"unix"}

  case "$target" in
  win32)
    export BUILD_DIR="${APP_ROOT}/build/win32"
    export BINARY_DIR="${APP_ROOT}/bin/win32"
    export TARGET_EXT=".exe"
    ;;
  win64)
    export BUILD_DIR="${APP_ROOT}/build/win64"
    export BINARY_DIR="${APP_ROOT}/bin/win64"
    export TARGET_EXT=".exe"
    ;;
  unix|*)
    export BUILD_DIR="${APP_ROOT}/build"
    export BINARY_DIR="${APP_ROOT}/bin"
    export TARGET_EXT=""
    ;;
  esac
}
