#!/usr/bin/env bash
import "build debug release run"

main() {
  local message="$1"
  local log_file
  log_file=$(mktemp "${TMPDIR:-/tmp}/rupa-build.XXXXXX")

  export DEBUGGING=false
  export CFLAGS="$CFLAGS $DEBUG_FLAGS $LEAK_FLAGS"

  # Build in the background so normal build output stays hidden. The user only
  # sees one progress line; compiler output is preserved for failures.
  (
    build_clean >/dev/null 2>&1
    build_common
  ) >"$log_file" 2>&1 &
  local pid=$!

  progress "percent" "$pid" "$message"

  if wait "$pid"; then
    rm -f "$log_file"
    return 0
  fi

  printf '\n'
  print_error "Build failed"
  cat "$log_file"
  rm -f "$log_file"
  return 1
}

main "$@"
