#!/bin/bash
#
# Helper script for getting build process metadata.
#
# Command line arguments:
#   $1: C++ compiler command
# Output:
#   Exactly 7 lines
#    current date (DD MMM YYYY)
#    current date+time+TZ offset (RFC2822)
#    short OS description
#    platform string
#    host name
#    user name
#    compiler version string

date "+%d %b %Y"
date "+%a, %d %b %Y %H:%M:%S %z"
if [ -r /etc/os-release ]; then
  . /etc/os-release
  IFS=" " read -ra namearr <<< $NAME
  NAME=${namearr[0]}
else
  if [ "$(uname -s)" = "Darwin" ]; then
    NAME="macOS"
    VERSION_ID="$(sw_vers | grep ProductVersion | cut -f2)"
  elif [ "$(uname -s)" = "Linux" ]; then
    NAME="Linux"
    IFS="." read -ra verarr <<< "$(uname -r)"
    VERSION_ID=${verarr[0]}.${verarr[1]}
  else
    NAME="$(uname -s)"
    VERSION_ID="$(uname -r)"
  fi
fi
[ -n "$VERSION_ID" ] && echo $NAME-$VERSION_ID || echo $NAME
echo "$(uname -s)-$(uname -r)-$(uname -m)"
uname -n
whoami
"$1" --version 2>/dev/null | head -1
