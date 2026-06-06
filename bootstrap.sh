#!/usr/bin/env bash

# Set strict-ish mode
set -euo pipefail

APP_NAME="rupa"
APP_ROOT="$(dirname "${BASH_SOURCE[0]}")"
APP_VERSION="1.0.0"
DEV_MODE="${DEV_MODE:-0}"

if [ $DEV_MODE -eq 0 ] && command -v bee >/dev/null 2>&1; then
  APP_ROOT="$HOME/$APP_NAME"
else
  APP_ROOT="$(realpath "$APP_ROOT")"
fi

# Load library
while IFS= read -r lib; do
  . "$lib"
done < <(find "$APP_ROOT/lib" -name "*.sh")

set_color
set_default_compiler
set_default_path
set_default_target
set_default_header
set_default_source
set_default_tracking

# Source command implementations
[ -f "${CMD_DIR}/main.sh" ] && . "${CMD_DIR}/main.sh"
