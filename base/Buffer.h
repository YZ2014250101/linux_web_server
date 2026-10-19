#pragma once
#include <cstring>
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
        int length() const { return static_cast<int>(cur_ - data_); }
        void reset() { cur_ = data_; }

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
}