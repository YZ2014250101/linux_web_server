#pragma once
#include <sys/epoll.h>
#include <stdexcept>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <vector>
class EpollWrapper {
public:
    EpollWrapper(const EpollWrapper&)  = delete;
    EpollWrapper& operator=(const EpollWrapper&) = delete;
    EpollWrapper(){
        epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
        if (epoll_fd_ < 0) {
            throw std::runtime_error("Failed to create epoll instance");
        }
    }
    ~EpollWrapper(){
        if(epoll_fd_ >= 0){
            close(epoll_fd_);
        }
    }
    bool addFd(int fd, uint32_t events){
        struct epoll_event ev{};
        ev.events=events;
        ev.data.fd=fd;
        if(epoll_ctl(epoll_fd_,EPOLL_CTL_MOD,fd,&ev)==0){
            return true;
        }
        if (errno != ENOENT) {
            int err = errno;
            std::cerr << "epoll_ctl MOD failed: " << strerror(err) << "\n";
            return false;
        }
        if(epoll_ctl(epoll_fd_,EPOLL_CTL_ADD,fd,&ev)<0){
            int err = errno;
            std::cerr << "Failed to add fd to epoll: " << strerror(err) << "\n";
            return false;
        }
        return true;
    }
    bool delFd(int fd){
        if(epoll_ctl(epoll_fd_,EPOLL_CTL_DEL,fd,nullptr)<0){
            int err = errno;
            std::cerr << "Failed to delete fd from epoll: " << strerror(err) << "\n";
            return false;
        }
        return true;
    }
    int wait(std::vector<epoll_event>& outEvents,int timeout){
        if(outEvents.empty()){
            std::cerr << "wait: outEvents is empty\n";
            return -1;
        }
        while(true){
            int nfds=epoll_wait(epoll_fd_,outEvents.data(),outEvents.size(),timeout);
            if(nfds<0){
                int err = errno;
                if(err==EINTR){
                    continue; // 被信号中断，继续等待
                }
                std::cerr << "epoll_wait error: " << strerror(err) << "\n";
                return -1;
            }
            return nfds;
        }
    }
private:
    int epoll_fd_;
};