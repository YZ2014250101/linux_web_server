#pragma once
#pragma once
#include <cstdint>
#include <string>

namespace me {
///
/// Time stamp in UTC, microseconds resolution.
/// Immutable class, only one int64_t member, recommend pass-by-value
///
class Timestamp {
public:
    /// Construct invalid Timestamp (0)
    Timestamp()
        : microSecondsSinceEpoch_(0) { }

    /// Construct with given microseconds since epoch
    explicit Timestamp(int64_t microSecondsSinceEpochArg)
        : microSecondsSinceEpoch_(microSecondsSinceEpochArg) { }

    void swap(Timestamp& that) { std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_); }

    // default copy / assignment / destructor is ok

    std::string toString() const;
    std::string toFormattedString(bool showMicroseconds = true) const;

    bool valid() const { return microSecondsSinceEpoch_ > 0; }

    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
    time_t secondsSinceEpoch() const { return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond); }

    /// get current UTC time
    static Timestamp now();
    static Timestamp invalid() { return Timestamp(); }

    static Timestamp fromUnixTime(time_t t) { return fromUnixTime(t, 0); }

    static Timestamp fromUnixTime(time_t t, int microseconds) {
        return Timestamp(static_cast<int64_t>(t) * kMicroSecondsPerSecond + microseconds);
    }

    static const int kMicroSecondsPerSecond = 1000 * 1000;

private:
    int64_t microSecondsSinceEpoch_;
};

inline bool operator<(Timestamp lhs, Timestamp rhs) {
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs) {
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

inline bool operator>(Timestamp lhs, Timestamp rhs) { return rhs < lhs; }

inline bool operator<=(Timestamp lhs, Timestamp rhs) { return !(rhs < lhs); }

inline bool operator>=(Timestamp lhs, Timestamp rhs) { return !(lhs < rhs); }

inline bool operator!=(Timestamp lhs, Timestamp rhs) { return !(lhs == rhs); }

/// return (high - low), unit: seconds (double)
inline double timeDifference(Timestamp high, Timestamp low) {
    int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
    return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}

/// add seconds to timestamp, return new timestamp
inline Timestamp addTime(Timestamp timestamp, double seconds) {
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}

} // namespace me
