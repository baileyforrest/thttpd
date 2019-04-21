#!/bin/bash

SCRIPT_DIR=$(dirname "$0")
cd $SCRIPT_DIR/..

clang-format -i -style=Google $($SCRIPT_DIR/all_files.sh)
