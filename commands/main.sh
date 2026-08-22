#!/usr/bin/env bash
import "build debug release run"

[[ $# -eq 0 ]] && print_warning "Argument is required"

case "$1" in
debug) build_debug "$@" ;;
release) build_release "$@" ;;
test) run_test "$@" ;;
esac
