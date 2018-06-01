#! /bin/sh

base_dir="${0%/*}"
exec_path="${0%.sh}"

export LD_LIBRARY_PATH="$base_dir:$LD_LIBRARY_PATH"

make
exec "$exec_path" "$@" 2>&1

