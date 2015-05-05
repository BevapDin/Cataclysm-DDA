#! /bin/bash

backup_dir='save.backup'
world='ConvWorld 1'

mkdir -p "$backup_dir"
( cd 'save' && find "$world" -type d -print0 ) | \
( cd "$backup_dir" && xargs -0 mkdir -p ) || exit $?

cd 'save' && cp -a -u -v "$world" ../"$backup_dir" 2>&1 | sort

exit 0
