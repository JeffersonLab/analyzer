#!/bin/sh
#
# Helper script for getting build process metadata.
#
# Command line arguments:
#   $1: C++ compiler command
# Output:
#   Exactly 7 lines
#    current date (DD MMM YYYY)
#    current date+time+TZ offset (RFC2822)
#    (empty line)
#    platform string
#    host name
#    user name
#    compiler version string

date "+%d %b %Y"
date "+%a, %d %b %Y %H:%M:%S %z"
# This used to be the git revision, which is now retrieved from within CMake
echo
echo "$(uname -s)-$(uname -r)-$(uname -m)"
uname -n
whoami
"$1" --version 2>/dev/null | head -1
