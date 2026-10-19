#include "LogStream.h"
#include <algorithm>
using namespace me;
using namespace me::detail;
namespace me {
namespace detail {
    const char* levelName[] = { "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL" };
    const char digits[] = "9876543210123456789";
    const char digitsHex[] = "0123456789ABCDEF";
    const char* zero = digits + 9;
    template <typename T> size_t convert(char* buf, T value) {
        T i = value;
        char* p = buf;
        do {
            int lsd = static_cast<int>(i % 10);
            i /= 10;
            *p++ = zero[lsd];
        } while (i != 0);
        if (value < 0) {
            *p++ = '-';
        }
        *p = '\0';
        std::reverse(buf, p);
        return static_cast<size_t>(p - buf);
    }

    size_t convertHex(char* buf, uintptr_t value) {
        uintptr_t i = value;
        char* p = buf;

        do {
            int lsd = static_cast<int>(i % 16);
            i /= 16;
            *p++ = digitsHex[lsd];
        } while (i != 0);

        *p = '\0';
        std::reverse(buf, p);

        return p - buf;
    }

} // namespace detail
template <typename T> void LogStream::formatInteger(T v) {
    if (buffer_.avail() >= kMaxNumericSize) {
        size_t len = convert(buffer_.current(), v);
        buffer_.add(len);
    }
}
LogStream& LogStream::operator<<(short num) {
    *this << static_cast<int>(num);
    return *this;
}
LogStream& LogStream::operator<<(unsigned short num) {
    *this << static_cast<int>(num);
    return *this;
}
LogStream& LogStream::operator<<(unsigned int num) {
    formatInteger(num);
    return *this;
}
LogStream& LogStream::operator<<(long num) {
    formatInteger(num);
    return *this;
}
LogStream& LogStream::operator<<(unsigned long num) {
    formatInteger(num);
    return *this;
}
LogStream& LogStream::operator<<(long long num) {
    formatInteger(num);
    return *this;
}
LogStream& LogStream::operator<<(unsigned long long num) {
    formatInteger(num);
    return *this;
}
LogStream& LogStream::operator<<(me::LogLevel level) {
    const char* str = levelName[level];
    return *this << str;
}
LogStream& LogStream::operator<<(int num) {
    formatInteger(num);
    return *this;
}
LogStream& LogStream::operator<<(const void* p) {
    uintptr_t v = reinterpret_cast<uintptr_t>(p);
    if (buffer_.avail() >= kMaxNumericSize) {
        char* buf = buffer_.current();
        buf[0] = '0';
        buf[1] = 'x';
        size_t len = convertHex(buf + 2, v);
        buffer_.add(len + 2);
    }
    return *this;
}
LogStream& LogStream::operator<<(double v) {
    if (buffer_.avail() >= kMaxNumericSize) {
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", v);
        buffer_.add(len);
    }
    return *this;
}
}