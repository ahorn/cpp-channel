#include <channel>
#include <functional>
#include <array>

#include <gtest/gtest.h>

// N is the number of philosophers, traditionally five

// For comparison, here's the equivalent machine-readable
// CSP code of ChannelTest::DiningPhilosophersDeadlockFree:
//
// N = 5
// 
// FORK_ID = {0..N-1}
// PHIL_ID = {0..N-1}
// 
// channel picksup, putsdown:FORK_ID.PHIL_ID
// 
// PHIL(i) = picksup!i!i -> picksup!((i+1)%N)!i ->
//   putsdown!((i+1)%N)!i -> putsdown!i!i -> PHIL(i)
// 
// FORK(i) = picksup!i?j -> putsdown!i!j -> FORK(i)
// 
// LPHIL(i) = picksup!((i+1)%N)!i -> picksup!i!i ->
//   putsdown!((i+1)%N)!i -> putsdown!i!i -> LPHIL(i)
// 
// PHILS' = ||| i:PHIL_ID @ if i==0 then LPHIL(0) else PHIL(i)
// 
// SYSTEM = PHILS'[|{|picksup, putsdown|}|]FORKS
template<size_t N>
struct dining_table
{
  std::array<cpp::channel<size_t>, N> picksup;
  std::array<cpp::channel<size_t>, N> putsdown;
};

// Allow fork to be used twice, so it can be picked up and
// put down by the person to the fork's right and left
template<size_t N>
void phil_fork(size_t i, dining_table<N>& t)
{
  t.picksup.at(i).recv();
  t.putsdown.at(i).recv();
  t.picksup.at(i).recv();
  t.putsdown.at(i).recv();
}

// Picks up left and then right fork;
// finally, puts down both forks
template<size_t N>
void phil_person(size_t i, dining_table<N>& t)
{
  t.picksup.at(i).send(i);
  t.picksup.at((i + 1) % N).send(i);
  t.putsdown.at(i).send(i);
  t.putsdown.at((i + 1) % N).send(i);
}

// Picks up right fork then left fork;
// finally, puts down both forks
template<size_t N>
void phil_different_person(size_t i, dining_table<N>& t)
{
  t.picksup.at((i + 1) % N).send(i);
  t.picksup.at(i).send(i);
  t.putsdown.at(i).send(i);
  t.putsdown.at((i + 1) % N).send(i);
}

TEST(CrvFunctionalTest, DiningPhilosophersDeadlockFree)
{
  constexpr size_t N = 5;
  dining_table<N> t;
  std::thread forks[N];
  std::thread phils[N];

  for (int i = 0; i < N; i++)
  {
    forks[i] = std::thread(phil_fork<N>, i, std::ref(t));

    // asymmetric philosophers solution to prevent deadlock
    if (i == 0)
      phils[i] = std::thread(phil_different_person<N>, i, std::ref(t));
    else
      phils[i] = std::thread(phil_person<N>, i, std::ref(t));
  }

  // terminate cleanly
  for (int i = 0; i < N; i++)
  {
    forks[i].join();
    phils[i].join();
  }
}

void thread_a(cpp::channel<char> c)
{
  c.send('A');
  const char r = c.recv();

  EXPECT_EQ('B', r);
}

void thread_b(cpp::channel<char> c)
{
  char r = c.recv();
  c.send('B');

  EXPECT_EQ('A', r);
}

TEST(CrvFunctionalTest, SenderReceiver)
{
  cpp::channel<char> c;

  std::thread a(thread_a, c);
  cpp::thread_guard a_guard(a);

  std::thread b(thread_b, c);
  cpp::thread_guard b_guard(b);
}

void sender_thread_a(cpp::channel<char> c)
{
  c.send('A');
}

void sender_thread_b(cpp::channel<char> c)
{
  c.send('B');
}

void receiver_thread_a(cpp::channel<char> c)
{
  char r = c.recv();
  EXPECT_TRUE(r == 'A' || r == 'B');
}

void receiver_thread_b(cpp::channel<char> c)
{
  char r = c.recv();
  EXPECT_TRUE(r == 'A' || r == 'B');
}

TEST(CrvFunctionalTest, SenderReceiverWithMultipleChannels)
{
  cpp::channel<char> c;

  std::thread sa(sender_thread_a, c);
  cpp::thread_guard sa_guard(sa);

  std::thread sb(sender_thread_b, c);
  cpp::thread_guard sb_guard(sb);

  std::thread ra(receiver_thread_a, c);
  cpp::thread_guard ra_guard(ra);

  std::thread rb(receiver_thread_a, c);
  cpp::thread_guard rb_guard(rb);
}

TEST(ChannelTest, Copy)
{
  cpp::channel<int> c;
  cpp::channel<int> d(c);
  EXPECT_EQ(c, d);
}

TEST(ChannelTest, Assignment)
{
  cpp::channel<int> c;
  cpp::channel<int> d;

  EXPECT_NE(c, d);

  d = c;
  EXPECT_EQ(c, d);
}

TEST(ChannelTest, Cast)
{
  cpp::channel<int> c;
  cpp::ichannel<int> d(c);
  cpp::ochannel<int> e(c);

  EXPECT_EQ(c, d);
  EXPECT_EQ(d, c);
  EXPECT_EQ(c, e);
  EXPECT_EQ(e, c);
}

TEST(ChannelTest, AssignmentWithCast)
{
  cpp::channel<int> c;
  cpp::ichannel<int> d(c);
  cpp::ochannel<int> e(c);

  d = c;
  e = c;
  EXPECT_EQ(c, d);
  EXPECT_EQ(d, c);
  EXPECT_EQ(c, e);
  EXPECT_EQ(e, c);
}

void sender(cpp::channel<int> c)
{
  c.send(7);
}

void receiver(cpp::channel<int> c, bool& done)
{
  int r = c.recv();
  EXPECT_EQ(7, r);
  done = true;
}

TEST(ChannelTest, Channel)
{
  bool done = false;
  cpp::channel<int> c;

  std::thread f(sender, c);
  std::thread g(receiver, c, std::ref(done));

  f.join();
  g.join();

  EXPECT_TRUE(done);
}

void out(cpp::ochannel<int> c)
{
  c.send(7);
}

void in(cpp::ichannel<int> c, bool& done)
{
  int r = c.recv();
  EXPECT_EQ(7, r);
  done = true;
}

TEST(ChannelTest, DirectedChannel)
{
  bool done = false;
  cpp::channel<int> c;

  std::thread f(out, c);
  std::thread g(in, c, std::ref(done));

  f.join();
  g.join();

  EXPECT_TRUE(done);
}

void higher_order(cpp::ichannel<cpp::channel<bool>> c)
{
  cpp::channel<bool> done(c.recv());
  done.send(true);
}

TEST(ChannelTest, HigherOrderChannel)
{
  cpp::channel<cpp::channel<bool>> c;
  cpp::channel<bool> done;

  std::thread f(higher_order, c);
  cpp::thread_guard f_guard(f);

  c.send(done);
  EXPECT_TRUE(done.recv());
}

void higher_order_with_cast(cpp::ichannel<cpp::channel<bool>> c)
{
  cpp::ochannel<bool> done(c.recv());
  done.send(true);
}

TEST(ChannelTest, HigherOrderChannelWithCast)
{
  cpp::channel<cpp::channel<bool>> c;
  cpp::channel<bool> done;

  std::thread f(higher_order_with_cast, c);
  cpp::thread_guard f_guard(f);

  c.send(done);
  EXPECT_TRUE(done.recv());
}

// Unless told by noexcept, the compiler doesn't
// know that this is an exception safe class
struct int_wrapper
{
  int i;

  explicit int_wrapper(int j)
  : i(j) {}

  int_wrapper(const int_wrapper& other)
  : i(other.i) {}
};

void foo_sender(cpp::channel<int_wrapper> c)
{
  c.send(int_wrapper(7));
}

void foo_receiver(cpp::channel<int_wrapper> c, bool& done)
{
  std::unique_ptr<int_wrapper> foo_ptr(c.recv_ptr());
  EXPECT_EQ(7, foo_ptr->i);
  done = true;
}

// use channel<T>::recv_ptr()
TEST(ChannelTest, ExceptionUnsafeClassWithPtr)
{
  bool done = false;
  cpp::channel<int_wrapper> c;

  std::thread f(foo_sender, c);
  std::thread g(foo_receiver, c, std::ref(done));

  f.join();
  g.join();

  EXPECT_TRUE(done);
}

void bar_sender(cpp::channel<int_wrapper> c)
{
  c.send(int_wrapper(7));
}

void bar_receiver(cpp::channel<int_wrapper> c, bool& done)
{
  int_wrapper foo(0);
  c.recv(foo);
  EXPECT_EQ(7, foo.i);
  done = true;
}

// use channel<T>::recv(T&)
TEST(ChannelTest, ExceptionUnsafeClassWithArg)
{
  bool done = false;
  cpp::channel<int_wrapper> c;

  std::thread f(bar_sender, c);
  std::thread g(bar_receiver, c, std::ref(done));

  f.join();
  g.join();

  EXPECT_TRUE(done);
}

void receive_buffered_chars(cpp::channel<char, 3> c)
{
  // nonblocking, elements are received
  // in the order they are sent
  char char_a(c.recv());
  char char_b(c.recv());
  char char_c(c.recv());

  EXPECT_EQ('A', char_a);
  EXPECT_EQ('B', char_b);
  EXPECT_EQ('C', char_c);
}

TEST(ChannelTest, InterThreadAsynchronousChannel)
{
  cpp::channel<char, 3> c;

  // nonblocking
  c.send('A');
  c.send('B');
  c.send('C');

  // receive elements in another thread
  std::thread f(receive_buffered_chars, c);
  cpp::thread_guard f_guard(f);
}

TEST(ChannelTest, IntraThreadAsynchronousChannel)
{
  cpp::channel<char, 3> c;

  // nonblocking
  c.send('A');
  c.send('B');
  c.send('C');

  char char_a(c.recv());
  char char_b(c.recv());
  char char_c(c.recv());

  EXPECT_EQ('A', char_a);
  EXPECT_EQ('B', char_b);
  EXPECT_EQ('C', char_c);
}
