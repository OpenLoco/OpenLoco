#!/usr/bin/env sh

set -e

readlink_bin='readlink'

if [ "$(uname)" == "Darwin" ]; then
  if ! [ -x "$(command -v greadlink)" ]; then
    echo 'Please install greadlink with Homebrew: brew install coreutils'
    exit 1
  fi
  readlink_bin='greadlink'
fi

# Ensure we are in root directory
basedir="$($readlink_bin -f `dirname $0`/..)"
cd $basedir

scripts/run-clang-format.py -r src --exclude src/Thirdparty
