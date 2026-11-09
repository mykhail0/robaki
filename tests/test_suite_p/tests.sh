#!/bin/bash

BUILD_DIR=../../build
cp "${BUILD_DIR}/screen-worms-client" .
cp "${BUILD_DIR}/screen-worms-server" .
python3 server_parameter_tests.py
python3 server_tests.py
# python3 client_tests.py
rm screen-worms-client
rm screen-worms-server
