#pragma once

// ------------------------------------------------------------------ *structors

template <
    typename Type
> ::xrn::network::detail::Queue<Type>::Queue() = default;

template <
    typename Type
> ::xrn::network::detail::Queue<Type>::~Queue()
{
    this->clear();
}



// ------------------------------------------------------------------ front

template <
    typename Type
> auto ::xrn::network::detail::Queue<Type>::front() const
    -> const Type&
{
    XRN_DEBUG("locking");
    ::std::scoped_lock lock{ m_mutexQueue };
    XRN_DEBUG("unlocking");
    return m_deque.front();
}

template <
    typename Type
> auto ::xrn::network::detail::Queue<Type>::front()
    -> Type&
{
    XRN_DEBUG("locking");
    ::std::scoped_lock lock{ m_mutexQueue };
    XRN_DEBUG("unlocking");
    return m_deque.front();
}

template <
    typename Type
> void ::xrn::network::detail::Queue<Type>::push_front(
    Type item
)
{
    {
        XRN_DEBUG("locking");
        ::std::scoped_lock lock{ m_mutexQueue };
        m_deque.emplace_front(::std::move(item));
        XRN_DEBUG("unlocking");
    }

    this->notify();
}

template <
    typename Type
> auto ::xrn::network::detail::Queue<Type>::pop_front()
    -> Type
{
    XRN_DEBUG("locking");
    ::std::scoped_lock lock{ m_mutexQueue };
    auto tmpValue{ ::std::move(m_deque.front()) };
    m_deque.pop_front();
    XRN_DEBUG("unlocking");
    return tmpValue;
}

template <
    typename Type
> auto ::xrn::network::detail::Queue<Type>::lockFree_pop_front()
    -> Type
{
    auto tmpValue{ ::std::move(m_deque.front()) };
    m_deque.pop_front();
    return tmpValue;
}

template <
    typename Type
> void ::xrn::network::detail::Queue<Type>::remove_front()
{
    XRN_DEBUG("locking");
    ::std::scoped_lock lock{ m_mutexQueue };
    XRN_DEBUG("unlocking");
    m_deque.pop_front();
}



// ------------------------------------------------------------------ back

template <
    typename Type
> auto ::xrn::network::detail::Queue<Type>::back() const
    -> const Type&
{
    XRN_DEBUG("locking");
    ::std::scoped_lock lock{ m_mutexQueue };
    XRN_DEBUG("unlocking");
    return m_deque.back();
}

template <
    typename Type
> auto ::xrn::network::detail::Queue<Type>::back()
    -> Type&
{
    XRN_DEBUG("locking");
    ::std::scoped_lock lock{ m_mutexQueue };
    XRN_DEBUG("unlocking");
    return m_deque.back();
}

template <
    typename Type
> void ::xrn::network::detail::Queue<Type>::push_back(
    Type item
)
{
    {
        XRN_DEBUG("locking");
        ::std::scoped_lock lock{ m_mutexQueue };
        m_deque.emplace_back(::std::move(item));
        XRN_DEBUG("unlocking");
    }

    this->notify();
}

template <
    typename Type
> auto ::xrn::network::detail::Queue<Type>::pop_back()
    -> Type
{
    XRN_DEBUG("locking");
    ::std::scoped_lock lock{ m_mutexQueue };
    auto tmpValue{ ::std::move(m_deque.back()) };
    m_deque.pop_back();
    XRN_DEBUG("unlocking");
    return tmpValue;
}

template <
    typename Type
> void ::xrn::network::detail::Queue<Type>::remove_back()
{
    XRN_DEBUG("locking");
    ::std::scoped_lock lock{ m_mutexQueue };
    m_deque.pop_back();
    XRN_DEBUG("unlocking");
}



// ------------------------------------------------------------------ helpers

template <
    typename Type
> auto ::xrn::network::detail::Queue<Type>::empty() const
    -> bool
{
    XRN_DEBUG("locking");
    ::std::scoped_lock lock{ m_mutexQueue };
    XRN_DEBUG("unlocking");
    return m_deque.empty();
}

template <
    typename Type
> auto ::xrn::network::detail::Queue<Type>::lockFreeEmpty() const
    -> bool
{
    return m_deque.empty();
}

template <
    typename Type
> auto ::xrn::network::detail::Queue<Type>::count() const
    -> ::std::size_t
{
    XRN_DEBUG("locking");
    ::std::scoped_lock lock{ m_mutexQueue };
    XRN_DEBUG("unlocking");
    return m_deque.size();
}

template <
    typename Type
> auto ::xrn::network::detail::Queue<Type>::lockFreeCount() const
    -> ::std::size_t
{
    return m_deque.size();
}

template <
    typename Type
> void ::xrn::network::detail::Queue<Type>::clear()
{
    XRN_DEBUG("locking");
    ::std::scoped_lock lock{ m_mutexQueue };
    m_deque.clear();
    XRN_DEBUG("unlocking");
}



// ------------------------------------------------------------------ blocking

template <
    typename Type
> auto ::xrn::network::detail::Queue<Type>::getMutex()
    -> ::std::mutex&
{
    return m_mutexQueue;
}

template <
    typename Type
> void ::xrn::network::detail::Queue<Type>::wait() const
{
    ::std::unique_lock lock{ m_mutexBlocker };
    m_blocker.wait(lock);
}

template <
    typename Type
> void ::xrn::network::detail::Queue<Type>::notify() const
{
    ::std::scoped_lock lock{ m_mutexBlocker };
    m_blocker.notify_all();
}
