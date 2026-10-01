#include "main_reactor.h"
#include "connection.h"
#include "ring_buffer.h"
#include "http/http_request.h"
#include "http/http_response.h"
#include "mvc_controller.h"
#include "reflection.h"
#include <iostream>
#include <memory>

// ========== 业务 Controller ==========
class UserController {
public:
    void GetUser(const HttpRequest& req, HttpResponse& resp) {
        Json j;
        j["id"]   = 1;
        j["name"] = "alice";
        j["age"]  = 25;
        resp.SetJson(j);          
    }

    void Login(const HttpRequest& req, HttpResponse& resp) {
        Json body = req.GetJson();
        std::string username = body["username"].asString();
        std::string password = body["password"].asString();

        Json result;
        if (username == "alice" && password == "123") {
            result["code"] = 0;
            result["msg"]  = "login success";
            result["data"]["username"] = username;
        } else {
            result["code"] = 1;
            result["msg"]  = "invalid";
        }
        resp.SetJson(result);      // ✅ JSON
    }

    void Register(const HttpRequest& req, HttpResponse& resp) {
        std::string username = req.GetForm("username");
        resp.SetHtml("<h1>Register: " + username + "</h1>");
    }
};

class OrderController {
public:
    void List(const HttpRequest& req, HttpResponse& resp) {
        resp.SetHtml("<h1>Order List</h1>");
    }

    void Detail(const HttpRequest& req, HttpResponse& resp) {
        resp.SetHtml("<h1>Order Detail</h1>");
    }
};

// ========== 注册类 + 方法 ==========
REGISTER_METHOD(UserController, GetUser);
REGISTER_METHOD(UserController, Login);
REGISTER_METHOD(UserController, Register);

REGISTER_METHOD(OrderController, List);
REGISTER_METHOD(OrderController, Detail);

// ========== 注册路由 ==========
ADD_ROUTE("/user",     UserController,  GetUser);
ADD_ROUTE("/login",    UserController,  Login);
ADD_ROUTE("/register", UserController,  Register);
ADD_ROUTE("/orders",   OrderController, List);
ADD_ROUTE("/order",    OrderController, Detail);

// ========== Connection 回调 ==========
void setupConnection(std::shared_ptr<Connection> conn) {
    conn->setMessageCallback([](Connection* c, RingBuffer* buf) {
        while (true) {
            // 1. 取所有可读数据
            if (buf->readableLen() == 0) break;
            std::string data = buf->readString(buf->readableLen());

            // 2. 解析 HTTP
            HttpRequest req;
            int ret = req.Parse(data);
            if (ret == 0) break;                 // 不完整
            if (ret < 0) { c->handleClose(); return; } // 出错

            // 3. 处理
            HttpResponse resp;
            Router::Instance().HandleRequest(req, resp);
            c->send(resp.ToString());
        }
    });

    conn->setCloseCallback([](Connection* c) {
        std::cout << "Connection closed fd = " << c->fd() << "\n";
    });
}

// ========== main ==========
int main() {
    try {
        MainReactor server(8080, 4);
        server.setConnectionCallback([](std::shared_ptr<Connection> conn) {
            setupConnection(conn);
        });
        std::cout << "HTTP server on 8080\n";
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}