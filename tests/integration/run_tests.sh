#!/usr/bin/env bash
#
# Drver script for Podd integration tests.
#
# Assumes ROOT and ANALYZER are set up. Further assumes that all ROOT scripts
# and reference results are in the same directory as this script.
#
# Ole Hansen, ole@jlab.org, 10-Nov-2023

set -e
#set -x

# Scratch directory. Need about 1 GB of space.
WORK="$1"
[ -z "$WORK" ] && WORK="/var/tmp"
if [ -e "$WORK" ]; then
  WORK="$(realpath "$WORK")"
  if [ ! -d "$WORK" ]; then
    echo ">>>> $1 is not a directory"
    exit 20
  fi
else
  if ! mkdir -p "$WORK"; then
    echo ">>>> Cannot create $WORK"
    exit 21
  fi
  WORK="$(realpath "$WORK")"
fi

# File names
RAWFILENAME="g2p_3132.dat.0"
EVIOF="$WORK/$RAWFILENAME"
DSTF="${EVIOF//.dat.0/.root}"
LOGF="${DSTF//.root/.log}"
REF_ROOTFILE="ref.root"

THIS="$( cd -P "$( dirname "${BASH_SOURCE[0]}" )" > /dev/null && pwd )"
DB_DIR="$THIS"/DB
export DB_DIR

if ! which root >/dev/null 2>&1; then
  echo ">>>> Cannot find ROOT"
  exit 1
fi
if ! which analyzer >/dev/null 2>&1; then
  echo ">>>> Cannot find analyzer"
  exit 2
fi
if ! analyzer --version >/dev/null; then
  echo ">>>> Error running analyzer"
  exit 3
fi

SHASUM=sha1sum   # Linux default (coreutils)

if [ "$(uname -s)" = Darwin ] && [ -z "$DYLD_LIBRARY_PATH" ]; then
  # macOS clears DYLD_LIBRARY_PATH when we execute this script, but
  # we need it to be set for ROOT to find our dictionaries
  ANALYZER_EXE="$(which analyzer)"
  ANALYZER_BIN="$(dirname "$ANALYZER_EXE")"
  ANALYZER="$(dirname "$ANALYZER_BIN")"
  export ANALYZER
  if [ -z "$ROOTSYS" ]; then
    ROOTSYS="$(dirname "$(dirname "$(which root)")")"
    export ROOTSYS
  fi
  # Extract the RPATH of the analyzer executable, if any
  ANALYZER_RPATH="$(otool -l "$ANALYZER_EXE" | grep -A2 LC_RPATH | grep path | \
    while read -r "P"; do
      echo "$P" | cut -d' ' -f2 | sed "s%@loader_path%$ANALYZER_BIN%"
    done | tr '\n' ':')"
  if [ -n "$ANALYZER_RPATH" ]; then
    DYLD_LIBRARY_PATH="${ANALYZER_RPATH::${#ANALYZER_RPATH}-1}"
  elif [ -n "$ANALYZER_LIBPATH" ]; then
    DYLD_LIBRARY_PATH="$ANALYZER_LIBPATH"
  else
    # Fallback if all else fails
    DYLD_LIBRARY_PATH="$ANALYZER/lib:$ROOTSYS/lib"
  fi
  export DYLD_LIBRARY_PATH

  # Ships with macOS 11+ (part of Perl). Otherwise install Homebrew coreutils
  if which shasum >/dev/null 2>&1; then
    SHASUM="shasum -a1"
  fi
fi

# Get raw data if necessary
pushd "$WORK" >/dev/null
if [ ! -r "$EVIOF" ]; then
  echo "Downloading raw data. This may take a few minutes."
  if ! curl -Os https://hallaweb.jlab.org/podd/download/g2p_3132.dat.0 ; then
    echo ">>>> Download error. Check your network connection."
    exit 4
  fi
fi
echo "Verifying raw data file"
if ! $SHASUM -c "$THIS/$RAWFILENAME".sha1 >/dev/null 2>&1 ; then
  echo ">>>> Raw data checksum failure. Check URL and network."
  exit 5
fi
echo "File checksum OK"
popd >/dev/null

pushd "$THIS" >/dev/null
# Replay the raw data
# echo "Replaying $EVIOF. This will take a few minutes"
if ! analyzer -l -b -q replay.cxx\(\""$EVIOF"\",\""$DSTF"\"\) >/dev/null; then
  echo ">>>> Error running replay.cxx"
  exit 10
fi

err=0
# Compare run summary with reference
grep -Ev '^(====|Reading)' "$LOGF" > "$WORK/tmp.log"
if ! diff -q "$WORK/tmp.log" "${REF_ROOTFILE//.root/.log}" ; then
  echo ">>>> Run summary differs"
  err=11
else
  echo "Run summary compares OK"
fi
rm "$WORK/tmp.log"

# Verify ROOT file data, comparing with reference
if ! analyzer -l -b -q verify.cxx\(\""$DSTF"\",\""$REF_ROOTFILE"\"\) >/dev/null; then
  echo ">>>> Error testing ROOT file"
  err=12
else
  echo "ROOT file tests OK"
fi

popd >/dev/null
exit $err
