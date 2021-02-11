#!/bin/sh
#
# Helper script for getting build process metadata.
#
# Command line arguments:
#   $1: C++ compiler command
#   $2: the path to this git repository (for out of source builds)
# Output:
#   Exactly 7 lines
#    date
#    date+time
#    platform string
#    host name
#    user name
#    git revision (short)
#    compiler version string

date -u "+%b %d %Y"
date -u "+%a %b %d %H:%M:%S %Z %Y"
echo $(uname -s)-$(uname -r)-$(uname -m)
uname -n
whoami
repo_path=""
if [ ! -z "$2" ];
then
  repo_path="-C $2"
fi
gitrev=$(git ${repo_path} rev-parse --short HEAD 2>/dev/null)
[ -n "$gitrev" ] && echo $gitrev || echo ""
$1 --version 2>/dev/null | head -1
