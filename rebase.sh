#! /bin/bash

all=false
onto=''
if [ "$1" == "--all" ] ; then
	all=true
elif [ "$1" == '--gitk' ] ; then
	exec gitk official-z-level..upstream/master src/
elif [ "$1" != '' ] ; then
	onto="$1"
fi

git fetch upstream || exit $?

ncb() {
	git log --merges --first-parent ..upstream/master --reverse --format=format:%H "$@"
}

nc() {
	if [ "$onto" != '' ] ; then
		echo "$onto"
	elif "$all" ; then
		ncb | tail -1
	else
		ncb "$@" | head -1
	fi
}

next_commit="$(nc -- src/)"

if [ -z "$next_commit" ] ; then
	next_commit="$(nc)"
	if [ -z "$next_commit" ] ; then
		echo "All is well."
		exit 0
	fi
	echo "Only json changes, use the normal git-rebase.sh if you want to include them."
	exit 0
fi

echo "Rebasing onto $next_commit"

git rebase --ignore-whitespace -Xignore-space-change --onto "$next_commit" && exit 0

git gui &
