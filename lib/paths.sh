#!/usr/bin/env bash
set_default_path() {
  export APP_ROOT=${APP_ROOT:-$(realpath .)}
  export BINARY_DIR="${APP_ROOT}/bin"
  export BUILD_DIR="${APP_ROOT}/build"
  export CMD_DIR="${APP_ROOT}/commands"
  export LOG_DIR="${APP_ROOT}/.logs"
  export SRC_DIR="${APP_ROOT}/src"
}
