#! /bin/bash

if [ "$#" = 0 ] ; then
	exec "$0" -k release-cataclysm
fi

exec make DEBUG_SYMBOLS=1 BUILD_PREFIX=release- LUA=1 RELEASE=1 "$@"
