#!/usr/bin/env bash

# Load paths first to ensure consistency
if [ -f "$(dirname "${BASH_SOURCE[0]}")/paths.sh" ]; then
  source "$(dirname "${BASH_SOURCE[0]}")/paths.sh"
fi

# Editor configuration
EDITOR=bee

# These will be set by paths.sh, but provide defaults for backward compatibility
: ${BEE_CONFIG_DIR:="${HOME}/.config/bee"}

# Other configuration (unchanged)
REPO_URL="https://github.com/aidomx/bee.git"
INSTALL_DIR="${BEE_CONFIG_DIR}"
BEE_BIN="/usr/bin/bee"
TERMUX_VERSION="0.119.0-beta.3"

connection_test() {
  local url=$1

  if ping -q -c 1 -W 10 "$url" >/dev/null 2>&1; then

    if command -v curl >/dev/null 2>&1; then
      local response
      if response=$(curl -s --connect-timeout 10 --max-time 15 "$url" 2>/dev/null); then
        echo "$response"
        return 0
      fi
    fi

    if command -v wget >/dev/null 2>&1; then
      local response
      if response=$(wget -q -O - --timeout=10 "$url" 2>/dev/null); then
        echo "$response"
        return 0
      fi
    fi

    local domain
    if [[ "$url" =~ https?://([^/]+) ]]; then
      domain="${BASH_REMATCH[1]}"
      if ping -c 1 -W 5 "$domain" >/dev/null 2>&1; then
        echo "connected"
        return 0
      fi
    fi

    if [[ "$url" =~ https?://([^:/]+)(:([0-9]+))? ]]; then
      local host="${BASH_REMATCH[1]}"
      local port="${BASH_REMATCH[3]}"

      if [[ "$url" == https://* ]]; then
        port="${port:-443}"
      else
        port="${port:-80}"
      fi

      if timeout 5 bash -c "echo > /dev/tcp/$host/$port" 2>/dev/null; then
        echo "connected"
        return 0
      fi
    fi

    return 1
  fi
  return 1
}

check_internet_connection() {
  local url=${1:-"github.com"}

  (connection_test "$url" >/dev/null 2>&1) &
  local CHECK_PID=$!
  progress "dots" $CHECK_PID " - Checking internet connection"
  wait $CHECK_PID || {
    echo ""
    print_error "No internet connection, please check your internet, and try again!"
    return 1
  }
  return 0
}

import() {
  [[ $# -eq 0 || -z "$@" ]] && {
    print_warning "${YELLOW}import:${NC} No file or directory specified"
    return 1
  }

  if [[ $# -eq 1 ]]; then
    local -a list_file
    IFS=' ' read -ra list_file <<<"$1"

    for file in "${list_file[@]}"; do
      local single_file="$(echo "$APP_ROOT"/**/"$file.sh")"
      [[ ! -f "$single_file" ]] && {
        print_warning "${YELLOW}import($single_file):${NC} No file or directory specified"
        return 0
      }

      . "$single_file"
    done

    return 0
  fi

  for value in "$@"; do
    [[ ! -d "$value" || ! -f "${value}" ]] && {
      print_warning "${YELLOW}import(${value}):${NC} No such file or directory"
      continue
    }

    # Check jika directory punya file .sh
    for src in "$value"/*.sh; do
      # Handle case where no .sh files exist
      [[ ! -f "$src" ]] && {
        print_warning "${YELLOW}import(${dir}):${NC} No .sh files found"
        continue
      }
      . "$src"
    done
  done
}
