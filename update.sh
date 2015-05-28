#! /bin/sh

cont=false
if [ "$1" = "--cont" ] ; then
	cont=true
fi

if $cont ; then
	git commit || exit $?
else
	git stash || exit $?
	git fetch upstream || exit $?
	if ! git merge upstream/master ; then
		echo "Failed, fix them and close git gui to continue."
		git gui
		if ! git commit ; then
			echo "Failed again, fix this and rerun $0 with the --cont flag."
			exit 1
		fi
	fi
fi

git stash pop || exit $?

make CCACHE=1 LUA=1 RELEASE=1 -j2 || exit $?

./backup-save.sh
./xo.sh
./cd-extractor.py

exit 0

