#include "WNWorker.h"

namespace wn {

void *WorkerQueue::workerRunloop(void *s) {
  WorkerQueue *This = static_cast<WorkerQueue *>(s);
  This->markThread();
  This->threadRunning_ = true;
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
        } catch (std::exception &e) {
          LOG(ERROR) << "Error: " << e.what();
        }
        continue;
      }
    } catch (std::exception &error) {
      LOG(ERROR) << "WorkerQueue: " << This->workerName_ << ", exception, "
                 << error.what();
    }

    if (!task) {
      continue;
    }

    try {
      task();
    } catch (std::exception &error) {
      LOG(ERROR) << "WorkerQueue: " << This->workerName_
                 << ", task, exception, " << error.what();
    }
  }
  This->threadRunning_ = false;
  return NULL;
}

void WorkerQueue::start() {
  try {
    auto w = [this] { this->workerRunloop(this); };
    thread_ = std::make_unique<std::thread>(w);
  } catch (std::exception &e) {
    LOG(ERROR) << "WorkerQueue: failed to start: " << e.what();
  }
}

void WorkerQueue::markThread() {
  int ret = 0;
  LOG(INFO) << "markThread: key_me_: " << key_me_;
  const std::string shortName = workerName_.substr(0, 15);
#ifdef __APPLE__
  ret = pthread_setname_np(shortName.c_str());
#else
  ret = pthread_setname_np(pthread_self(), shortName.c_str());
#endif
  if (ret) {
    LOG(ERROR) << "WorkerQueue: " << this->workerName_
               << " failed to pthread_setname_np: " << strerror(ret);
  }
  ret = pthread_setspecific(key_me_, this);
  if (ret) {
    LOG(ERROR) << "WorkerQueue: " << this->workerName_
               << " failed to pthread_setspecific: " << strerror(ret);
  }
}

void WorkerQueue::test() {
  LOG(INFO) << "WorkerQueue: test: start";
  LOG(INFO) << "WorkerQueue: test: 000";
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
  LOG(INFO) << "WorkerQueue: test: 001";
  {
    wn::WorkerQueue w("test1");

    // wait until the thread started and marked
    std::atomic_int started(0);
    w.enqueueTask([&] { started++; });
    while (started == 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    // have to be running by now:
    CHECK(w.isWorkerQueueRunning() == true);
    // not on thread:
    CHECK(w.isOnWorkerQueue() == false);
    // on the worker thread:
    w.enqueueTask([&] { CHECK(w.isOnWorkerQueue() == true); });
    w.join();
    // not running anymore
    CHECK(w.isWorkerQueueRunning() == false);
  }

  LOG(INFO) << "WorkerQueue: test: 002";

  {
    wn::WorkerQueue w("test_sync");
    std::atomic_int count(0);
    const auto N = 5;
    for (int i = 0; i < N; ++i) {
      w.enqueueTaskSync([&, i] {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        LOG(INFO) << "test: test_sync: i: " << i << " count: " << count;
        count += 100;
      });
    }

    w.enqueueTaskSync([&] {
      // all the async tasks are done:
      CHECK_EQ(count, 500);
      count += 1;
      LOG(INFO) << "test: test_sync: final: sync: count: " << count;
    });

    // sync and async tasks are done (without the join)
    CHECK_EQ(count, 501);
    w.join();
  }

  LOG(INFO) << "WorkerQueue: test: 003";
}

void WorkerQueue::join() {
  const bool isAlreadyJoined = joined_.exchange(true);
  if (!isAlreadyJoined) {
    this->stop();
    this->thread_->join();
  }
}

void WorkerQueue::stop() {
  this->enqueueTask([this] { keepGoing_ = false; });
}

} // namespace wn
