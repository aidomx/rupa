#!/usr/bin/env bash

check_os() {
  # Priority 1: Termux specific environment variables
  [[ -n "$TERMUX_VERSION" ]] && echo "Android" && return

  # Priority 2: Android-specific directories and files
  [[ -d /system ]] && [[ -d /data/data ]] && echo "Android" && return

  # Priority 3: uname -o dengan Termux path detection
  local os=$(uname -o 2>/dev/null || echo "unknown")
  local path=$(echo $PATH | grep "com.termux")
  [[ "$os" == "Android" ]] && echo "Android" && return
  [[ "$os" == "GNU/Linux" && -n "$path" ]] && echo "Android" && return

  # Priority 4: Check /proc/version for Android kernels
  if [[ -f /proc/version ]] && grep -qi "android" /proc/version; then
    echo "Android" && return
  fi

  # Other OS detection
  local kernel=$(uname -s)
  case "$kernel" in
  Darwin) echo "macOS" ;;
  Linux) echo "Linux" ;;
  MINGW* | MSYS* | CYGWIN*) echo "Windows" ;;
  *) echo "unknown" ;;
  esac
}

is_mobile() {
  [[ "$(check_os)" == "Android" ]]
}

get_win_size() {
  case "$(check_os)" in
  Android) echo 20 ;;
  *) echo 80 ;;
  esac
}
