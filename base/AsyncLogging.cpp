#include "AsyncLogging.h"
using namespace me;
namespace me {
AsyncLogging::AsyncLogging(const std::string& basename, off_t rollSize, int flushInterval)
    : basename_(basename),rollSize_(rollSize), flushInterval_(flushInterval){
}
}