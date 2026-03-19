#pragma once
#ifndef CATA_SRC_QUEUED_EOCS_H
#define CATA_SRC_QUEUED_EOCS_H

struct queued_eoc {
public:
    effect_on_condition_id eoc;
    time_point time;
    global_variables::impl_t context;
};

struct eoc_compare {
    bool operator()( const queued_eoc &lhs, const queued_eoc &rhs ) const {
        return lhs.time > rhs.time;
    }
};

struct queued_eocs {
    using storage_iter = std::list<queued_eoc>::iterator;

    struct eoc_compare : ::eoc_compare {
        bool operator()( const storage_iter &lhs, const storage_iter &rhs ) const {
            return ::eoc_compare::operator()( *lhs, *rhs );
        }
    };
    std::priority_queue<storage_iter, std::vector<storage_iter>, eoc_compare> queue;
    std::list<queued_eoc> list;

    queued_eocs();

    queued_eocs( const queued_eocs &rhs );
    queued_eocs( queued_eocs &&rhs ) noexcept;

    queued_eocs &operator=( const queued_eocs &rhs );
    queued_eocs &operator=( queued_eocs &&rhs ) noexcept;

    /* std::priority_queue compatibility layer */
    bool empty() const;
    const queued_eoc &top() const;
    void push( const queued_eoc &eoc );
    void pop();
};

#endif // CATA_SRC_QUEUED_EOCS_H
