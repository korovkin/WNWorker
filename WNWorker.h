#pragma once

#include <atomic>
#include <condition_variable>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <thread>
#include <assert.h>

#include <glog/logging.h>

namespace wn {

typedef std::function<void()> Task;

class Worker {
public:
    Worker(std::string workerName,
           bool autoStart,
           size_t queueSize = 100)
        : thread_(0)
        , keepGoing_(true)
        , queueSize_(queueSize)
        , workerName_(std::move(workerName))
        , joined_(false)
    {
        if (autoStart) {
            this->start();
        }
    }

    virtual ~Worker() {
        if (key_me_) {
            pthread_key_delete(key_me_);
        }
        this->join();
    }

    virtual void enqueueTask(Task task) {
        if (!task) {
            return;
        }

        std::unique_lock<std::mutex> lock(mutex_);
        {
            const size_t currentSize = taskQueue_.size();
            if (taskQueue_.size() >= queueSize_) {
                LOG(WARNING) << "WARN: " << this->name() << ", currentSize, "
                             << currentSize << ", " << queueSize_;
            }
            taskQueue_.emplace(task);
        }

        condition_.notify_all();
    }

    virtual void join() {
        const bool alreadyJoined = joined_.exchange(true);
        if (thread_) {
            if (!alreadyJoined) {
                this->stop();
                pthread_join(thread_, NULL);
            }
            thread_ = 0;
        }
    }

    const std::string& name() const {
        return workerName_;
    }

    bool isCurrentWorker()
    {
        bool ret = true;
        void* current = pthread_getspecific(key_me_);
        ret = (current == this);
        return ret;
    }

    void runloop() {
        Worker::doRunLoop(this);
    }

    bool isKeepGoing() const {
        return this->keepGoing_;
    }

    void stop() {
        this->enqueueTask([this]{
            keepGoing_ = false;
        });
    }

protected:

    void start() {
        try {
            pthread_create(
                    &this->thread_,
                    NULL,
                    Worker::doRunLoop,
                    (void*)this);
        }
        catch (std::exception& e) {
            LOG(ERROR) << "failed to start: " << e.what();
        }
    }

    void nameCurrentThread();
    static void* doRunLoop(void* arg);

protected:
    pthread_t thread_;
    bool keepGoing_;
    size_t queueSize_;
    std::string workerName_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::queue<Task> taskQueue_;
    std::atomic_bool joined_;
    pthread_key_t key_me_;

private: // non-copyable
    Worker(const Worker& other);
    Worker& operator=( const Worker& other);

public:
    static void test();

};
}
