#!/bin/sh
set -e

# Ensure we are in root directory
basedir="$(readlink -f `dirname $0`/..)"
cd $basedir

scripts/run-clang-format.py -r src --exclude src/Thirdparty
