#pragma once
#include "http/http_request.h"
#include "http/http_response.h"

class OrderController {
public:
    void list(HttpRequest& req, HttpResponse& resp) {
        resp.setBody("Order List\n");
    }

    void detail(HttpRequest& req, HttpResponse& resp) {
        resp.setBody("Order Detail\n");
    }
};