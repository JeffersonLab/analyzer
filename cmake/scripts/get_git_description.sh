#!/bin/sh
#
# Helper script for writing header file defining current git description
#
# Command line arguments:
#   $1: git executable path,
#       or "nogit" if no git support, resulting in empty definition
#   $2: output file
#   $3: preprocessor variable to #define
# Output:
#   Current git description written to $2

OUT="$2".tmp
printf "#define $3" > "$OUT"
if [ "$1" != "nogit" ]; then
  printf " \"%s\"\n" "$($1 describe --tags --always --long --dirty 2>/dev/null)" >> "$OUT"
else
  printf " \"\"\n" >> "$OUT"
fi
if cmp -s "$OUT" "$2"; then
  # nothing changed, do not rewrite (to prevent unneeded rebuild)
  rm -f "$OUT"
else
  # description changed, or $2 does not yet exist -> update $2
  mv -f "$OUT" "$2"
fi
