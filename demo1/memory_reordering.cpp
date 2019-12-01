#include <atomic>
#include <random>
#include <thread>
#include <chrono>
#include <stdio.h>
#include <csignal>

#include "semaphore.h"
#include "gtest/gtest.h"

volatile bool g_shutdown = false;

void ThreadProc(std::function<void()>&& func, Semaphore& begin_sem, Semaphore& end_sem)
{
    // Seed with a real random value, if available
    std::random_device rd;
    std::mt19937 eng(rd()); // Mersenne Twister
    std::uniform_int_distribution<int> dist(0, 7);

    while (!g_shutdown)
    {
        begin_sem.wait();
    
        // Random delay
        while (dist(eng) != 0)
        {
        }

        func();

        end_sem.signal();
    }
}

void SignalHandler(int signal)
{
    if (signal == SIGINT)
    {
        g_shutdown = true;
    }
}

// TSO: W -> R is relaxed  (on different memory locations)
// #StoreLoad memory reordering test
void RunTest()
{
    int A;
    int B;
    int R1;
    int R2;

    Semaphore sem1(0);
    Semaphore sem2(0);
    Semaphore end_sem1(0);
    Semaphore end_sem2(0);

    std::signal(SIGINT, SignalHandler);

    std::thread t1(
        ThreadProc,
        [&]() {
            A = 1;
            R1 = B;
        },
        std::ref(sem1),
        std::ref(end_sem1)
    );
    std::thread t2(
        ThreadProc,
        [&]() {
            B = 1;
            R2 = A;
        },
        std::ref(sem2),
        std::ref(end_sem2)
    );

    t1.detach();
    t2.detach();

    constexpr int total_iterations = 100000;
    int detected = 0;
    auto begin = std::chrono::high_resolution_clock::now();
    for (int iterations = 1; !g_shutdown && iterations < total_iterations; iterations++)
    {
        // Reset A and B
        A = 0;
        B = 0;

        sem1.signal();
        sem2.signal();

        end_sem1.wait();
        end_sem2.wait();

        // Check if there was a simultaneous reorder
        if (R1 == 0 && R2 == 0)
        {
            detected++;
            printf("%d reorders detected after %d iterations\n", detected, iterations);
        }
    }

    if (!g_shutdown)
    {
        g_shutdown = true;
        sem1.signal();
        sem2.signal();

        end_sem1.wait();
        end_sem2.wait();

    }

    auto end = std::chrono::high_resolution_clock::now();

    EXPECT_TRUE(detected > 0);

    printf("Total %d reorders detected after %d iterations\n", detected, total_iterations);
    printf("That's about 1 in every %d execitions or %4.2f%%\n",
        static_cast<int>(total_iterations / static_cast<float>(detected) + 0.5f),
        100 * (detected / static_cast<float>(total_iterations)));

    printf("Total time: %Id64 milliseconds\n",
           std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());
}

TEST(MemoryReordering, Demo1)
{
    RunTest();
}
