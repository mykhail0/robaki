#!/bin/bash

clear
cp ../../build/screen-worms-* .
python3 tests_100.py
python3 tests_200.py
python3 tests_300.py
rm screen-worms-*
