#pragma once
#include "sub_reactor.h"
#include "socket.h"
#include "connection.h"
#include <functional>
#include <memory>
#include <iostream>
#include <cstring>

class MainReactor {
public:
    using ConnectionCallback = std::function<void(std::shared_ptr<Connection>)>;

    MainReactor(int port, int subReactorCount)
        : port_(port), subReactors_(subReactorCount)
    {
        if (!listenSocket_.Create())
            throw std::runtime_error("Failed to create listen socket");
        if (!listenSocket_.SetReuseAddr(true))
            throw std::runtime_error("Failed to set SO_REUSEADDR");
        if (!listenSocket_.Bind(port_))
            throw std::runtime_error("Failed to bind listen socket");
        if (!listenSocket_.Listen())
            throw std::runtime_error("Failed to listen on socket");

        mainLoop_.addEvent(listenSocket_.fd(), EPOLLIN, [this]() {
            acceptConnection();
        });

        for (auto& sub : subReactors_) sub.start();
    }

    void start() {
        mainLoop_.loop();
    }

    void setConnectionCallback(ConnectionCallback cb) {
        onConnection_ = std::move(cb);
    }

private:
    std::vector<SubReactor> subReactors_;
    int nextSubIdx_ = 0;
    EventLoop mainLoop_;
    Socket listenSocket_;
    int port_ = 8080;

    ConnectionCallback onConnection_;  

    void acceptConnection() {
        while (true) {
            sockaddr_in clientAddr{};
            int clientFd = listenSocket_.Accept(clientAddr);
            if (clientFd < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return;
                if (errno == EINTR) continue;
                std::cerr << "Accept error: " << strerror(errno) << "\n";
                return;
            }

            std::cout << "Accepted new connection, fd = " << clientFd << "\n";

            SubReactor& subReactor = subReactors_[nextSubIdx_];
            nextSubIdx_ = (nextSubIdx_ + 1) % subReactors_.size();
            EventLoop* subLoop = subReactor.getEventLoop();

            auto conn = std::make_shared<Connection>(subLoop, clientFd);

            // ✅ "交给上层"（上层设所有回调）
            if (onConnection_) {
                onConnection_(conn);
            }

            conn->start();
        }
    }
};