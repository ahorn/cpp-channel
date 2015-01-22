# Building and testing slow events

Slow event tool shows channel behaviour in extreme cases, with a
lot of channels and listening threads.

Compile ```event``` binary

    g++ -std=c++11 -I./include bench/src/cpp/event.cpp -o event

Run it using either with ```wait``` or ```try_once``` options

   ./event wait
   ./event try_once

