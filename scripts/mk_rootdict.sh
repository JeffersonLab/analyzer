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
# In part, this is a workaround for a CMake limitation. Until version 3.8,
# CMake was unable to pass ;-lists properly to an external command via
# add_custom_command. Version 3.8 added the COMMAND_EXPAND_LISTS option
# that removes this limitation. However, even with this option,
# parsing command line arguments instead with this script still has advantages:
# (a) it removes duplicate list entries, which CMake is still unable to do with
# generator expressions; and (b) it allows us to support lower CMake versions
# than the rather recent 3.8 (April 2017). The downside is the dependence on bash.

# FIXME: Remove in Podd 1.8. CMake now has all necessary functionality

ROOTCLING="$1"
shift

# Parse command line options
POSITIONAL=()
PREOPTIONS=()
OPTIONS=()
INCDIRS=()
DEFINES=()
while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
	-f)
	    PREOPTIONS+=("$1")
	    shift
# For compatibility with ROOT 5's rootcling, put the dictionary file name
# after -f and before other options (except -vN)
	    PREOPTIONS+=("$1")
	    shift
	    ;;
	-v*)
# The -v[N] option must be unique and come before -f, but we don't enforce that here
	    PREOPTIONS+=("$1")
	    shift
	    ;;
# Options with arguments
	-m|-s|-rmf|-rml|--moduleMapFile|--excludePath|--isystem)
	    OPTIONS+=("$1")
	    shift
	    OPTIONS+=("$1")
	    shift
	    ;;
# No-argument options or -X=ARG options
	-*)
      if echo "$1" | grep -q =; then
# Drop the "=". Older versions of rootcling don't understand that format
        IFS='=' read -ra OPTS <<< "$1"
        for i in "${OPTS[@]}"; do
          OPTIONS+=("$i")
        done
      else
	      OPTIONS+=("$1")
	    fi
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
	*)
# The positional arguments should be the header files, followed by LinkDefs
	    POSITIONAL+=("$1")
	    shift
	    ;;
    esac
done

# echo "=========== translated command: ================"
# echo $ROOTCLING ${PREOPTIONS[@]} ${OPTIONS[@]} ${INCDIRS[@]} ${DEFINES[@]} ${POSITIONAL[@]}
# echo "=========== end translated command ============="
eval $ROOTCLING ${PREOPTIONS[@]} ${OPTIONS[@]} ${INCDIRS[@]} ${DEFINES[@]} ${POSITIONAL[@]}
