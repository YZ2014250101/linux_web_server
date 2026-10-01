#pragma once
#include "event_loop.h"
#include <thread>
class SubReactor {
public:
    SubReactor() = default;
    SubReactor(const SubReactor&) = delete;
    SubReactor& operator=(const SubReactor&) = delete;
    ~SubReactor(){
        eventLoop_.stop();
        if(thread_.joinable()){
            thread_.join();
        }
    }
    void start(){
        if(thread_.joinable()){
            return; // 已经启动
        }
        thread_ = std::thread([this](){
            eventLoop_.loop();
        });
    }
    EventLoop* getEventLoop(){
        return &eventLoop_;
    }
private:
    EventLoop eventLoop_;
    std::thread thread_;
};