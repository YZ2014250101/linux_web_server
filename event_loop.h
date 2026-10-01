#pragma once
#include "epoll_wrapper.h"
#include <unordered_map>
#include <functional>
#include <atomic>
#include <sys/eventfd.h>
#include <thread>
#include <mutex>
using EventCallback = std::function<void()>;
class EventLoop{
public:
    EventLoop(){
        loopThreadId_ = std::this_thread::get_id();
        wakeup_fd_ = eventfd(0,EFD_NONBLOCK | EFD_CLOEXEC);
        if(wakeup_fd_<0){
            throw std::runtime_error("Failed to create eventfd");
        }
        epoll_.addFd(wakeup_fd_,EPOLLIN);
    }
    ~EventLoop(){
        if (wakeup_fd_ >= 0) close(wakeup_fd_);
    }
    void addEvent(int fd,uint32_t events,EventCallback cb){
        if(!epoll_.addFd(fd,events)){
            std::cerr << "Failed to add fd to epoll\n";
            return;
        }
        callbacks_[fd]=cb;
    }
    void removeEvent(int fd){
        if(!epoll_.delFd(fd)){
            if(errno != ENOENT) {
                std::cerr << "Failed to remove fd from epoll\n";
            }
        }
        callbacks_.erase(fd);
    }
    bool isInLoopThread() const {
        return std::this_thread::get_id() == loopThreadId_;
    }
    void runInLoop(EventCallback cb){
        if(isInLoopThread()){
            cb();
        }else{
            {
                queueInLoop(std::move(cb));
            }
            wakeup();
        }
    }
    void queueInLoop(EventCallback cb){
        {
            std::lock_guard<std::mutex> lock(mutex_);
            pendingFunctors_.push_back(std::move(cb));
        }
        if(!isInLoopThread()){
            wakeup();   
        }
    }
    void wakeup(){
        uint64_t v = 1;
        ssize_t n = write(wakeup_fd_, &v, sizeof(v));
        if (n != sizeof(v)) {
            std::cerr << "Failed to write to wakeup fd\n";
        }
    }
    void doPendingFunctors(){
        std::vector<EventCallback> functors;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            functors.swap(pendingFunctors_);
        }
        for(const auto& cb:functors){
            cb();
        }
    }
    void stop(){
        stop_.store(true);
        wakeup(); // 唤醒事件循环，使其退出
    }
    void loop(){
        loopThreadId_ = std::this_thread::get_id();
        std::vector<epoll_event> events(1024);
        while(!stop_.load()){
            int nfds=epoll_.wait(events,-1);
            if(nfds<0){
                if(errno==EINTR){
                    continue; // 被信号中断，继续等待
                }
                std::cerr << "epoll wait error\n";
                break;
            }
            if (nfds == 0) {
                continue; // 超时，没有事件发生
            }

            for(int i = 0;i<nfds;i++){
                int fd=events[i].data.fd;
                if(fd==wakeup_fd_){
                    uint64_t v;
                    read(wakeup_fd_, &v, sizeof(v)); // 读取唤醒fd的数据，清除事件
                    continue; // 跳过唤醒fd的处理
                }
                auto it=callbacks_.find(fd);
                if(it!=callbacks_.end()){
                    it->second(); // 调用回调函数
                }
            }
            doPendingFunctors(); // 执行待处理的回调函数
        }
    }
private:
    EpollWrapper epoll_;
    std::unordered_map<int,EventCallback> callbacks_;
    std::atomic<bool> stop_{false};
    int wakeup_fd_ = -1; // 用于唤醒事件循环的文件描述符

    std::thread::id loopThreadId_; // 事件循环所在的线程ID
    std::mutex mutex_; // 保护PendingFunctors_的互斥锁
    std::vector<EventCallback> pendingFunctors_; // 待执行的回调函数队列
};