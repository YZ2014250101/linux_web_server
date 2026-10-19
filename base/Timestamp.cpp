#include "Timestamp.h"
#include <cstdio>
#include <ctime>
#include <sys/time.h>

namespace me {

Timestamp Timestamp::now() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    int64_t micro = static_cast<int64_t>(tv.tv_sec) * kMicroSecondsPerSecond + tv.tv_usec;
    return Timestamp(micro);
}

// 输出纯数字字符串："1750123456123456"
std::string Timestamp::toString() const {
    char buf[32] = { 0 };
    snprintf(buf, sizeof buf, "%lld", static_cast<long long>(microSecondsSinceEpoch_));
    return buf;
}

// 格式化输出，UTC时间
// showMicroseconds=true: "2025-10-19 08:30:22.123456"
// showMicroseconds=false: "2025-10-19 08:30:22"
std::string Timestamp::toFormattedString(bool showMicroseconds) const {
    char buf[32] = { 0 };
    time_t sec = static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    int64_t usec = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;

    struct tm tm_utc;
    gmtime_r(&sec, &tm_utc); // gmtime_r：转UTC时间，线程安全

    if (showMicroseconds) {
        snprintf(buf, sizeof buf, "%04d-%02d-%02d %02d:%02d:%02d.%06lld", tm_utc.tm_year + 1900, tm_utc.tm_mon + 1,
            tm_utc.tm_mday, tm_utc.tm_hour, tm_utc.tm_min, tm_utc.tm_sec, static_cast<long long>(usec));
    } else {
        snprintf(buf, sizeof buf, "%04d-%02d-%02d %02d:%02d:%02d", tm_utc.tm_year + 1900, tm_utc.tm_mon + 1,
            tm_utc.tm_mday, tm_utc.tm_hour, tm_utc.tm_min, tm_utc.tm_sec);
    }
    return std::string(buf);
}

} // namespace me
