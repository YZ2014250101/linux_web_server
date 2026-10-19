#pragma once
#include "Buffer.h"
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <fcntl.h>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>
namespace me {
class AsyncLogging {
public:
    AsyncLogging(const std::string& basename, off_t rollSize, int flushInterval = 3);
    ~AsyncLogging();
    void start();
    void stop();
    void append(const char* logline, int len);

private:
    void threadFunc();
    ssize_t writeFull(int fd, const char* logline, size_t len);
    typedef me::detail::FormatBuffer<me::detail::kLargeBufferSize> Buffer;
    typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
    typedef BufferVector::value_type BufferPtr;
    int fd_;
    std::string basename_;
    off_t rollSize_;
    int flushInterval_;
    std::atomic<bool> running_ { false };

    BufferVector buffers_;
    BufferPtr currentBuffer_;
    BufferPtr nextBuffer_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::thread thread_;
};

static AsyncLogging* getGlobalAsyncLogging() {
    static AsyncLogging g_asyncLogging("app.log", 100 * 1024 * 1024, 3);
    static bool init = false;
    if (!init) {
        g_asyncLogging.start();
        init = true;
    }
    return &g_asyncLogging;
}
// 输出回调，交给g_output
static void asyncOutputCallback(const char* msg, int len) {
    // 第一次进来自动初始化日志实例
    AsyncLogging* log = getGlobalAsyncLogging();
    log->append(msg, len);
}

}