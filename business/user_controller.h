#pragma once
#include "reflection.h"
#include "json.h"
#include <string>

class UserController {
public:
    // GET /user
    void GetUser(const HttpRequest& req, HttpResponse& resp) {
        Json j;
        j["id"]   = 1;
        j["name"] = "alice";
        j["age"]  = 25;
        resp.SetJson(j);
    }

    // POST /login
    void Login(const HttpRequest& req, HttpResponse& resp) {
        Json body = req.GetJson();           // ✅ 接收 JSON
        std::string username = body["username"].asString();
        std::string password = body["password"].asString();

        Json result;
        if (username == "alice" && password == "123") {
            result["code"] = 0;
            result["msg"]  = "login success";
            result["data"]["username"] = username;
        } else {
            result["code"] = 1;
            result["msg"]  = "invalid username or password";
        }
        resp.SetJson(result);                // ✅ 返回 JSON
    }

    // POST /register
    void Register(const HttpRequest& req, HttpResponse& resp) {
        Json body = req.GetJson();
        std::string username = body["username"].asString();

        Json result;
        result["code"] = 0;
        result["msg"]  = "register success";
        result["data"]["username"] = username;
        resp.SetJson(result);
    }
};

class OrderController {
public:
    // GET /orders
    void List(const HttpRequest& req, HttpResponse& resp) {
        Json arr;
        for (int i = 1; i <= 3; ++i) {
            Json order;
            order["id"]     = i;
            order["amount"] = i * 100.0;
            order["status"] = "pending";
            arr.push_back(order);
        }
        Json result;
        result["code"] = 0;
        result["data"] = arr;
        resp.SetJson(result);
    }

    // GET /order
    void Detail(const HttpRequest& req, HttpResponse& resp) {
        Json j;
        j["id"]     = 1;
        j["amount"] = 199.5;
        j["status"] = "shipped";
        resp.SetJson(j);
    }
};