#!/bin/bash
# Helper script to clear all LC_RPATH entries of a macOS binary

if otool -l $1 | grep -q LC_RPATH; then
  otool -l $1 | grep -A2 LC_RPATH | grep '[ ]*path' | \
  sed -E 's/[ ]*path (.+) \(offset.*/\1/' | \
    while read R; do
      install_name_tool -delete_rpath $R $1
    done
fi
