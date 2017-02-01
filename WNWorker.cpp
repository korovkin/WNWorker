#include "WNWorker.h"

namespace wn
{

void* Worker::doRunLoop(void* s)
{
    Worker* This = (Worker*)s;
    This->nameCurrentThread();

    while (This->keepGoing_) {
        Task task;
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
            LOG(ERROR)
                << "Worker, " << This->workerName_
                << ", exception, " << error.what();
        }

        if (!task) {
            continue;
        }

        try {
            task();
        } catch (std::exception& error) {
            LOG(ERROR)
                << "Worker, " << This->workerName_
                << ", task, exception, " << error.what();
        }
    }

    return NULL;
}

void Worker::nameCurrentThread()
{
    int ret = 0;
    pthread_key_create(&key_me_, NULL);
    std::string shortName = workerName_.substr(0, 15);
    ret = pthread_setname_np(shortName.c_str());
    if (ret) {
        LOG(ERROR) << "failed to pthread_setname_np: " << strerror(ret);
    }
    ret = pthread_setspecific(key_me_, this);
    if (ret) {
        LOG(ERROR) << "failed to pthread_setspecific: " << strerror(ret);
    }
}

void Worker::test()
{
    // auto start:
    {
        std::atomic_int count(0);
        Worker w("test1", true);
        w.enqueueTask([&]{
            count++;
        });
        w.enqueueTask([&]{
            count++;
        });
        w.join();
        CHECK_EQ(count, 2);
    }

    // no auto start:
    {
        std::atomic_int count(0);
        Worker w("test2", false);
        w.enqueueTask([&]{
            count++;
        });
        w.enqueueTask([&]{
            count++;
        });
        w.enqueueTask([&]{
            w.stop();
        });
        w.runloop();
        w.join();
        CHECK_EQ(count, 2);
    }

    LOG(INFO) << "test: success";
}

}

