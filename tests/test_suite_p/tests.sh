#!/bin/bash

set -u

SCRIPT_DIR="$(dirname "$(realpath "$0")")"
cd "$SCRIPT_DIR" || {
  echo "Error: Failed to change directory to $SCRIPT_DIR" >&2
  exit 1
}

BUILD_DIR=../../build
cp "${BUILD_DIR}/screen-worms-client" .
cp "${BUILD_DIR}/screen-worms-server" .
python3 server_parameter_tests.py
python3 server_tests.py
python3 client_tests.py
rm screen-worms-client
rm screen-worms-server
