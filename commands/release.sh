#!/usr/bin/env bash
build_release() {
  export CFLAGS="$CFLAGS $RELEASE_FLAGS"
  build_clean
  build_common
}
