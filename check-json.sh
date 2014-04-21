#! /bin/bash

check_monsters_groups() {
    sort | \
    uniq | \
	while read monster ; do
		if [ "$monster" = 'GROUP_NULL' ] ; then
			continue
		fi
		if ! grep -Eqe '"name" *: *"'"$monster"'"' data/json/monstergroups.json ; then
			echo "Unknown monster group $monster"
		fi
	done
}

check_monsters() {
    sort | \
    uniq | \
	while read monster ; do
		if [ "$monster" = 'mon_null' ] ; then
			continue
		fi
		if ! grep -Eqe '"id" *: *"'"$monster"'"' data/json/monsters.json ; then
			echo "Unknown monster $monster"
			grep -F '"'"$monster"'"' src/*.cpp
		fi
	done
}


grep add_spawn src/*.cpp | \
	sed -n '
		s#.*add_spawn *( *\"##
		T
		s#".*##
		p
	' | \
    check_monsters

grep place_spawns src/*.cpp | \
    sed -n '
        s#.*place_spawns *([^"]*\"##
        T
        s#".*##
        p
    ' | \
    check_monsters_groups

grep GetMType src/*.cpp | \
    sed -n '
        s#.*GetMType *( *\"##
        T
        s#".*##
        p
    ' | \
    check_monsters

grep \"mon_ src/*.cpp | \
    sed -n '
        s#.*\"mon_#mon_#
        T
        s#".*##
        p
    ' | \
    check_monsters

grep \"GROUP_ src/*.cpp | \
    sed '
        s#.*\"GROUP_#GROUP_#
        s#".*##
    ' | \
    check_monsters_groups

grep mongroup src/*.cpp | \
	sed -n '
		s#.*mongroup *([^"]*\"##
		T
		s#".*##
		p
	' | \
    check_monsters_groups

grep -Ee '"monster" *:' -e '"default" *:' data/json/monstergroups.json | \
	sed '
		s#^.*"monster" *: *"##
		s#^.*"default" *: *"##
		s#".*##
	' | \
    check_monsters


