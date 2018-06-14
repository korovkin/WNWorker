#include "WNWorker.h"

namespace wn
{

void* WorkerQueue::workerRunloop(void* s)
{
    WorkerQueue* This = static_cast<WorkerQueue*>(s);
    This->markThread();

    while (This->keepGoing_) {
        WorkerTask task;

        try {
            std::unique_lock<std::mutex> lock(This->mutex_);
            if (!This->taskQueue_.empty()) {
                task = This->taskQueue_.front();
                This->taskQueue_.pop();
            } else {
                try {
                    This->condition_.wait(lock);
                } catch (std::exception& e) {
                    LOG(ERROR) << "Error: " << e.what();
                }
                continue;
            }
        }
        catch (std::exception& error) {
            LOG(ERROR) << "WorkerQueue: " << This->workerName_ << ", exception, " << error.what();
        }

        if (!task) {
            continue;
        }

        try {
            task();
        } catch (std::exception& error) {
            LOG(ERROR) << "WorkerQueue: " << This->workerName_ << ", task, exception, " << error.what();
        }
    }

    return NULL;
}

void WorkerQueue::start() {
    try {
        auto w = [this]{
            this->workerRunloop(this);
        };
        thread_ = std::make_unique<std::thread>(w);
    }
    catch (std::exception& e) {
        LOG(ERROR) << "WorkerQueue: failed to start: " << e.what();
    }
}

void WorkerQueue::markThread()
{
    int ret = 0;
    pthread_key_create(&key_me_, NULL);
    std::string shortName = workerName_.substr(0, 15);
    ret = pthread_setname_np(shortName.c_str());
    if (ret) {
        LOG(ERROR) << "WorkerQueue: " << this->workerName_ << " failed to pthread_setname_np: " << strerror(ret);
    }
    ret = pthread_setspecific(key_me_, this);
    if (ret) {
        LOG(ERROR) << "WorkerQueue: " << this->workerName_ << " failed to pthread_setspecific: " << strerror(ret);
    }
}

void WorkerQueue::test()
{
    LOG(INFO) << "WorkerQueue: test: start";

    {
        wn::WorkerQueue w("test1");

        std::atomic_int count(0);
        std::string order;

        // using std::move(task):
        auto task = [&] {
            count++;
            order += "a";
        };
        w.enqueueTask(std::move(task));

        // inline lambda (task):
        w.enqueueTask([&] {
            count++;
            order += "b";
        });
        w.join();

        // validate the count
        CHECK_EQ(count, 2);
        // validate the order
        CHECK_EQ(order, "ab");
    }

    {
        wn::WorkerQueue w("test1");

        // not on thread:
        CHECK(w.isOnWorkerQueue() == false);

        // on the worker thread:
        w.enqueueTask([&] { CHECK(w.isOnWorkerQueue()); });
        w.join();
    }

    {
        wn::WorkerQueue w("test1");
        // empty worker queue
        w.join();
    }

    LOG(INFO) << "WorkerQueue: test: success";
}

void WorkerQueue::join() {
    const bool isAlreadyJoined = joined_.exchange(true);
    if (!isAlreadyJoined) {
        this->stop();
        this->thread_->join();
    }
}

void WorkerQueue::stop() {
    this->enqueueTask([this]{
        keepGoing_ = false;
    });
}

}

