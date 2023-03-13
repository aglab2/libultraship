#include "queue_executor.h"

void QueueExecutor::start(bool allowSameThreadExec, SyncFn fn) {
    std::lock_guard lck(initMutex_);
    if (running_)
        return;

    running_ = true;
    allowSameThreadExec_ = allowSameThreadExec;
    tasks_.clear();
    executor_ = std::thread{ &QueueExecutor::loop, this };
    if (fn)
        syncInternal(std::move(fn));

    acceptsTasks_ = true;
}

void QueueExecutor::syncInternal(SyncFn fn) {
    auto task = std::make_shared<SyncTask>(std::move(fn));
    std::future<void> future;
    {
        std::unique_lock<std::mutex> lck(mutex_);
        if (allowSameThreadExec_ && tasks_.empty()) {
            task->steal();
        } else {
            future = task->future();
        }
        tasks_.emplace_back(task);
    }

    cv_.notify_one();
    if (future.valid())
        future.get();
    else
        task->run();
}

void QueueExecutor::sync(SyncFn fn) {
    if (!acceptsTasks_)
        return;

    return syncInternal(std::move(fn));
}

void QueueExecutor::async(AsyncFn fn) {
    if (!acceptsTasks_)
        return;

    auto task = std::make_shared<AsyncTask>(std::move(fn));
    {
        std::unique_lock<std::mutex> lck(mutex_);
        tasks_.emplace_back(std::move(task));
    }

    cv_.notify_one();
}

void QueueExecutor::asyncOnce(AsyncFn fn) {
    if (!acceptsTasks_)
        return;

    auto task = std::make_shared<AsyncTask>(std::move(fn));
    {
        std::unique_lock<std::mutex> lck(mutex_);
        if (!tasks_.empty())
            return;

        tasks_.emplace_back(std::move(task));
    }

    cv_.notify_one();
}

void QueueExecutor::stop(StopFn fn) {
    std::lock_guard lck(initMutex_);
    if (!running_)
        return;

    acceptsTasks_ = false;
    syncInternal([&](std::promise<void>& p) {
        if (fn)
            fn();

        running_ = false;
        p.set_value();
    });
    executor_.join();
}

void QueueExecutor::loop() {
    std::unique_lock<std::mutex> lck(mutex_);
    while (running_) {
        if (tasks_.empty())
            cv_.wait(lck, [&] { return !tasks_.empty(); });

        auto task = std::move(tasks_.front());
        lck.unlock();

        task->process();

        lck.lock();
        tasks_.pop_front();
    }
}