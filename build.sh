#!/usr/bin/env bash

export APP_NAME="rupa"
export APP_ROOT="$(dirname "${BASH_SOURCE[0]}")"
export APP_VERSION="1.0.0"
export DEV_MODE="${DEV_MODE:-0}"

if [ $DEV_MODE -eq 0 ] && command -v rupa >/dev/null 2>&1; then
  export APP_ROOT="$HOME/$APP_NAME"
else
  export APP_ROOT="$(realpath "$APP_ROOT")"
fi

# library location
export APP_LIB="$APP_ROOT/commands/lib"
BOOTSTRAP="$APP_ROOT/commands/bootstrap.sh"
export COMPDB_FILE="$APP_ROOT/compile_commands.json"

main_build() {
  local args="$1"
  local bootstrap="$BOOTSTRAP"
  local extra="${2:-}"
  export compdb=false

  [[ -f $COMPDB_FILE ]] && export compdb=true

  if $compdb || [[ "$args" == "test" ]]; then
    . $bootstrap "$args" "$@"
    return
  fi

  if command -v intercept-build >/dev/null 2>&1; then
    intercept-build $bootstrap "$args" "$extra"
  elif command -v bear >/dev/null 2>&1; then
    bear -- $bootstrap "$args" "$extra"
  else
    . $bootstrap "$args" "$extra"
  fi
}

main_build "$@"
