#include "WorkerThread.hpp"

void Work1()
{
    std::cout << "Halo 1" << std::endl;
}

void Work2()
{
    std::cout << "Halo 2" << std::endl;
}

void Work3()
{
    std::cout << "Halo 3" << std::endl;
}

int main()
{
    WorkerThread::Init();

    for (size_t i = 0; i < 100; i++)
    {
        if (i % 3 == 0)
            WorkerThread::EnqueueWork(Work1);
        if (i % 3 == 1)
            WorkerThread::EnqueueWork(Work2);
        if (i % 3 == 2)
            WorkerThread::EnqueueWork(Work3);
        Sleep(100);
    }

    // WorkerThread::Destroy();
    Sleep(1000);
    std::cout << "Test" << std::endl;

    for (size_t i = 0; i < 100; i++)
    {
        if (i % 3 == 0)
            WorkerThread::EnqueueWork(Work1);
        if (i % 3 == 1)
            WorkerThread::EnqueueWork(Work2);
        if (i % 3 == 2)
            WorkerThread::EnqueueWork(Work3);
    }

    Sleep(10000);
}