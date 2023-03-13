#pragma once

// This is an elaborate implementation of macOS dispatch queue
// It always persists the 'thread' that is executing
// That means that 'sync' is always executing on either created thread or in current thread
// Similarly to macOS impl 'async' always ex

#include <deque>
#include <future>

class QueueExecutor {
  public:
    using SyncFn = std::function<void(std::promise<void>&)>;
    using AsyncFn = std::function<void()>;
    using StopFn = std::function<void()>;
    QueueExecutor() = default;

    void start(bool allowSameThreadExec, SyncFn fn = {});
    void sync(SyncFn);
    void async(AsyncFn);
    void asyncOnce(AsyncFn);
    void stop(StopFn = {});

  private:
    class Task {
      public:
        virtual void process() = 0;
    };

    class SyncTask final : public Task {
      public:
        SyncTask(SyncFn fn) : task_(fn) {
        }

        virtual void process() final override {
            if (!stolen_) {
                run();
            } else {
                completion_.get_future().wait();
            }
        }

        void run() {
            task_(completion_);
        }
        void steal() {
            stolen_ = true;
        }
        auto future() {
            return completion_.get_future();
        }

      private:
        // Flag that notifies that task was 'stolen' from 'executor' and it needs to wait for it to finish
        bool stolen_ = false;

        // Packaged task to be executed. Future is used for syncing purposes when necessary
        // If task is 'stolen', executor will use future to ensure caller thread is done
        // If task is not 'stolen', sync caller will check is 'executor' is done
        std::promise<void> completion_;

        SyncFn task_;
    };

    class AsyncTask final : public Task {
      public:
        AsyncTask(AsyncFn fn) : task_(fn) {
        }

        virtual void process() final override {
            task_();
        }

      private:
        AsyncFn task_;
    };

    using TaskPtr = std::shared_ptr<Task>;

    // OpenGL will be unhappy if different thread will attempt to execute the code related to it
    // This flag will force execution on 'executor' even if current thread can be used instead
    bool allowSameThreadExec_ = false;

    // Lots of shimes needed for the tasks queue, built around condvar + deque
    std::condition_variable cv_;
    std::mutex mutex_;
    std::deque<TaskPtr> tasks_;
    bool running_ = false;
    std::atomic_bool acceptsTasks_ = false;

    // The Executor as it goes
    std::thread executor_;

    // Sync for 'start' and 'stop' to avoid weird edge cases
    std::mutex initMutex_;

    void loop();

    void syncInternal(SyncFn fn);
};