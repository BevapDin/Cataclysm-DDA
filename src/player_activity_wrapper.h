#pragma once

#include "activity.h"
#include "player_activity.h"

class player_activity_wrapper : public activity, public player_activity
{
    public:
        player_activity_wrapper( player_activity act ) : activity(), player_activity( std::move( act ) ) {
        }
        ~player_activity_wrapper() override = default;

        OnTurnResult do_turn( player &u ) override;

        bool is_suspendable() const override;
        std::string stop_phrase() const override;
        bool rooted() const override;
        bool resume_with( const activity &other ) override;

        virtual void serialize( JsonOut &jsout ) const override;
};
