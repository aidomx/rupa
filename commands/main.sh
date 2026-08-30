#!/usr/bin/env bash
import "build debug release run"

[[ $# -eq 0 ]] && print_warning "Argument is required"

# Parse arguments
local_action=""
local_target="unix"
local_has_target=false

for ((i = 1; i <= $#; i++)); do
  arg="${!i}"
  case "$arg" in
  debug | release | test | docs)
    [[ -z "$local_action" ]] && local_action="$arg"
    ;;
  --target | -t)
    ((i++))
    local_target="${!i:-unix}"
    local_has_target=true
    ;;
  *)
    ;;
  esac
done

case "$local_action" in
debug) build_debug "$@" ;;
release) build_release "$local_target" "$@" ;;
test) run_test "$@" ;;
docs)
  cd "$APP_ROOT/docs"
  case "${2:-}" in
  build) npx vitepress build ;;
  preview) npx vitepress preview ;;
  *) npx vitepress dev ;;
  esac
  ;;
esac
