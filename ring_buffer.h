#pragma once
#include <vector>
#include <string>
#include <algorithm>

class RingBuffer {
public:
    RingBuffer(size_t capacity) : buf_(capacity), capacity_(capacity) {}

    size_t write(const char* data, size_t len) {
        if (len == 0) return 0;
        size_t writeAvail = capacity_ - readableLen();
        if (writeAvail == 0) return 0;
        size_t realWriteLen = len < writeAvail ? len : writeAvail;
        if (writePos_ >= readPos_) {
            size_t tailSpace = capacity_ - writePos_;
            if (realWriteLen <= tailSpace) {
                std::copy(data, data + realWriteLen, buf_.begin() + writePos_);
            } else {
                std::copy(data, data + tailSpace, buf_.begin() + writePos_);
                std::copy(data + tailSpace, data + realWriteLen, buf_.begin());
            }
        } else {
            std::copy(data, data + realWriteLen, buf_.begin() + writePos_);
        }
        writePos_ = (writePos_ + realWriteLen) % capacity_;
        if (writePos_ == readPos_) isFull = true;
        return realWriteLen;
    }

    size_t read(char* data, size_t len) {
        if (len == 0) return 0;
        size_t readAvail = readableLen();
        if (readAvail == 0) return 0;
        size_t realReadLen = len < readAvail ? len : readAvail;
        if (readPos_ < writePos_) {
            std::copy(buf_.begin() + readPos_, buf_.begin() + readPos_ + realReadLen, data);
        } else {
            size_t tailSpace = capacity_ - readPos_;
            if (realReadLen <= tailSpace) {
                std::copy(buf_.begin() + readPos_, buf_.begin() + readPos_ + realReadLen, data);
            } else {
                std::copy(buf_.begin() + readPos_, buf_.end(), data);
                std::copy(buf_.begin(), buf_.begin() + (realReadLen - tailSpace), data + tailSpace);
            }
        }
        readPos_ = (readPos_ + realReadLen) % capacity_;
        isFull = false;
        return realReadLen;
    }

    void reset() {
        readPos_ = 0;
        writePos_ = 0;
        isFull = false;
    }

    size_t readableLen() const {
        if (isFull) return capacity_;
        if (writePos_ >= readPos_) return writePos_ - readPos_;
        return capacity_ - readPos_ + writePos_;
    }

    size_t writableLen() const {
        return capacity_ - readableLen();
    }

    size_t find(char c) const {
        size_t len = readableLen();
        for (size_t i = 0; i < len; ++i) {
            size_t pos = (readPos_ + i) % capacity_;
            if (buf_[pos] == c) return i;
        }
        return std::string::npos;
    }

    size_t findCRLF() const {
        size_t len = readableLen();
        for (size_t i = 0; i + 1 < len; ++i) {   
            size_t pos = (readPos_ + i) % capacity_;
            if (buf_[pos] == '\r' && buf_[(pos + 1) % capacity_] == '\n')
                return i;
        }
        return std::string::npos;  
    }

    size_t findCRLFCRLF() const {
        size_t len = readableLen();
        for (size_t i = 0; i + 3 < len; ++i) { 
            size_t pos = (readPos_ + i) % capacity_;
            if (buf_[pos] == '\r' &&
                buf_[(pos + 1) % capacity_] == '\n' &&
                buf_[(pos + 2) % capacity_] == '\r' &&
                buf_[(pos + 3) % capacity_] == '\n')
                return i;
        }
        return std::string::npos;  
    }

    std::string readString(size_t n) {
        std::string result(n, '\0');
        read(&result[0], n);
        return result;
    }

    std::string readLine() {
        size_t pos = find('\n');
        if (pos == std::string::npos) return "";
        std::string line = readString(pos + 1);
        if (!line.empty() && line.back() == '\n') line.pop_back();
        if (!line.empty() && line.back() == '\r') line.pop_back();
        return line;
    }

    void retrieve(size_t n) {
        readPos_ = (readPos_ + n) % capacity_;
        isFull = false;
    }

private:
    std::vector<char> buf_;
    size_t readPos_ = 0;
    size_t writePos_ = 0;
    size_t capacity_ = 0;
    bool isFull = false;
};