#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include <glog/logging.h>

namespace wn {

typedef std::function<void()> WorkerTask;

class WorkerQueue {
public:
  /** a worker queue that grows above maxQueueSize will start printing warning
   * messages */
  WorkerQueue(std::string workerName, size_t maxQueueSize = 100)
      : keepGoing_(true), maxQueueSize_(maxQueueSize),
        workerName_(std::move(workerName)), joined_(false),
        threadRunning_(false), key_me_(0) {
    key_me_ = 0;
    pthread_key_create(&key_me_, NULL);
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

  /** enqueue the given task for **sync** execution */
  virtual void enqueueTaskSync(WorkerTask task) {
    if (!task) {
      return;
    }

    // TODO: do we really want this? i don't think so.
    // if (isOnWorkerQueue()) {
    //   task();
    //   return;
    // }

    std::mutex mutex;
    std::condition_variable cv;
    bool finished = false;

    // enqueue the task and wait for it to finish
    enqueueTask([&] {
      // TODO: do we want to catch the exceptions here?

      try {
        task();
      } catch (const std::exception &e) {
        LOG(ERROR) << "enqueueTaskSync:"
        << " name: " << this->name()
        << " e: " << e.what();
      }

      {
        std::lock_guard<std::mutex> lock(mutex);
        finished = true;
      }
      cv.notify_one();
    });

    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [&] { return finished; });
  }

  /** get the worker queue name */
  const std::string &name() const { return workerName_; }

  /** check if currently running on this worker queue (for assertion and
   * debugging) */
  bool isOnWorkerQueue() {
    bool ret = true;
    void *current = pthread_getspecific(key_me_);
    LOG(INFO) << "isOnWorkerQueue: "
              << " key_me_ " << key_me_ << " this: " << this
              << " current: " << current << " this: " << this;
    ret = (current == this);
    return ret;
  }

  /** check if the worker thread is running */
  bool isWorkerQueueRunning() const {
    const bool running = threadRunning_;
    void *current = pthread_getspecific(key_me_);
    LOG(INFO) << "isOnWorkerQueue: "
              << " key_me_ " << key_me_ << " this: " << this
              << " current: " << current << " this: " << this
              << " running: " << std::to_string(threadRunning_);
    return running;
  }

  /** send a stop request to the thread and join the thread */
  void join();

  /** send a stop request to the thread
      (stop the thread runloop) */
  void stop();

  /** join and delete the worker queue */
  virtual ~WorkerQueue() {
    if (key_me_) {
      pthread_key_delete(key_me_);
    }
    this->join();
  }

protected:
  void start();
  void markThread();
  static void *workerRunloop(void *arg);

protected:
  std::unique_ptr<std::thread> thread_;
  bool keepGoing_;
  size_t maxQueueSize_;
  std::string workerName_;
  std::mutex mutex_;
  std::condition_variable condition_;
  std::queue<WorkerTask> taskQueue_;
  std::atomic_bool joined_;
  std::atomic_bool threadRunning_;
  pthread_key_t key_me_;

private: // not-copyable
  WorkerQueue(const WorkerQueue &other);
  WorkerQueue &operator=(const WorkerQueue &other);

public:
  static void test();
};
} // namespace wn
