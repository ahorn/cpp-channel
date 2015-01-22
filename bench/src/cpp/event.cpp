#include <channel>
#include <vector>
#include <iostream>

static bool g_exit = false;
static std::mutex g_cout_mutex;

// Listen using wait()
void listen_wait(cpp::channel<char> c)
{
  {
    std::lock_guard<std::mutex> lock(g_cout_mutex);
    std::cout << "Starting listen_wait() listener" << std::endl;
  }
  bool exit = false;
  while (!exit) {
    cpp::select().recv(c, [&exit](char c) {
      if (c == '!')
        exit = true;
      std::lock_guard<std::mutex> lock(g_cout_mutex);
      std::cout << c << std::endl;
    }).wait();
  }
  {
    std::lock_guard<std::mutex> lock(g_cout_mutex);
    std::cout << "Exiting listen_wait() listener" << std::endl;
  }
}

// Listen using try_once()
void listen_try_once(cpp::channel<char> c)
{
  {
    std::lock_guard<std::mutex> lock(g_cout_mutex);
    std::cout << "Starting try_once() listener" << std::endl;
  }
  bool exit = false;
  while (!exit) {
    cpp::select().recv(c, [&exit](char c) {
      if (c == '!')
        exit = true;
      std::lock_guard<std::mutex> lock(g_cout_mutex);
      std::cout << c << std::endl;
    }).try_once();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  {
    std::lock_guard<std::mutex> lock(g_cout_mutex);
    std::cout << "Exiting try_once() listener" << std::endl;
  }
}

// Simulation case for slow event waiting
int main(int argc, char * argv[])
{
  constexpr unsigned int thread_count = 10;
  constexpr unsigned int channel_count = 100;

  if (argc != 2)
  {
    std::cout << "Specify either 'wait' or 'try_once'" << std::endl;
    return 1;
  }
  std::string mode(argv[1]);
  if (mode != "wait" && mode != "try_once")
  {
    std::cout << "Specify either 'wait' or 'try_once'" << std::endl;
    return 1;
  }

  std::vector<std::thread> listeners;
  std::vector<cpp::channel<char>> channels;

  for (auto c = 0; c < channel_count; c++)
  {
    cpp::channel<char> events;
    channels.push_back(events);
    for (auto i = 0; i < thread_count; i++)
    {
      if (mode == "wait")
        listeners.push_back(std::thread(listen_wait, events));
      else
        listeners.push_back(std::thread(listen_try_once, events));
    }
  }

  std::string message("Hello World");
  for (auto& c : message)
  {
    for (auto& channel : channels)
    {
      channel.send(c);
    }
    std::this_thread::sleep_for(std::chrono::seconds(3));
  }
  for (auto c = 0; c < channel_count; c++)
  {
    for (auto i = 0; i < thread_count; i++)
    {
      channels.at(c).send('!');
    }
  }
  for (auto& t : listeners)
  {
    if (t.joinable())
      t.join();
  }
  return EXIT_SUCCESS;
}

