#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <functional>

#include <glog/logging.h>

namespace wn {

typedef std::function<void()> WorkerTask;

class WorkerQueue {
public:
    /** a worker queue that grows above maxQueueSize will start printing warning messages */
    WorkerQueue(std::string workerName, size_t maxQueueSize = 100)
        : keepGoing_(true)
        , maxQueueSize_(maxQueueSize)
        , workerName_(std::move(workerName))
        , joined_(false)
    {
        if (workerName_ == "") {
            LOG(ERROR) << "WorkerQueue: you must name your worker";
        }
        this->start();
    }

    /** enqueue the given task for **async** execution */
    virtual void enqueueTask(WorkerTask task) {
        if (!task) {
            return;
        }

        std::unique_lock<std::mutex> lock(mutex_);
        const size_t currentSize = taskQueue_.size();
        if (taskQueue_.size() >= maxQueueSize_) {
            LOG(WARNING) << "WorkerQueue: WARN: " << this->name() << ", currentSize, "
            << currentSize << ", " << maxQueueSize_;
        }
        taskQueue_.emplace(task);

        condition_.notify_all();
    }

    /** get the worker queue name */
    const std::string& name() const {
        return workerName_;
    }

    /** check if currently running on this worker queue (for assertion and debugging) */
    bool isOnWorkerQueue()
    {
        bool ret = true;
        LOG(INFO) << "isOnWorkerQueue: key_me_: " << key_me_;
        void* current = pthread_getspecific(key_me_);
        LOG(INFO) << "isOnWorkerQueue: "
            << " this: " << this
            << " current: " << current
            << " this: " << this;
        ret = (current == this);
        return ret;
    }

    /** send a stop request to the thread and join the thread */
    void join();

    /** send a stop request to the thread
        (stop the thread runloop) */
    void stop();

    virtual ~WorkerQueue() {
        if (key_me_) {
            pthread_key_delete(key_me_);
        }
        this->join();
    }

protected:
    void start();
    void markThread();
    static void* workerRunloop(void* arg);

protected:
    std::unique_ptr<std::thread> thread_;
    bool keepGoing_;
    size_t maxQueueSize_;
    std::string workerName_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::queue<WorkerTask> taskQueue_;
    std::atomic_bool joined_;
    pthread_key_t key_me_;

private: // not-copyable
    WorkerQueue(const WorkerQueue& other);
    WorkerQueue& operator=( const WorkerQueue& other);

public:
    static void test();

};
}

