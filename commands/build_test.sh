#!/usr/bin/env bash

# Set strict-ish mode
set -euo pipefail

# Load library
while IFS= read -r lib; do
  . "$lib"
done < <(find "$APP_LIB" -name "*.sh")

set_color
set_default_compiler
set_default_path
set_default_target
set_default_header
set_default_source
set_default_tracking

# Source command implementations
[ -f "${CMD_DIR}/test.sh" ] && . "${CMD_DIR}/test.sh"
