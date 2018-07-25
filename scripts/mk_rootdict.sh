#!/bin/bash
#
# mk_rootdict.sh. Wrapper script around rootcling/rootcont.
#
# Process INCDIRS followed by ""-quoted ;-list into sequence of -I directives;
# similarly, translate DEFINES followed by ;-list into sequence of -D arguments.
# Remove duplicates while preserving order (keep only first occurrence found).
# Quote any paths and definitions containing spaces.
# Call rootcling/rootcint with the translated arguments, preserving the rest
# of the command line.
#
# In part, this is a workaround for what seems to be a CMake bug.
# Apparently CMake (up to 3.12.0) passes inconsistent arguments. If it starts out
# as a ;-list, it is passed as a space-separated list to add_custom_command();
# if it starts out as a generator expression, it is passed as a ;-list.
#
# Even if this bug were fixed, we'd still benefit form this script because
# (a) it removes duplicates, which CMake won't be able to do with generator
# expressions, and (b) passing quoted strings and arguments containing spaces
# to add_custom_command() is apparently not really possible from CMake. The
# developers recommend to write an external script for that purpose anyway.
# So here we are.

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
	INCDIRS)
	    if [ "$2"x != "x" ]; then
		while read inc; do
		    if echo $inc | grep -q " "; then
			inc=\""$inc"\"
		    fi
		    INCDIRS+=("-I$inc")
		done < <(echo $2 | sed -e 's/^;//' -e 's/;$//' -e 's/;;/;/g' | tr ';' '\n' | \
		    awk '!nseen[$0]++')
	    fi
	    shift 2
	    ;;
	DEFINES)
	    if [ "$2"x != "x" ]; then
		while read def; do
		    if echo $def | grep -q " "; then
			def=\""$def"\"
		    fi
		    INCDIRS+=("-D$def")
		done < <(echo $2 | sed -e 's/^;//' -e 's/;$//' -e 's/;;/;/g' | tr ';' '\n' | \
		    awk '!nseen[$0]++')
	    fi
	    shift 2
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

# echo "=========== translated command: ================"
# echo $ROOTCLING ${PREOPTIONS[@]} $DICTNAME ${PCMNAME[@]} ${OPTIONS[@]} ${INCDIRS[@]} ${DEFINES[@]} ${POSITIONAL[@]}
# echo "=========== end translated command ============="
eval $ROOTCLING ${PREOPTIONS[@]} $DICTNAME ${PCMNAME[@]} ${OPTIONS[@]} ${INCDIRS[@]} ${DEFINES[@]} ${POSITIONAL[@]}
