#include <channel>
#include <vector>
#include <iostream>

// Send the sequence 2, 3, 4, ..., N to channel 'c'
template<size_t N>
void generate_numbers(cpp::ochannel<unsigned> c)
{
  for (unsigned i = 2; i <= N; i++)
  {
    c.send(i);
  }
}

// Copy number n from channel 'in' to channel 'out' if and
// only if n is not divisible by 'prime' and n < N.
template<size_t N>
void filter_numbers(
  cpp::ichannel<unsigned> in,
  cpp::ochannel<unsigned> out,
  const unsigned prime)
{
  unsigned i;
  do
  {
    i = in.recv();
    if (i % prime != 0)
      out.send(i);
  }
  while (i < N);
}

// The prime sieve up to N: daisy-chain threads together
template<size_t N>
void sieve_numbers(cpp::channel<unsigned> primes)
{
  cpp::channel<unsigned> c;
  std::vector<std::thread> threads;
  threads.emplace_back(generate_numbers<N>, c);

  unsigned prime;
  for (;;)
  {
    prime = c.recv();
    primes.send(prime);

    if (prime >= N)
      break;

    cpp::channel<unsigned> c_prime;
    threads.emplace_back(filter_numbers<N>, c, c_prime, prime);
    c = c_prime;
  }

  for (std::thread& thread : threads)
    thread.join();
}

// Classical inefficient concurrent prime sieve
//
// \see http://blog.onideas.ws/eratosthenes.go
// \see http://golang.org/test/chan/sieve1.go
//
// TODO: use thread pool before reaching conclusions
int main()
{
  constexpr unsigned N = 94321;

  cpp::channel<unsigned> primes;
  std::thread sieve(sieve_numbers<N>, primes);
  cpp::thread_guard sieve_guard(sieve);

  unsigned p;
  do
  {
    p = primes.recv();
    std::cout << p << std::endl;
  }
  while (p < N);

  return EXIT_SUCCESS;
}

