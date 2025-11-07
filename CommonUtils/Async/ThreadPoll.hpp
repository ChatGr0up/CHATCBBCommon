#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <functional>

namespace CommonUtils {
class ThreadPool {
public:
    ThreadPool(int numThreads);
    ~ThreadPool();
    void enqueueTask(std::function<void()> task);
};
}
#endif // THREADPOOL_HPP