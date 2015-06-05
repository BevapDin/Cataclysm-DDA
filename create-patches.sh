#! /bin/bash

for d in src/*.{cpp,h} data/json/items/*.json ; do
	git diff upstream/master... -- "$d" >"$d.patch"
done
find src data -iname '*.patch' -empty -delete
ls -lhS src/*.patch data/json/items/*.patch

