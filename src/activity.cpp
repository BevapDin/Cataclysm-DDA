#include "activity.h"

#include <stdexcept>

// Intentionally empty. One can not copy activities. But we usually don't need to anyway.
activity_wrapper::activity_wrapper( const activity_wrapper & )
{
}

activity_wrapper &activity_wrapper::operator=( const activity_wrapper & )
{
    return *this;
}

void activity_wrapper::assign( std::unique_ptr<activity> act )
{
    if( current_ ) {
        throw std::runtime_error( "Can not set an activity when one is currently active" );
    }
    current_ = std::move( act );
}

bool activity_wrapper::resume_with( const activity &act )
{
    if( backlog_.empty() ) {
        return false;
    }
    if( !backlog_.back()->resume_with( act ) ) {
        return false;
    }
    current_ = std::move( backlog_.back() );
    backlog_.pop_back();
    return true;
}

void activity_wrapper::abort()
{
    if( !current_ ) {
        return;
    }
    if( current_->is_suspendable() ) {
        backlog_.emplace_back( std::move( current_ ) );
    }
    current_.reset();
}

void activity_wrapper::aborted()
{
    current_.reset();
}

void activity_wrapper::do_turn( player &u )
{
    if( !current_ ) {
        return;
    }
    switch( current_->do_turn( u ) ) {
        case activity::OnTurnResult::CONTINUE:
            return;
        case activity::OnTurnResult::FINISHED:
            current_.reset();
            return;
        case activity::OnTurnResult::ABORTED:
            // No fall through because it's just coincidentally the same code.
            current_.reset();
            return;
        case activity::OnTurnResult::SUSPENDED:
            backlog_.emplace_back( std::move( current_ ) );
            return;
    }
}

void activity_wrapper::cancel()
{
    // Clear any backlog items that aren't auto-resume.
    //@todo why?
    for( auto iter = backlog_.begin(); iter != backlog_.end(); ) {
        //        if( iter->auto_resume ) {
        //            iter++;
        //        } else {
        iter = backlog_.erase( iter );
        //        }
    }
    if( current_ && current_->is_suspendable() ) {
        backlog_.emplace_back( std::move( current_ ) );
    }
    current_.reset();
}
