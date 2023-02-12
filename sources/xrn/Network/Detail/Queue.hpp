// TODO: lock free thread safe queue

#pragma once



namespace xrn::network::detail {



template <
    typename Type
> class Queue {

public:

    // ------------------------------------------------------------------ *structors

    Queue();

    ~Queue();



    // ------------------------------------------------------------------ front

    [[ nodiscard ]] auto front() const
        -> const Type&;

    [[ nodiscard ]] auto front()
        -> Type&;

    void push_front(
        Type item
    );

    [[ nodiscard ]] auto pop_front()
        -> Type;

    [[ nodiscard ]] auto lockFree_pop_front()
        -> Type;

    void remove_front();



    // ------------------------------------------------------------------ back

    [[ nodiscard ]] auto back() const
        -> const Type&;

    [[ nodiscard ]] auto back()
        -> Type&;

    void push_back(
        Type item
    );

    [[ nodiscard ]] auto pop_back()
        -> Type;

    void remove_back();



    // ------------------------------------------------------------------ helpers

    [[ nodiscard ]] auto empty() const
        -> bool;

    [[ nodiscard ]] auto lockFreeEmpty() const
        -> bool;

    [[ nodiscard ]] auto count() const
        -> ::std::size_t;

    [[ nodiscard ]] auto lockFreeCount() const
        -> ::std::size_t;

    void clear();



    // ------------------------------------------------------------------ blocking

    [[ nodiscard ]] auto getMutex()
        -> ::std::mutex&;

    void wait() const;

    void notify() const;



private:

    ::std::deque<Type> m_deque;

    mutable ::std::mutex m_mutexQueue;
    mutable ::std::mutex m_mutexBlocker;
    mutable ::std::condition_variable m_blocker;

};



} // namespace xrn::network::detail

#include <xrn/Network/Detail/Queue.impl.hpp>
