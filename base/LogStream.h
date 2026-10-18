#pragma once
#include "LogLevel.h"
#include "noncopyable.h"
#include <string.h>
namespace me {

namespace detail {
    const int kSmallBufferSize = 4096;
    const int kLargeBufferSize = 4096 * 1024;

    /**
     * @brief 固定大小的格式化缓冲区模板类
     * @tparam SIZE 缓冲区大小
     */
    template <int SIZE> class FormatBuffer {
    public:
        /**
         * @brief 构造函数，初始化当前指针指向缓冲区起始位置
         */
        FormatBuffer()
            : cur_(data_) { }
        /**
         * @brief 向缓冲区追加数据
         * @param data 要追加的数据指针
         * @param len 数据长度
         */
        void append(const char* data, size_t len) {
            // 检查缓冲区剩余空间是否足够容纳要追加的数据
            if (static_cast<size_t>(avail()) > len) {
                // 复制数据到缓冲区当前位置
                memcpy(cur_, data, len);
                // 更新当前指针位置
                cur_ += len;
            }
        }
        /**
         * @brief 获取缓冲区起始地址
         * @return 缓冲区起始地址的常量指针
         */
        const char* data() const { return data_; }
        /**
         * @brief 获取缓冲区剩余可用空间
         * @return 剩余可用空间的大小
         */
        int avail() const { return static_cast<int>(end() - cur_); }
        char* current() { return cur_; }
        void add(size_t len) { cur_ += len; }

    private:
        /**
         * @brief 获取缓冲区结束位置
         * @return 缓冲区结束位置的常量指针
         */
        const char* end() const { return data_ + SIZE; }
        // 缓冲区数据存储数组
        char data_[SIZE];
        // 指向当前写入位置的指针
        char* cur_;
    };

} // namesoace detail

using OutputFunc = void (*)(const char*, int);
static OutputFunc g_output = nullptr; // 全局输出函数指针，用于将日志数据写入目标输出

class LogStream : noncopyable {
public:
// 析构函数，当LogStream对象被销毁时自动调用
    ~LogStream() {
    // 向流中添加一个换行符，确保每条日志记录独占一行
        *this << "\n";
    // 调用全局输出函数g_output，将缓冲区中的日志数据写入目标输出
    // 参数1：缓冲区数据的起始地址
    // 参数2：缓冲区中已使用数据的长度（从起始地址到当前指针位置的字节数）
        me::g_output(buffer_.data(), static_cast<int>(buffer_.current() - buffer_.data()));
     }
    typedef detail::FormatBuffer<detail::kSmallBufferSize> Buffer;
    LogStream& operator<<(const me::LogLevel level);
    LogStream& operator<<(const int num);
    LogStream& operator<<(const char c) {
        buffer_.append(&c, 1);
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

} // namespace me