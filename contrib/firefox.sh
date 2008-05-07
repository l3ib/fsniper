#!/bin/bash
# Handler to prevent later handlers from being called on temporary Firefox files

# USAGE:
# Add this before any other handlers in all directories Firefox downloads to
# Also add this to your config to prevent handlers from being called on Firefox's .part files:
#    *.part {
#        handler = echo ignoring file: %%
#    }


args="$*"
length=`expr length "$*"`
size=`du "${args}" | awk '{print $1'}`

# if this is "foo" and "foo.part" exists, firefox is still downloading foo
# so we ignore it for now. it will be closed and the handler called again
# once the download finishes.
if [[ -e "${args}.part" && "$size" -eq "0" ]]
then
	exit 0
fi

exit 1
