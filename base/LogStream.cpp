#include "LogStream.h"
#include <algorithm>
using namespace me;
using namespace me::detail;
namespace me {
namespace detail {
    const char* levelName[] = { "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL" };
    const char digits[] = "9876543210123456789";
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

} // namespace detail
template <typename T> void LogStream::formatInteger(T v) {
    if (buffer_.avail() >= kMaxNumericSize) {
        size_t len = convert(buffer_.current(), v);
        buffer_.add(len);
    }
}
LogStream& LogStream::operator<<(const me::LogLevel level) {
    const char* str = levelName[level];
    return *this << str;
}
LogStream& LogStream::operator<<(const int num) {
    formatInteger(num);
    return *this;
}
}