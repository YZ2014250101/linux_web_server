#pragma once
#include "LogStream.h"
#include <cstdio>
#include <memory>
#include <string>
#include <thread>
#include <vector>
namespace me {
class AsyncLogging {
public:
    AsyncLogging(const std::string& basename, off_t rollSize, int flushInterval = 3);
    ~AsyncLogging();
    void start();
    void append(const char* logline, int len);
private:
    typedef me::detail::FormatBuffer<me::detail::kLargeBufferSize> Buffer;
    typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
    typedef BufferVector::value_type BufferPtr;
    std::string basename_;
    off_t rollSize_;
    int flushInterval_;
    bool running_;
    static std::thread thread_;
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
static void asyncOutputCallback(const char* msg, int len)
{
    // 第一次进来自动初始化日志实例
    AsyncLogging* log = getGlobalAsyncLogging();
    log->append(msg, len);
}
#define LOG_INFO me::LogStream() << LogLevel::INFO << " " << __FILE__ << ":" << __LINE__ << " " << __FUNCTION__ << " "

}