#! /bin/bash

if [ "$#" = 0 ] ; then
	exec "$0" -k release-cataclysm
fi

exec make BUILD_PREFIX=release- LUA=1 RELEASE=1 "$@"
