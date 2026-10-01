#pragma once
#include <string>
#include <map>
#include <functional>
#include "http/http_request.h"
#include "http/http_response.h"

using ControllerHandler = std::function<void(HttpRequest&, HttpResponse&)>;

struct ClassMeta {
    std::string name;
    std::map<std::string, ControllerHandler> methods;
};

class ReflectionMgr {
public:
    static ReflectionMgr& Instance() {   
        static ReflectionMgr instance;
        return instance;
    }

    void RegisterClass(const std::string& class_name, ClassMeta class_meta) {
        //                                                    
        class_meta_map[class_name] = std::move(class_meta);  
    }

    ClassMeta* GetClassMeta(const std::string& class_name) {
        auto it = class_meta_map.find(class_name);
        if (it == class_meta_map.end()) return nullptr;
        return &it->second;
    }

    ControllerHandler GetMethod(const std::string& class_name,
                                const std::string& method_name) {
        auto* meta = GetClassMeta(class_name);
        if (!meta) return nullptr;
        auto mit = meta->methods.find(method_name);
        if (mit == meta->methods.end()) return nullptr;
        return mit->second;
    }

private:
    ReflectionMgr() = default;
    ReflectionMgr(const ReflectionMgr&) = delete;
    ReflectionMgr& operator=(const ReflectionMgr&) = delete;

    std::map<std::string, ClassMeta> class_meta_map;
};
//注册方法
#define REGISTER_METHOD(ClassName, MethodName)\
    static bool _reg_method_##ClassName_##MethodName = [](){\
        auto& mgr = ReflectionMgr::Instance();\
        auto* meta = mgr.GetClassMeta(#ClassName);\
        if (!meta){\
            ClassMeta m;\
            m.name = #ClassName;\
            mgr.RegisterClass(#ClassName, std::move(m));\
            meta = mgr.GetClassMeta(#ClassName);\
        }\
        meta->methods[#MethodName] = [](HttpRequest& req, HttpResponse& res){\
            static ClassName obj;\
            obj.MethodName(req, res);\
        };\
        return true;\
    }();