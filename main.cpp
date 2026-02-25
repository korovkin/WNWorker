#include <glog/logging.h>
#include <gflags/gflags.h>

#include "WNWorker.h"

int main(int argc, char** argv)
{
    google::InitGoogleLogging("wnworker");
    gflags::SetVersionString("whynot");
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    FLAGS_logtostderr = true;

    LOG(INFO) << "hello";
    wn::WorkerQueue::test();
    LOG(INFO) << "EXIT_SUCCESS";
    return EXIT_SUCCESS;
}
