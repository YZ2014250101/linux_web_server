#pragma once
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
class Socket {
public:
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(): socket_fd_(-1) {}
    explicit Socket(int fd): socket_fd_(fd) {};
    Socket(Socket&& other) noexcept : socket_fd_(other.socket_fd_) {
        other.socket_fd_ = -1;
    }
    Socket& operator=(Socket&&) = delete;
    ~Socket(){
        if(socket_fd_ >= 0){
            close(socket_fd_);
        }
    }
    bool Create(){
        if (socket_fd_ >= 0){
            return false;
        }
        socket_fd_ = socket(AF_INET, SOCK_CLOEXEC | SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (socket_fd_ < 0) return false;
        return true;
    }
    bool Bind(int port){
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port);
        if (bind(socket_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            return false;
        }
        return true;
    }
    bool Listen(int backlog = SOMAXCONN){
        if (listen(socket_fd_, backlog) < 0) {
            return false;
        }
        return true;
    }
    int Accept(sockaddr_in& client_addr){
        socklen_t client_len = sizeof(client_addr);
        return accept4(socket_fd_,
                   reinterpret_cast<sockaddr*>(&client_addr),
                   &client_len,
                   SOCK_CLOEXEC | SOCK_NONBLOCK);
    }
    ssize_t Send(const void* buffer, size_t length){
        return send(socket_fd_, buffer, length, 0);
    }
    ssize_t Recv(void* buffer, size_t length){
        return recv(socket_fd_, buffer, length, 0);
    }
    int fd() const { return socket_fd_; }
    bool SetReuseAddr(bool enable){
        int optval = enable ? 1 : 0;
        return setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == 0;
    }
    bool SetReusePort(bool enable){
        int optval = enable ? 1 : 0;
        return setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) == 0;
    }
    bool SetNonBlocking(bool enable){
        int flags = fcntl(socket_fd_, F_GETFL, 0);
        if (flags < 0) return false;
        if (enable) {
            flags |= O_NONBLOCK;
        } else {
            flags &= ~O_NONBLOCK;
        }
        return fcntl(socket_fd_, F_SETFL, flags) == 0;
    }
    bool SetKeepAlive(bool enable){
        int optval = enable ? 1 : 0;
        return setsockopt(socket_fd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) == 0;
    }
    bool SetTcpNoDelay(bool enable){
        int optval = enable ? 1 : 0;
        return setsockopt(socket_fd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)) == 0;
    }


private:
    int socket_fd_;
};