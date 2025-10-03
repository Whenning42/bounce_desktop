// Exposes:
//  - time literals e.g. '100 ms'
//  - steady_clock::now() as sc_now()
//  - this_thread::sleep_for() as sleep_for()

#ifndef TIME_ALIASES_H_
#define TIME_ALIASES_H_

#include <thread>

using namespace std::chrono_literals;
using std::this_thread::sleep_for;

inline std::chrono::_V2::steady_clock::time_point sc_now() {
  return std::chrono::steady_clock::now();
}

#endif
