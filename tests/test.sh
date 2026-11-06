#!/bin/bash

set -u

SCRIPT_DIR="$(dirname "$(realpath "$0")")"
cd "$SCRIPT_DIR" || {
    echo "Error: Failed to change directory to $SCRIPT_DIR" >&2
    exit 1
}

server="../build/server"

if [ ! -x "$server" ]; then
    echo "Error: $server executable not found or lacks execute permissions." >&2
    exit 1
fi
