#!/bin/bash

# TODO this script doesn't work when launched from random directory
cp ../../build/screen-worms-client .
cp ../../build/screen-worms-server .
python3 tests_100.py
# python3 tests_200.py
python3 tests_300.py
rm screen-worms-*
