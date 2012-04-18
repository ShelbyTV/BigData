#ifndef __PARALLELIZE_H__
#define __PARALLELIZE_H__

#include <vector>
#include <thread>

#define PARALLELIZE(numThreads, function, ...)                        \
{                                                                     \
   std::vector<std::thread> parallelizeThreads;                       \
   for (unsigned int parallelizeCounter = 0;                          \
        parallelizeCounter < numThreads;                              \
        parallelizeCounter++) {                                       \
                                                                      \
      parallelizeThreads.push_back(std::thread(function ,             \
                                               ##__VA_ARGS__,         \
                                               parallelizeCounter,    \
                                               numThreads));          \
   }                                                                  \
                                                                      \
   for (unsigned int parallelizeCounter = 0;                          \
        parallelizeCounter < numThreads;                              \
        parallelizeCounter++) {                                       \
                                                                      \
      parallelizeThreads[parallelizeCounter].join();                  \
   }                                                                  \
}

#endif // __PARALLELIZE_H__
