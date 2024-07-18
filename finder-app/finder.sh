#!/bin/sh

: '
Accepts the following runtime arguments: the first argument is a path to a directory on the filesystem, referred to below as filesdir; the second argument is a text string which will be searched within these files, referred to below as searchstr

- Exits with return value 1 error and print statements if any of the parameters above were not specified

- Exits with return value 1 error and print statements if filesdir does not represent a directory on the filesystem

- Prints a message "The number of files are X and the number of matching lines are Y" where X is the number of files in the directory and all subdirectories and Y is the number of matching lines found in respective files, where a matching line refers to a line which contains searchstr (and may also contain additional content).
'

if ! [ $# -eq 2 ]; then
    echo "usage: $0 path text"
    exit 1
fi

FILESDIR=$1
SEARCHSTR=$2

if ! [ -d "${FILESDIR}" ]; then
    echo "Not a directory: ${FILESDIR}"
    exit 1
fi

FILECNT=$(find "${FILESDIR}" -type f | wc -l)
MATCHCNT=$(grep -rc "${SEARCHSTR}" "${FILESDIR}" | awk -F: '{s+=$2} END {print s}')

echo "The number of files are ${FILECNT} and the number of matching lines are ${MATCHCNT}."

exit 0
