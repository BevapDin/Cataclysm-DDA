#! /bin/sh

do_stash=false

if git status --short|grep -q '^.M ' ; then
	do_stash=true
	git stash || exit $?
fi
git fetch upstream || exit $?
if ! git rebase --ignore-whitespace -Xignore-space-change -Xignore-all-space -Xignore-space-at-eol upstream/master ; then
	echo "Failed, fix them, now!" 1>&2
	git gui &
	exit 1
fi

if "$do_stash" ; then
	git stash pop || exit $?
fi

make CCACHE=1 LUA=1 RELEASE=1 -j2 cataclysm || exit $?

../keeping-track/cd-extractor.py Boats More_Survival_Tools Craft_Gunpowder

exit 0

