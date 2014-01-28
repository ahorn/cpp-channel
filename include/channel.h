// Copyright 2014, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef CPP_CHANNEL_H
#define CPP_CHANNEL_H

#include <mutex>
#include <deque>
#include <limits>
#include <memory>
#include <thread>
#include <cstddef>
#include <cassert>
#include <type_traits>
#include <condition_variable>

namespace cpp
{

namespace internal
{

// since C++14 in std, see Herb Sutter's blog
template<class T, class ...Args>
std::unique_ptr<T> make_unique(Args&& ...args)
{
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template<class T>
struct _is_exception_safe :
  std::integral_constant<bool,
    std::is_nothrow_copy_constructible<T>::value or
    std::is_nothrow_move_constructible<T>::value>
{};

template<class T, std::size_t N>
class _channel
{
static_assert(N < std::numeric_limits<std::size_t>::max(),
  "N must be strictly less than the largest possible size_t value");

private:
  std::mutex m_mutex;
  std::condition_variable m_send_begin_cv;
  std::condition_variable m_send_end_cv;
  std::condition_variable m_recv_cv;

  std::deque<std::pair</* sender */ std::thread::id, T>> m_buffer;
  bool m_is_send_done;

  template<typename U>
  void _send(U&&);

  void _wait_until_nonempty(std::unique_lock<std::mutex>& lock)
  {
    m_recv_cv.wait(lock, [this]{ return !m_buffer.empty(); });
  }

  bool is_full() const
  {
    return m_buffer.size() > N;
  }

public:
  _channel(const _channel&) = delete;

  // Propagates exceptions thrown by std::condition_variable constructor
  _channel()
  : m_mutex(), m_send_begin_cv(), m_send_end_cv(), m_recv_cv(),
    m_buffer(), m_is_send_done(true) {}

  // Propagates exceptions thrown by std::condition_variable::wait()
  void send(const T& t)
  {
    _send(t);
  }

  // Propagates exceptions thrown by std::condition_variable::wait()
  void send(T&& t)
  {
    _send(std::move(t));
  }

  // Propagates exceptions thrown by std::condition_variable::wait()
  T recv();


  // Propagates exceptions thrown by std::condition_variable::wait()
  void recv(T&);

  // Propagates exceptions thrown by std::condition_variable::wait()
  std::unique_ptr<T> recv_ptr();
};

}

template<typename T, std::size_t N> class ichannel;
template<typename T, std::size_t N> class ochannel;

/// Go-style concurrency

/// Thread synchronization mechanism as in the Go language.
/// As in Go, cpp::channel<T, N> are first-class values.
///
/// Unlike Go, however, cpp::channels<T, N> cannot be nil
/// not closed. This simplifies the usage of the library.
///
/// The template arguments are as follows:
///
/// * T -- type of data to be communicated over channel
/// * N is zero -- synchronous channel
/// * N is positive -- asynchronous channel with buffer size N
///
/// Note that cpp::channel<T, N>::recv() is only supported if T is
/// exception safe. This is automatically checked at compile time.
/// If T is not exception safe, use any of the other receive member
/// functions.
///
/// \see http://golang.org/ref/spec#Channel_types
/// \see http://golang.org/ref/spec#Send_statements
/// \see http://golang.org/ref/spec#Receive_operator
/// \see http://golang.org/doc/effective_go.html#chan_of_chan
template<class T, std::size_t N = 0>
class channel
{
static_assert(N < std::numeric_limits<std::size_t>::max(),
  "N must be strictly less than the largest possible size_t value");

private:
  friend class ichannel<T, N>;
  friend class ochannel<T, N>;

  std::shared_ptr<internal::_channel<T, N>> m_channel_ptr;

public:
  channel(const channel& other) noexcept
  : m_channel_ptr(other.m_channel_ptr) {}

  // Propagates exceptions thrown by std::condition_variable constructor
  channel()
  : m_channel_ptr(std::make_shared<internal::_channel<T, N>>()) {}

  channel& operator=(const channel& other) noexcept
  {
    m_channel_ptr = other.m_channel_ptr;
    return *this;
  }

  bool operator==(const channel& other) const noexcept
  {
    return m_channel_ptr == other.m_channel_ptr;
  }

  bool operator!=(const channel& other) const noexcept
  {
    return m_channel_ptr != other.m_channel_ptr;
  }

  bool operator==(const ichannel<T, N>&) const noexcept;
  bool operator!=(const ichannel<T, N>&) const noexcept;

  bool operator==(const ochannel<T, N>&) const noexcept;
  bool operator!=(const ochannel<T, N>&) const noexcept;

  // Propagates exceptions thrown by std::condition_variable::wait()
  void send(const T& t)
  {
    m_channel_ptr->send(t);
  }

  // Propagates exceptions thrown by std::condition_variable::wait()
  void send(T&& t)
  {
    m_channel_ptr->send(std::move(t));
  }

  // Propagates exceptions thrown by std::condition_variable::wait()
  T recv()
  {
    static_assert(internal::_is_exception_safe<T>::value,
      "Cannot guarantee exception safety, use another recv operator");

    return m_channel_ptr->recv();
  }

  // Propagates exceptions thrown by std::condition_variable::wait()
  std::unique_ptr<T> recv_ptr()
  {
    return m_channel_ptr->recv_ptr();
  }

  // Propagates exceptions thrown by std::condition_variable::wait()
  void recv(T& t)
  {
    m_channel_ptr->recv(t);
  }
};

/// Can only be used to receive elements of type T
template<class T, std::size_t N = 0>
class ichannel
{
private:
  friend class channel<T, N>;
  std::shared_ptr<internal::_channel<T, N>> m_channel_ptr;

public:
  ichannel(const channel<T, N>& other) noexcept
  : m_channel_ptr(other.m_channel_ptr) {}

  ichannel(const ichannel& other) noexcept
  : m_channel_ptr(other.m_channel_ptr) {}

  ichannel& operator=(const ichannel& other) noexcept
  {
    m_channel_ptr = other.m_channel_ptr;
    return *this;
  }

  bool operator==(const ichannel& other) const noexcept
  {
    return m_channel_ptr == other.m_channel_ptr;
  }

  bool operator!=(const ichannel& other) const noexcept
  {
    return m_channel_ptr != other.m_channel_ptr;
  }

  // Propagates exceptions thrown by std::condition_variable::wait()
  T recv()
  {
    static_assert(internal::_is_exception_safe<T>::value,
      "Cannot guarantee exception safety, use another recv operator");

    return m_channel_ptr->recv();
  }

  // Propagates exceptions thrown by std::condition_variable::wait()
  void recv(T& t)
  {
    m_channel_ptr->recv(t);
  }

  // Propagates exceptions thrown by std::condition_variable::wait()
  std::unique_ptr<T> recv_ptr()
  {
    return m_channel_ptr->recv_ptr();
  }

};

/// Can only be used to send elements of type T
template<class T, std::size_t N = 0>
class ochannel
{
private:
  friend class channel<T, N>;
  std::shared_ptr<internal::_channel<T, N>> m_channel_ptr;

public:
  ochannel(const channel<T, N>& other) noexcept
  : m_channel_ptr(other.m_channel_ptr) {}

  ochannel(const ochannel& other) noexcept
  : m_channel_ptr(other.m_channel_ptr) {}

  ochannel& operator=(const ochannel& other) noexcept
  {
    m_channel_ptr = other.m_channel_ptr;
    return *this;
  }

  bool operator==(const ochannel& other) const noexcept
  {
    return m_channel_ptr == other.m_channel_ptr;
  }

  bool operator!=(const ochannel& other) const noexcept
  {
    return m_channel_ptr != other.m_channel_ptr;
  }

  // Propagates exceptions thrown by std::condition_variable::wait()
  void send(const T& t)
  {
    m_channel_ptr->send(t);
  }

  // Propagates exceptions thrown by std::condition_variable::wait()
  void send(T&& t)
  {
    m_channel_ptr->send(std::move(t));
  }
};

template<typename T, size_t N>
bool channel<T, N>::operator==(const ichannel<T, N>& other) const noexcept
{
  return m_channel_ptr == other.m_channel_ptr;
}

template<typename T, size_t N>
bool channel<T, N>::operator!=(const ichannel<T, N>& other) const noexcept
{
  return m_channel_ptr != other.m_channel_ptr;
}

template<typename T, size_t N>
bool channel<T, N>::operator==(const ochannel<T, N>& other) const noexcept
{
  return m_channel_ptr == other.m_channel_ptr;
}

template<typename T, size_t N>
bool channel<T, N>::operator!=(const ochannel<T, N>& other) const noexcept
{
  return m_channel_ptr != other.m_channel_ptr;
}

template<typename T, size_t N>
template<typename U>
void internal::_channel<T, N>::_send(U&& u)
{
  // unlock before notifying threads; otherwise, the
  // notified thread would unnecessarily block again
  {
    // wait (if necessary) until buffer is no longer full and any
    // previous _send() is not blocking the sender any longer
    std::unique_lock<std::mutex> lock(m_mutex);
    m_send_begin_cv.wait(lock, [this]{ return !is_full() && m_is_send_done; });
    m_buffer.emplace_back(std::this_thread::get_id(), std::forward<U>(u));
    m_is_send_done = false;
  }

  // nonblocking
  m_recv_cv.notify_one();

  // wait (if necessary) until u has been received by another thread
  {
    std::unique_lock<std::mutex> lock(m_mutex);

    // It is enough to check !is_full() because m_is_send_done == false.
    // Thus, no other thread could have caused the buffer to fill up
    // during the brief time we didn't own the lock.
    m_send_end_cv.wait(lock, [this]{ return !is_full(); });
    m_is_send_done = true;
  }

  // see also scenario described in _channel<T, N>::recv()
  m_send_begin_cv.notify_one();
}

template<typename T, size_t N>
T internal::_channel<T, N>::recv()
{
  static_assert(internal::_is_exception_safe<T>::value,
    "Cannot guarantee exception safety, use another recv operator");

  std::unique_lock<std::mutex> lock(m_mutex);
  _wait_until_nonempty(lock);

  std::pair<std::thread::id, T> pair(std::move(m_buffer.front()));
  assert(!is_full() || std::this_thread::get_id() != pair.first);

  m_buffer.pop_front();
  assert(!is_full());

  // unlock before notifying threads; otherwise, the
  // notified thread would unnecessarily block again
  lock.unlock();

  // nonblocking
  //
  // Consider two concurrent _send() calls denoted by s and s'.
  // Suppose s waits to enqueue an element (i.e. m_send_begin_cv),
  // whereas s' waits for acknowledgment (i.e. m_send_end_cv) that
  // its previously enqueued element has been dequeued. Before s
  // can proceed, s' must finish. Hence, we notify m_send_end_cv.
  //
  // Now suppose there is no such s'. Then, m_is_send_done == true
  // and m_buffer.size() <= N, i.e. !is_full(). Therefore, s can
  // proceed, as required.
  m_send_end_cv.notify_one();
  return pair.second;
}

template<typename T, size_t N>
void internal::_channel<T, N>::recv(T& t)
{
  // unlock before notifying threads; otherwise, the
  // notified thread would unnecessarily block again
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    _wait_until_nonempty(lock);

    std::pair<std::thread::id, T> pair(std::move(m_buffer.front()));
    assert(!is_full() || std::this_thread::get_id() != pair.first);

    m_buffer.pop_front();
    assert(!is_full());

    t = std::move(pair.second);
  }

  // as illustrated in _channel<T, N>::recv()
  m_send_end_cv.notify_one();
}

template<typename T, size_t N>
std::unique_ptr<T> internal::_channel<T, N>::recv_ptr()
{
  std::unique_lock<std::mutex> lock(m_mutex);
  _wait_until_nonempty(lock);

  std::pair<std::thread::id, T> pair(std::move(m_buffer.front()));
  assert(!is_full() || std::this_thread::get_id() != pair.first);

  m_buffer.pop_front();
  assert(!is_full());

  // TODO: use std when in C++14 mode
  std::unique_ptr<T> t_ptr(make_unique<T>(std::move(pair.second)));

  // unlock before notifying threads; otherwise, the
  // notified thread would unnecessarily block again
  lock.unlock();

  // as illustrated in _channel<T, N>::recv()
  m_send_end_cv.notify_one();
  return t_ptr;
}

}

#endif
