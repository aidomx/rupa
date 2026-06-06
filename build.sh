#!/usr/bin/env bash
main_build() {
  local args="$1"
  local extra="${2:-}"
  export compdb=false

  [[ -f "compile_commands.json" ]] && export compdb=true

  if $compdb || [[ "$args" == "test" ]]; then
    . bootstrap.sh "$args" "$extra"
    return
  fi

  if command -v intercept-build >/dev/null 2>&1; then
    intercept-build ./bootstrap.sh "$args" "$extra"
  elif command -v bear >/dev/null 2>&1; then
    bear -- ./bootstrap.sh "$args" "$extra"
  else
    . bootstrap.sh "$args" "$extra"
  fi
}

main_build "$@"
