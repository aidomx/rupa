#!/usr/bin/env bash
build_debug() {
  export DEBUGGING=true
  export CFLAGS="$CFLAGS $DEBUG_FLAGS $LEAK_FLAGS"
  echo -e "${CYAN}$DEBUG_HEADER${NC}"
  build_clean
  build_common
  $DEBUGGING && scan_results
}
