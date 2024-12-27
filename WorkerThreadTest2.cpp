#include "WorkerThread.hpp"

void Work1()
{
    std::cout << "Halo 1" << std::endl;
    Sleep(1000);
}

void Work2()
{
    std::cout << "Halo 2" << std::endl;
    Sleep(1000);
}

void Work3()
{
    std::cout << "Halo 3" << std::endl;
    Sleep(1000);
}


int main() {
    WorkerThread::Init();

    for (size_t i = 0; i < 100; i++) {
                if (i % 3 == 0)
            WorkerThread::EnqueueWork(Work1);
        if (i % 3 == 1)
            WorkerThread::EnqueueWork(Work2);
        if (i % 3 == 2)
            WorkerThread::EnqueueWork(Work3);
        Sleep(500);
    }

    Sleep(100 * 1000);
}