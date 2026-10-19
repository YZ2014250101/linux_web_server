#include "AsyncLogging.h"
#include <cstdio>
#include <memory>
using namespace std;
using namespace me;
namespace me {
AsyncLogging::AsyncLogging(const std::string& basename, off_t rollSize, int flushInterval)
    : basename_(basename)
    , rollSize_(rollSize)
    , flushInterval_(flushInterval) {
    currentBuffer_ = std::make_unique<Buffer>();
    nextBuffer_ = std::make_unique<Buffer>();
    buffers_.reserve(16);
}
AsyncLogging::~AsyncLogging() {
    if (running_) {
        stop();
    }
    if (fd_ >= 0) {

        close(fd_);
        fd_ = -1;
    }
}
void AsyncLogging::append(const char* logline, int len) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (currentBuffer_->avail() > len) {
        currentBuffer_->append(logline, len);
    } else {
        buffers_.push_back(std::move(currentBuffer_));
        if (nextBuffer_) {
            currentBuffer_ = std::move(nextBuffer_);
        } else {
            currentBuffer_ = std::make_unique<Buffer>();
        }
        currentBuffer_->append(logline, len);
        cond_.notify_one(); // 唤醒一个等待的线程将currentBuffer_写入文件
    }
}

void AsyncLogging::start() {
    if (running_)
        return;
    running_ = true;
    thread_ = std::thread(&AsyncLogging::threadFunc, this);
}

void AsyncLogging::stop() {
    running_ = false;
    cond_.notify_one();
    if (thread_.joinable()) {
        thread_.join();
    }
}

ssize_t AsyncLogging::writeFull(int fd, const char* logline, size_t len) {
    size_t written = 0;
    while (written < len) {
        ssize_t n = write(fd, logline + written, len - written);
        if (n < 0) {
            if (errno != EINTR) {
                return n;
            }
            continue;
        }
        written += n;
    }
    return written;
}

/**
 * 异步日志线程的主函数
 * 负责将日志数据写入文件
 */
void AsyncLogging::threadFunc() {
    // 创建两个新的缓冲区
    BufferPtr newBuffer1 = std::make_unique<Buffer>();
    BufferPtr newBuffer2 = std::make_unique<Buffer>();
    // 创建待写入缓冲区向量，并预留空间
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);
    // 以写入、创建、追加模式打开日志文件，设置权限为0644
    fd_ = open(basename_.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    // 如果文件打开失败，直接返回
    if (fd_ < 0)
        return;
    // 当线程正在运行时
    while (running_) {
        {
            // 使用互斥锁保护共享资源
            std::unique_lock<std::mutex> lock(mutex_);
            // 如果当前缓冲区为空，等待条件变量触发或超时
            if (buffers_.empty()) {
                cond_.wait_for(lock, std::chrono::seconds(flushInterval_));
            }
            // 将当前缓冲区移入待写入队列
            buffers_.push_back(std::move(currentBuffer_));
            // 重用已分配的缓冲区
            currentBuffer_ = std::move(newBuffer1);
            // 交换缓冲区队列
            buffersToWrite.swap(buffers_);
            // 如果没有下一个缓冲区，使用第二个预分配的缓冲区
            if (!nextBuffer_) {
                nextBuffer_ = std::move(newBuffer2);
            }
        }
        // 遍历所有待写入的缓冲区，将数据写入文件
        for (const auto& buffer : buffersToWrite) {
            writeFull(fd_, buffer->data(), buffer->length());
        }
        // 如果待写入缓冲区数量超过2个，只保留最新的2个
        if (buffersToWrite.size() > 2) {
            buffersToWrite.resize(2);
        }
        // 回收缓冲区以供重用
        if (!newBuffer1) {
            newBuffer1 = std::move(buffersToWrite.back());
            newBuffer1->reset();
            buffersToWrite.pop_back();
        }
        if (!newBuffer2) {
            newBuffer2 = std::move(buffersToWrite.back());
            newBuffer2->reset();
            buffersToWrite.pop_back();
        }
        // 清空待写入缓冲区
        buffersToWrite.clear();
        if (::fdatasync(fd_) < 0) {
            perror("fdatasync failed");
        }
    }
    {
        std::unique_lock<std::mutex> lock(mutex_);
        buffers_.push_back(std::move(currentBuffer_));
        buffersToWrite.swap(buffers_);
    }
    for (const auto& buffer : buffersToWrite) {
        writeFull(fd_, buffer->data(), buffer->length());
    }
    buffersToWrite.clear();
    if (::fdatasync(fd_) < 0) {
        perror("fdatasync failed");
    }
}
}