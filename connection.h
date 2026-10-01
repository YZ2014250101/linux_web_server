#pragma once
#include "event_loop.h"
#include "socket.h"
#include "ring_buffer.h"
#include <functional>
#include <string>
#include <memory>
class Connection : public std::enable_shared_from_this<Connection>{
public:
    using MessageCallback = std::function<void(Connection*,RingBuffer*)>;
    using CloseCallback = std::function<void(Connection*)>;
    
    Connection(EventLoop* loop, int fd):
        loop_(loop),
        socket_(fd),
        readBuffer_(65536),
        writeBuffer_(65536) {
    }
    ~Connection() {}
    void start(){
        auto self = shared_from_this();
        loop_->runInLoop([this,self](){
            socket_.SetNonBlocking(true);
            loop_->addEvent(socket_.fd(),EPOLLIN,[this,self](){
                OnRead();
            });
        });
    }
    void OnRead(){
        if(closed_) return;
        char buf[65536];
        while(true){
            ssize_t n = socket_.Recv(buf,sizeof(buf));
            if(n>0){
                readBuffer_.write(buf,n);
            }else if(n==0){
                handleClose();
                return;
            }else{
                if(errno==EAGAIN || errno==EWOULDBLOCK){
                    break; // 没有更多数据可读
                }
                if(errno==EINTR){
                    continue; // 被信号中断，继续读取
                }
                handleClose();
                return;
            }
        }
        if(messageCallback_){
            messageCallback_(this,&readBuffer_);
        }
    }
    void send(const std::string &data){
        if(closed_) return;
        writeBuffer_.write(data.data(),data.size());
        auto self = shared_from_this();
        loop_->runInLoop([this,self](){
            onWrite();
        });
    }
    void onWrite(){
        if(closed_) return;
        char buf[65536];
        auto self = shared_from_this();
        while(writeBuffer_.readableLen()>0){
            size_t n = writeBuffer_.read(buf,sizeof(buf));
            ssize_t sent = socket_.Send(buf,n);
            if(sent<0){
                if(errno==EAGAIN || errno==EWOULDBLOCK){
                    // 不能立即发送，等待下一次可写事件
                    writeBuffer_.write(buf, n);
                    loop_->addEvent(socket_.fd(),EPOLLOUT,[this,self](){
                        onWrite();
                    });
                    return;
                }
                if(errno==EINTR){
                    writeBuffer_.write(buf, n);
                    continue; // 被信号中断，继续发送
                }
                handleClose();
                return;
            }
            if(static_cast<size_t>(sent)<n){
                // 没有发送完，剩余数据继续写入缓冲区
                writeBuffer_.write(buf+sent,n-sent);
                loop_->addEvent(socket_.fd(),EPOLLOUT,[this,self](){
                    onWrite();
                });
                return;
            }
        }
        // 所有数据已发送，取消写事件监听
        loop_->addEvent(socket_.fd(), EPOLLIN, [this,self]() {
            OnRead();
        });
    }
    void handleClose(){
        if(closed_) return;
        closed_ = true;
        loop_->removeEvent(socket_.fd());
        if(closeCallback_){
            closeCallback_(this);
        }
    }
    void setMessageCallback(MessageCallback cb) {
        messageCallback_ = std::move(cb);
    }

    void setCloseCallback(CloseCallback cb) {
        closeCallback_ = std::move(cb);
    }
    int fd() const { return socket_.fd(); }
    std::shared_ptr<Connection> getShared() {
        return shared_from_this();
    }
private:
    EventLoop* loop_;
    Socket socket_;
    RingBuffer readBuffer_;
    RingBuffer writeBuffer_;
    MessageCallback messageCallback_;
    CloseCallback closeCallback_;
    
    bool closed_ = false;
};