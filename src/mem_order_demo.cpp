#include <atomic>
#include <random>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <stdio.h>
#include <csignal>

#if defined(WIN32) && defined (USE_FLUSH_PROCESS_WRITE_BUFFER)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

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
    static const int count_max = 1;
    std::mutex mutex_;
    std::condition_variable cv_;
    unsigned long count_;
};

Semaphore g_sem1(0);
Semaphore g_sem2(0);
Semaphore g_end_sem1(0);
Semaphore g_end_sem2(0);

#if USE_ATOMIC
std::atomic<int> g_X, g_Y;
std::atomic<int> g_r1, g_r2;
#else
int g_X, g_Y;
int g_r1, g_r2;
#endif

volatile bool g_shutdown = false;

void f1()
{
    // Seed with a real random value, if available
    std::random_device rd;
    std::mt19937 eng(rd()); // Mersenne Twister
    std::uniform_int_distribution<int> dist(0, 7);

    while (!g_shutdown)
    {
        g_sem1.wait();
    
        // Random delay
        while (dist(eng) != 0)
        {
        }

        g_X = 1;
#if USE_ATOMIC
        g_r1.store(g_Y, std::memory_order_relaxed);
#else
        //std::atomic_thread_fence(std::memory_order_seq_cst);
        //FlushProcessWriteBuffers();
        g_r1 = g_Y;
#endif
        g_end_sem1.signal();
    }
}

void f2()
{
    // Seed with a real random value, if available
    std::random_device rd;
    std::mt19937 eng(rd()); // Mersenne Twister
    std::uniform_int_distribution<int> dist(0, 7);
        
    while (!g_shutdown)
    {
        g_sem2.wait();

        // Random delay
        while (dist(eng) != 0)
        {
        }

        g_Y  = 1;
#if USE_ATOMIC
        g_r2.store(g_X, std::memory_order_relaxed);
#else
        //std::atomic_thread_fence(std::memory_order_seq_cst);
        //FlushProcessWriteBuffers();
        g_r2 = g_X;
#endif
        g_end_sem2.signal();
    }
}

void SignalHandler(int signal)
{
    if (signal == SIGINT)
    {
        g_shutdown = true;
    }
}

int main()
{
    std::signal(SIGINT, SignalHandler);

    std::thread t1(f1);
    std::thread t2(f2);

    t1.detach();
    t2.detach();

    int detected = 0;
    auto begin = std::chrono::high_resolution_clock::now();
    for (int iterations = 1; !g_shutdown && iterations < 100000; iterations++)
    {
        // Reset X and Y
        g_X = 0;
        g_Y = 0;

        g_sem1.signal();
        g_sem2.signal();

        g_end_sem1.wait();
        g_end_sem2.wait();

        //std::chrono::milliseconds dura(100);
        //std::this_thread::sleep_for(dura);

        // Check if there was a simultaneous reorder
        if (g_r1 == 0 && g_r2 == 0)
        {
            detected++;
            printf("%d reorders detected after %d iterations\n", detected, iterations);
            printf("    X=%d, Y=%d, r1=%d, r2=%d\n", g_X, g_Y, g_r1, g_r2);
        }
    }

    if (!g_shutdown)
    {
        g_shutdown = true;
        g_sem1.signal();
        g_sem2.signal();

        g_end_sem1.wait();
        g_end_sem2.wait();

    }

    auto end = std::chrono::high_resolution_clock::now();
    printf("Total time: %d milliseconds\n", std::chrono::duration_cast<std::chrono::milliseconds>(end -begin).count());
}