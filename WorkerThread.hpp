#include "Winapi.hpp"
#include "SPSCQueue.hpp"
#include <functional>
#include <iostream>

namespace WorkerThread {
    TP_CALLBACK_ENVIRON _environment;
    PTP_POOL _pool = nullptr;
    PTP_CLEANUP_GROUP _cleanupGroup = nullptr;

    void Init() {
        InitializeThreadpoolEnvironment(&_environment);

        _pool = CreateThreadpool(nullptr);

        if (_pool == nullptr)
            throw Winapi::Error("CreateThreadPool failed");

        SetThreadpoolThreadMaximum(_pool, 1);
        if (!SetThreadpoolThreadMinimum(_pool, 1))
            throw Winapi::Error("SetThreadpoolThreadMinimum failed");

        
        _cleanupGroup = CreateThreadpoolCleanupGroup();

        if (_cleanupGroup == nullptr)
            throw Winapi::Error("CreateThreadpoolCleanupGroup failed");

        SetThreadpoolCallbackPool(&_environment, _pool);
        SetThreadpoolCallbackCleanupGroup(&_environment, _cleanupGroup, nullptr);
    }

    using WorkerType = std::function<void()>;
    SPSCQueue<WorkerType> _queue(128);

    size_t cnt = 0;
    VOID CALLBACK _WorkCallback(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_WORK work) {
        std::cout << cnt << ". " << _queue.count() << std::endl;
        WorkerType worker = _queue.pop(); 
        worker();
        std::cout << cnt << ". " << _queue.count() << std::endl;
        cnt++;
    }

    void SubmitWork(WorkerType&& worker) {
        _queue.push(std::move(worker));

        PTP_WORK work = CreateThreadpoolWork(_WorkCallback, nullptr, &_environment);
        SubmitThreadpoolWork(work);
    }

    void Destroy() {
        CloseThreadpoolCleanupGroupMembers(_cleanupGroup, true, NULL);
        CloseThreadpoolCleanupGroup(_cleanupGroup);
        CloseThreadpool(_pool);
    }
}