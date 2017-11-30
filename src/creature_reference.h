#pragma once
#ifndef WEAK_CREATURE_PTR
#define WEAK_CREATURE_PTR

class Creature;
class player;
class JsonOut;
class JsonIn;

class creature_reference {
	private:
		int creature_id = 0;

	public:
		creature_reference() = default;
		//@todo change to Creature, but currently only works for player
		creature_reference( const player &critter );

		template<typename T = Creature>
		T *get() const;

		bool operator==( const creature_reference &rhs ) const {
			return creature_id == rhs.creature_id;
		}
		bool operator!=( const creature_reference &rhs ) const {
			return !operator==( rhs );
		}

		void serialize( JsonOut &jsout ) const;
		void deserialize( JsonIn &jsin );
};

#endif
