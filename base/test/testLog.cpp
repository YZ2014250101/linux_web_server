#include "../LogStream.h"
#include <iostream>
using namespace me;

int main() {
    auto log = me::getGlobalAsyncLogging();

    // ========== 测试所有 LogStream 重载类型 ==========
    LOG_INFO << "===== 测试各类整数 =====";
    short s = -123;
    unsigned short us = 65000;
    int i = -123456;
    unsigned int ui = 4000000000U;
    long l = -123456789L;
    unsigned long ul = 1234567890123UL;
    long long ll = -9223372036854775807LL;
    unsigned long long ull = 18446744073709551615ULL;
    LOG_INFO << "short=" << s;
    LOG_INFO << "unsigned short=" << us;
    LOG_INFO << "int=" << i;
    LOG_INFO << "unsigned int=" << ui;
    LOG_INFO << "long=" << l;
    LOG_INFO << "unsigned long=" << ul;
    LOG_INFO << "long long=" << ll;
    LOG_INFO << "unsigned long long=" << ull;

    LOG_INFO << "===== 浮点数 float / double =====";
    float f = 3.1415926f;
    double d1 = -0.000000123456;
    double d2 = 123456.789;
    double d3 = 1e20;
    LOG_INFO << "float=" << f;
    LOG_INFO << "double small=" << d1;
    LOG_INFO << "double normal=" << d2;
    LOG_INFO << "double big exp=" << d3;

    LOG_INFO << "===== 指针、char、bool =====";
    const char* str = "test_ptr";
    const void* ptr = &str;
    char ch = 'X';
    bool bTrue = true;
    bool bFalse = false;
    LOG_INFO << "const char*=" << str;
    LOG_INFO << "pointer addr=" << ptr;
    LOG_INFO << "char=" << ch;
    LOG_INFO << "bool true=" << bTrue;
    LOG_INFO << "bool false=" << bFalse;

    LOG_INFO << "===== 混合链式输出（重点测试链式<<） =====";
    LOG_INFO << "mix output, int=" << 233 << ", double=" << 2.718 << ", str=" << "muduo clone";

    // 大量日志压测，触发buffer交换逻辑
    LOG_INFO << "===== 压力测试 200条日志 =====";
    for (int idx = 0; idx < 200; ++idx) {
        LOG_FATAL << "pressure test idx=" << idx << ", val=" << idx * 3.14;
        // std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::cout << "all log write finish" << std::endl;
    return 0;
}
