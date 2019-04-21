#!/bin/bash

SCRIPT_DIR=$(dirname "$0")
cd $SCRIPT_DIR/..

./third_party/styleguide/cpplint/cpplint.py --verbose=0 \
    --filter=-legal/copyright,-build/header_guard,-build/c++11,-whitespace/newline \
      --root=$SCRIPT_DIR/.. $($SCRIPT_DIR/all_files.sh)
