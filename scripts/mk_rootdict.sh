#!/bin/bash
#
# Call rootcling after cleaning up -I include directives followed by CMake ;-lists or
# a single space-separated list of directories.
#
# This is a workaround for what seems to be a CMake bug.
# Apparently CMake (up to 3.11) is too dumb to pass a single type of argument. If it
# starts out as a ;-list, it is passed as a space-separated list to add_custom_command;
# if it starts out as a generator expression, it is passed as a ;-list.

ROOTCLING="$1"
shift

POSITIONAL=()
PREOPTIONS=()
OPTIONS=()
INCDIRS=()
DEFINES=()
PCMNAME=()
while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
	-f)
	    PREOPTIONS+=("$1")
	    shift
	    ;;
	-v*)
	    PREOPTIONS+=("$1")
	    shift
	    ;;
	*)
	    break
    esac
done

DICTNAME="$1"
shift

while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
	-s)
	    PCMNAME+=("$1")
	    shift
	    PCMNAME+=("$1")
	    shift
	    ;;
	-I)
	    # Turn a possible CMake list into a sequence of -I options
	    if echo $2 | grep -q ';'; then
		incdirs="$(echo $2 | sed -E -e 's|^([^ ]+);|-I\1;|' -e 's|;([^ ;]+)| -I\1|g' -e 's|-I ||g')"
	    else
		incdirs="$(echo $2| sed -E -e 's|^([^ ]+) +|-I\1 |' -e 's| ([^ ]+)| -I\1|g' -e 's|-I ||g')"
	    fi
	    INCDIRS+=("$(echo $incdirs | tr ' ' '\n' | sort -u | tr '\n' ' ')")
	    shift 2
	    ;;
	-I*)
	    # Turn a possible CMake list into a sequence of -I options
	    if echo $1 | grep -q ';'; then
		incdirs=("$(echo $1 | sed -E -e 's|^-I||' -e 's|^([^ ]+);|-I\1;|' -e 's|;([^ ;]+)| -I\1|g' -e 's|-I ||g')")
	    else
		incdirs=("$(echo $1 | sed -E -e 's|^-I||' -e 's|^([^ ]+) +|-I\1 |' -e 's|^([^ ]+)$|-I\1|' -e 's| +([^ ]+)| -I\1|g' -e 's|-I ||g')")
	    fi
	    INCDIRS+=("$(echo $incdirs | tr ' ' '\n' | sort -u | tr '\n' ' ')")
	    shift
	    ;;
	-D)
	    # Turn a possible CMake list into a sequence of -D options
	    if echo $2 | grep -q ';'; then
		defs="$(echo $2 | sed -E -e 's|^([^ ]+);|-D\1;|' -e 's|;([^ ;]+)| -D\1|g' -e 's|-D ||g')"
	    else
		defs="$(echo $2| sed -E -e 's|^([^ ]+) +|-D\1 |' -e 's| ([^ ]+)| -D\1|g' -e 's|-D ||g')"
	    fi
	    DEFINES+=("$(echo $defs | tr ' ' '\n' | sort -u | tr '\n' ' ')")
	    shift 2
	    ;;
	-D*)
	    # Turn a possible CMake list into a sequence of -D options
	    if echo $1 | grep -q ';'; then
		defs=("$(echo $1 | sed -E -e 's|^-D||' -e 's|^([^ ]+);|-D\1;|' -e 's|;([^ ;]+)| -D\1|g' -e 's|-D ||g')")
	    else
		defs=("$(echo $1 | sed -E -e 's|^-D||' -e 's|^([^ ]+) +|-D\1 |' -e 's|^([^ ]+)$|-D\1|' -e 's| +([^ ]+)| -D\1|g' -e 's|-D ||g')")
	    fi
	    DEFINES+=("$(echo $defs | tr ' ' '\n' | sort -u | tr '\n' ' ')")
	    shift
	    ;;
	-*)
	    OPTIONS+=("$1")
	    shift
	    ;;
	*)
	    POSITIONAL+=("$1")
	    shift
	    ;;
    esac
done

eval $ROOTCLING ${PREOPTIONS[@]} $DICTNAME ${PCMNAME[@]} ${OPTIONS[@]} ${INCDIRS[@]} ${DEFINES[@]} ${POSITIONAL[@]}
