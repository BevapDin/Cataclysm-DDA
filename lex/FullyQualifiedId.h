#pragma once

#include <string>

class FullyQualifiedId {
    private:
        std::string id_;

    public:
        FullyQualifiedId() = default;
        ~FullyQualifiedId() = default;

        /**
         * Combine namespace and identifier withing that namespace into one identifier.
         * Handles an empty namespace separately.
         */
        FullyQualifiedId( const FullyQualifiedId &ns, const std::string &name );
        explicit FullyQualifiedId( const std::string &id );

        const std::string &as_string() const {
            return id_;
        }

        bool operator==( const FullyQualifiedId &rhs ) const {
            return id_ == rhs.id_;
        }
        bool operator!=( const FullyQualifiedId &rhs ) const {
            return !operator==( rhs );
        }
        bool operator<( const FullyQualifiedId &rhs ) const {
            return id_ < rhs.id_;
        }
        /**
         * Yields the last part of the id, that is everything after
         * the final "::". Returns everything if the id does not
         * contain any "::".
         */
        std::string back() const;
        /**
         * Check whether this id is part of the given namespace.
         * Also returns true if this id *is* the given namespace.
         */
        bool is_in_namespace( const FullyQualifiedId &ns ) const;
};

inline std::string operator+( const std::string &lhs, const FullyQualifiedId &rhs )
{
    return lhs + rhs.as_string();
}
inline std::string operator+( const FullyQualifiedId &lhs, const std::string &rhs )
{
    return lhs.as_string() + rhs;
}

inline std::string operator+( const char *const lhs, const FullyQualifiedId &rhs )
{
    return lhs + rhs.as_string();
}
inline std::string operator+( const FullyQualifiedId &lhs, const char *const rhs )
{
    return lhs.as_string() + rhs;
}
