#pragma once
#include "AsyncLogging.h"
#include "LogLevel.h"
#include "Timestamp.h"
#include "noncopyable.h"
#include <iostream>
#include <string.h>
using namespace std;
namespace me {

using OutputFunc = void (*)(const char*, int);
static OutputFunc g_output = nullptr; // 全局输出函数指针，用于将日志数据写入目标输出

class LogStream : noncopyable {
public:
    LogStream() {
        // 输入时间戳
        *this << Timestamp::now().toFormattedString() << " ";
    }
    // 析构函数，当LogStream对象被销毁时自动调用
    ~LogStream() {
        // 向流中添加一个换行符，确保每条日志记录独占一行
        *this << "\n";
        cout << "LogStream::~LogStream()" << endl;
        if (!g_output) {
            g_output = asyncOutputCallback;
        }
        // 调用全局输出函数g_output，将缓冲区中的日志数据写入目标输出
        // 参数1：缓冲区数据的起始地址
        // 参数2：缓冲区中已使用数据的长度（从起始地址到当前指针位置的字节数）
        g_output(buffer_.data(), static_cast<int>(buffer_.current() - buffer_.data()));
    }
    typedef detail::FormatBuffer<detail::kSmallBufferSize> Buffer;
    LogStream& operator<<(me::LogLevel level);
    LogStream& operator<<(short num);
    LogStream& operator<<(unsigned short num);
    LogStream& operator<<(unsigned int num);
    LogStream& operator<<(long num);
    LogStream& operator<<(unsigned long num);
    LogStream& operator<<(long long num);
    LogStream& operator<<(unsigned long long num);
    LogStream& operator<<(int num);
    LogStream& operator<<(float num) {
        *this << static_cast<double>(num);
        return *this;
    }
    LogStream& operator<<(double num);
    LogStream& operator<<(const void* p);
    LogStream& operator<<(char c) {
        buffer_.append(&c, 1);
        return *this;
    }
    LogStream& operator<<(const std::string& s) {
        buffer_.append(s.data(), s.size());
        return *this;
    }
    /**
     * 重载<<运算符，用于将C风格字符串写入日志流
     * @param str 要写入的C风格字符串指针
     * @return 返回当前LogStream对象的引用，以支持链式调用
     */
    LogStream& operator<<(const char* str) {
        if (str) { // 检查字符串指针是否为空
            buffer_.append(str, strlen(str)); // 如果不为空，将字符串内容追加到缓冲区
        } else {
            buffer_.append("(null)", 6); // 如果为空，追加"(null)"字符串到缓冲区
        }
        return *this; // 返回当前对象的引用，支持链式调用
    }

private:
    template <typename T> void formatInteger(T);

    Buffer buffer_;
    static const int kMaxNumericSize = 48;
};
#define LOG_INFO me::LogStream() << LogLevel::INFO << " " << __FILE__ << ":" << __LINE__ << " " << __FUNCTION__ << " "
#define LOG_WARN me::LogStream() << LogLevel::WARN << " " << __FILE__ << ":" << __LINE__ << " " << __FUNCTION__ << " "
#define LOG_ERROR me::LogStream() << LogLevel::ERROR << " " << __FILE__ << ":" << __LINE__ << " " << __FUNCTION__ << " "
#define LOG_FATAL me::LogStream() << LogLevel::FATAL << " " << __FILE__ << ":" << __LINE__ << " " << __FUNCTION__ << " "

} // namespace me