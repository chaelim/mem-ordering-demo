#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__

#include <mutex>
#include <condition_variable>

class Semaphore
{
public:
    explicit Semaphore(int init_count = count_max) : count_(init_count)
    {
    }

    void wait()
    {
        std::unique_lock<std::mutex> lk(mutex_);
        cv_.wait(lk, [=]{return 0 < count_;});
        --count_;
    }

    void signal()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            ++count_;
        }
        cv_.notify_one();
    }

private:
    static constexpr int count_max = 1;
    std::mutex mutex_;
    std::condition_variable cv_;
    unsigned long count_;
};


#endif // ifndef __SEMAPHORE_H__