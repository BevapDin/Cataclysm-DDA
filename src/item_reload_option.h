#pragma once
#ifndef CATA_SRC_ITEM_RELOAD_OPTION_H
#define CATA_SRC_ITEM_RELOAD_OPTION_H

#include "item_location.h"
#include "game_constants.h"

class Character;

class item_reload_option
{
	public:
		item_reload_option() = default;

		item_reload_option( const item_reload_option & );
		item_reload_option &operator=( const item_reload_option & );

		item_reload_option( const Character *who, const item_location &target, const item_location &ammo );

		const Character *who = nullptr;
		item_location target;
		item_location ammo;
		bool is_reload_one = false;

		int qty() const {
			return qty_;
		}
		void qty( int val );

		int moves() const;

		explicit operator bool() const {
			return who && target && ammo && qty_ > 0;
		}
	private:
		int qty_ = 0;
		int max_qty = INT_MAX;
};

#endif // CATA_SRC_ITEM_RELOAD_OPTION_H
