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
  debug | release | test)
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
esac
