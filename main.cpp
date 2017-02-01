#include <glog/logging.h>

#include "WNWorker.h"

int main(int argc, char** argv)
{
    google::InitGoogleLogging("wnworker");
    gflags::SetVersionString("whynot");
    google::ParseCommandLineFlags(&argc, &argv, true);
    FLAGS_logtostderr = true;

    LOG(INFO) << "hello";
    wn::Worker::test();
    LOG(INFO) << "EXIT_SUCCESS";

    return EXIT_SUCCESS;
}
