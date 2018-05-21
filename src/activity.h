#pragma once

#include <memory>
#include <vector>
#include <string>

class JsonOut;
class JsonIn;
class player;

class activity {
    protected:
        activity() = default;

    public:
        virtual ~activity() = default;

        activity( const activity & ) = delete;

        enum class OnTurnResult {
            /// Activity continues.
            CONTINUE,
            /// Activity has been finished.
            FINISHED,
            /// Activity has been aborted. It can not be resumed!
            ABORTED,
            /// ACtivity has been suspended.
            SUSPENDED,
        };

        virtual OnTurnResult do_turn( player &u ) = 0;

        virtual bool is_suspendable() const = 0;

        virtual std::string stop_phrase() const = 0;
        /**
         * Tries to continue this activity with the given new one.
         * This should only succeed when the new activity is equivalent
         * to the current one.
         * @returns Whether resuming worked.
         */
        virtual bool resume_with( const activity &other ) = 0;

        virtual void serialize( JsonOut &jsout ) const = 0;
};

class activity_wrapper {
    private:
        std::vector<std::unique_ptr<activity>> backlog_;
        std::unique_ptr<activity> current_;

    public:
        activity_wrapper() = default;
        activity_wrapper( const activity_wrapper & );
        activity_wrapper( activity_wrapper && ) = default;

        activity_wrapper &operator=( const activity_wrapper & );
        activity_wrapper &operator=( activity_wrapper && ) = default;

        bool has_current() const {
            return static_cast<bool>( current_ );
        }
        activity &current() {
            return *current_;
        }
        const activity &current() const {
            return *current_;
        }

        void assign( std::unique_ptr<activity> act );
        bool resume_with( const activity &act );

        void serialize( JsonOut &jsout ) const;
        void deserialize( JsonIn &jsin );

        explicit operator bool () const {
            return has_current();
        }
        /**
         * Called on each turn of the player. Basically just invokes
         * @ref activity::do_turn and handles the result.
         * Does either consume all the moves of the character, or
         * resets the current activity (suspended/aborted/finished).
         * Postcondition: `!has_current() || u.moves <= 0`
         */
        void do_turn( player &u );
        /**
         * Cancels the activity. It may get put into the @ref backlog_.
         * Postcondition: `!has_current()`
         */
        void cancel();
        /**
         * Current activity was aborted (not suspended) from outside,
         * the activity data is essentially lost (the activity is never
         * stored in @ref backlog_).
         * Postcondition: `!has_current()`
         */
        void aborted();
        /**
         * Suspends (if possible, otherwise aborts) the activity. A suspended
         * activity can be resumed, an aborted one can't.
         * Postcondition: `!has_current()`
         */
        void abort();
};
