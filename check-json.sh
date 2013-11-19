#! /bin/bash

grep add_spawn src/*.cpp | \
	sed -n '
		s#.*add_spawn *( *\"##
		T
		s#".*##
		p
	' | \
	sort | \
	uniq | \
	while read monster ; do
		if ! grep -Eqe '"id" *: *"'"$monster"'"' data/json/monsters.json ; then
			echo "Unknown monster $monster"
		fi
	done

grep place_spawns src/*.cpp | \
	sed -n '
		s#.*place_spawns *([^"]*\"##
		T
		s#".*##
		p
	' | \
	sort | \
	uniq | \
	while read monster ; do
		if ! grep -Eqe '"name" *: *"'"$monster"'"' data/json/monstergroups.json ; then
			echo "Unknown monster group $monster"
		fi
	done

grep -Ee '"monster" *:' -e '"default" *:' data/json/monstergroups.json | \
	sed '
		s#^.*"monster" *: *"##
		s#^.*"default" *: *"##
		s#".*##
	' | \
	sort | \
	uniq | \
	while read monster ; do
		if ! grep -Eqe '"id" *: *"'"$monster"'"' data/json/monsters.json ; then
			echo "Unknown monster $monster"
		fi
	done


