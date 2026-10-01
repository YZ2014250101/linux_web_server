#ifndef MVC_CONTROLLER_H
#define MVC_CONTROLLER_H

#include <string>
#include <map>
#include <functional>
#include "http/http_request.h"
#include "http/http_response.h"
#include "reflection.h"

class Router {
public:
    static Router& Instance() {
        static Router instance;
        return instance;
    }

    // 注册路由：路径 -> 类名.方法名
    void AddRoute(const std::string& path,
                  const std::string& class_name,
                  const std::string& method_name) {
        routes_[path] = {class_name, method_name};
    }

    // 处理请求
    void HandleRequest(HttpRequest& req, HttpResponse& resp) {
        auto it = routes_.find(req.path);
        if (it == routes_.end()) {
            resp.SetStatus(404);
            resp.SetHtml("<h1>404 Not Found</h1>");
            return;
        }

        std::string class_name  = it->second.first;
        std::string method_name = it->second.second;

        ControllerHandler handler =
            ReflectionMgr::Instance().GetMethod(class_name, method_name);

        if (!handler) {
            resp.SetStatus(500);
            resp.SetHtml("<h1>500 Internal Server Error</h1>");
            return;
        }

        try {
            handler(req, resp);
        } catch (...) {
            resp.SetStatus(500);
            resp.SetHtml("<h1>500 Internal Server Error</h1>");
        }
    }

private:
    Router() = default;
    std::map<std::string, std::pair<std::string, std::string>> routes_;
};

// 路由注册宏
#define ADD_ROUTE(path, ClassName, MethodName) \
    static bool _add_route_##ClassName##_##MethodName = []() { \
        Router::Instance().AddRoute(path, #ClassName, #MethodName); \
        return true; \
    }();

#endif // MVC_CONTROLLER_H