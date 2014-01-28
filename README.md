# cpp-channel

Go-style concurrency for C++11

## Getting started

To use the library, `#include <channel>`. Here's an example:

```C++
void thread_a(cpp::channel<char> c) {
  c.send('A');
  char r = c.recv();

  assert('B' == r);
}

void thread_b(cpp::channel<char> c) {
  char r = c.recv();
  c.send('B');

  assert('A' == r);
}

int main() {
  cpp::channel<char> c;

  std::thread a(thread_a, c);
  std::thread b(thread_b, c);

  a.join();
  b.join();

  return EXIT_SUCCESS;
}
```

As in Go, `cpp::channel<T, N>` are first-class values. In particular,
channel `c` in the example is passed by value to the newly created threads.

The [source code repository][test] contains several more examples including
the most standard one of them all, Dijkstra's dining philosophers. In fact,
it also shows the use of `std::ref` for a `dining_table` struct as these
are typically not passed by value.

[test]: https://github.com/ahorn/cpp-channel/blob/master/test/channel_test.cpp

## Documentation

A `cpp::channel<T, N>` is like a Go channel created with `make(chan T, N)`.
If `N` is zero, the channel is synchronous; otherwise, it is asynchronous,
as documented in the Go language specification:

* http://golang.org/ref/spec#Channel_types
* http://golang.org/ref/spec#Send_statements
* http://golang.org/ref/spec#Receive_operator

But to simplify the library usage, `cpp::channel` cannot be nil nor closed.

Similar to Go, there are `cpp::ichannel<T, N>` and `cpp::ochannel<T, N>` that
can only receive and send elements of type T, respectively.

Noteworthy, channels are first-class values.  Consequently, we can have
[channels of channels][chan-of-chan].

[chan-of-chan]: http://golang.org/doc/effective_go.html#chan_of_chan

## Installation

You only need a C++11-compliant compiler. There are no other external
dependencies.

To build the library on a (mostly) POSIX-compliant operating system,
execute the following commands from the `cpp-channel` directory:

    $ ./autogen.sh
    $ ./configure
    $ make
    $ make test
    $ make install

If `make test` fails, you can still install, but it is likely that some
features of this library will not work correctly on your system.
Proceed at your own risk.

Note that `make install` may require superuser privileges.

The troubleshooting section below has a few additional tips. For advanced
configuration options refer to the [Autoconf documentation][autoconf].

[autoconf]: http://www.gnu.org/software/autoconf/

## Troubleshooting

If `make test` fails with an error that indicates that `libstdc++.so.6`
or a specific version of `GLIBCXX` cannot be found, then check out GCC's
[FAQ][libstdcxx-faq].

[libstdcxx-faq]: http://gcc.gnu.org/onlinedocs/libstdc++/faq.html#faq.how_to_set_paths

## Bug Reports

You are warmly invited to submit patches as Github pull request,
formatted according to the existing coding style.
