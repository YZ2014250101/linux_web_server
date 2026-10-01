给你一份 **`README.md`**——**针对你项目的真实情况**——**"高性能 C++ Web 服务器 + MVC + 反射"**。

## `README.md`

```markdown
# Linux Web Server

基于现代 C++ 开发的高性能 Web 服务器，采用 **多 Reactor 架构** + **epoll** + **线程池**，
实现完整的 **HTTP 解析**、**MVC 框架**、**运行时反射** 和 **JSON API**。

## 特性

- **多 Reactor 架构**：MainReactor 负责 accept，多个 SubReactor 处理 IO，充分利用多核
- **epoll + 非阻塞 IO**：高效 IO 多路复用，支持高并发
- **线程池**：业务逻辑异步执行，避免阻塞 IO 线程
- **环形缓冲区（RingBuffer）**：处理 TCP 半包 / 粘包
- **完整 HTTP 解析**：请求行、头部、Body、Query、表单、文件上传
- **MVC 框架**：Router + Controller，路由与业务解耦
- **运行时反射**：类 / 方法自动注册，支持字符串动态调用
- **JSON 序列化 / 反序列化**：轻量级 Json 库，支持嵌套、数组、Unicode
- **静态资源服务**：HTML / CSS / JS / 图片，自动 MIME 类型
- **跨线程安全**：runInLoop / queueInLoop 保证 EventLoop 线程归属

## 架构

```
                    ┌──────────────────────┐
                    │      Client          │
                    └──────────┬───────────┘
                               │ HTTP
                    ┌──────────▼───────────┐
                    │    MainReactor       │  ← accept 新连接
                    │    (主线程)          │
                    └──────────┬───────────┘
                               │ 轮询分发
        ┌──────────────────────┼──────────────────────┐
        │                      │                      │
┌───────▼────────┐   ┌─────────▼────────┐   ┌─────────▼────────┐
│  SubReactor 1  │   │  SubReactor 2    │   │  SubReactor N    │
│  (EventLoop)   │   │  (EventLoop)     │   │  (EventLoop)     │
│  epoll_wait    │   │  epoll_wait      │   │  epoll_wait      │
└───────┬────────┘   └─────────┬────────┘   └─────────┬────────┘
        │                      │                      │
        └──────────────────────┼──────────────────────┘
                               │
                    ┌──────────▼───────────┐
                    │     Connection       │  ← fd + RingBuffer + 回调
                    └──────────┬───────────┘
                               │
                    ┌──────────▼───────────┐
                    │   HttpRequest /      │  ← 解析
                    │   HttpResponse       │
                    └──────────┬───────────┘
                               │
                    ┌──────────▼───────────┐
                    │   Router (路由)       │
                    │   ReflectionMgr      │  ← 反射
                    │   Controller         │  ← 业务
                    └──────────────────────┘
```

## 目录结构

```
.
├── main.cpp                    # 入口
├── CMakeLists.txt              # 构建（可选）
│
├── reactor/                    # Reactor 层
│   ├── event_loop.h            # 事件循环
│   ├── epoll_wrapper.h         # epoll 封装
│   ├── sub_reactor.h           # 子 Reactor
│   ├── main_reactor.h          # 主 Reactor
│   └── thread_pool.h           # 线程池
│
├── connection/                 # 连接层
│   ├── connection.h            # 连接对象
│   └── ring_buffer.h           # 环形缓冲
│
├── http/                       # HTTP 层
│   ├── http_request.h          # 请求
│   └── http_response.h         # 响应
│
├── framework/                  # 框架层
│   ├── reflection.h            # 反射
│   ├── mvc_controller.h        # Router
│   └── json.h                  # JSON 库
│
└── business/                   # 业务层
    └── user_controller.h       # Controller
```

## 快速开始

### 环境

- Linux（Ubuntu 20.04+ / WSL2）
- g++ 9+（支持 C++17）
- pthread

### 编译

```bash
g++ -std=c++17 -pthread -I. main.cpp -o server
./server
```

### 测试

```bash
# 启动服务器
./server

# 另一个终端
curl http://127.0.0.1:8080/user
# {"id":1,"name":"alice","age":25}

curl -X POST http://127.0.0.1:8080/login \
     -H "Content-Type: application/json" \
     -d '{"username":"alice","password":"123"}'
# {"code":0,"data":{"username":"alice"},"msg":"login success"}

curl http://127.0.0.1:8080/orders
# {"code":0,"data":[{"id":1,...}]}
```

### 浏览器访问

```
http://127.0.0.1:8080/user
http://127.0.0.1:8080/orders
```

## 使用示例

### 1. 写一个 Controller

```cpp
class UserController {
public:
    void GetUser(HttpRequest& req, HttpResponse& resp) {
        Json j;
        j["id"]   = 1;
        j["name"] = "alice";
        j["age"]  = 25;
        resp.SetJson(j);
    }

    void Login(HttpRequest& req, HttpResponse& resp) {
        Json body = req.GetJson();
        std::string username = body["username"].asString();

        Json result;
        result["code"] = 0;
        result["data"]["username"] = username;
        resp.SetJson(result);
    }
};
```

### 2. 注册

```cpp
// 反射注册（方法）
REGISTER_METHOD(UserController, GetUser);
REGISTER_METHOD(UserController, Login);

// 路由注册（路径 → 类.方法）
ADD_ROUTE("/user",  UserController, GetUser);
ADD_ROUTE("/login", UserController, Login);
```

### 3. 完成

**不用改任何框架代码**——加方法 → 注册 → 路由——**自动生效**。

## 核心模块

### EventLoop —— 事件循环

每个 `EventLoop` 绑定一个线程，负责 `epoll_wait` 和事件分发。

- `runInLoop` / `queueInLoop`：跨线程安全执行回调
- `eventfd` 唤醒：跨线程通知
- `doPendingFunctors`：锁外执行回调

### Connection —— 连接对象

封装 fd + 读写缓冲 + 生命周期。

- `RingBuffer` 处理半包 / 粘包
- `shared_from_this` 保证回调期间对象存活
- `send` 内部 `runInLoop`，保证线程安全

### Router + 反射 —— MVC 框架

- **Router**：路径 → (类名, 方法名)
- **ReflectionMgr**：类名 + 方法名 → `std::function`
- **宏注册**：`REGISTER_METHOD` + `ADD_ROUTE`

**优点**：加接口不改框架代码，注册即用。

### JSON 库

轻量级 JSON 序列化 / 反序列化：

- `dump()`：对象 → JSON 字符串
- `parse()`：JSON 字符串 → 对象
- 支持 `null` / `bool` / `number` / `string` / `array` / `object`
- 支持 `\uXXXX` Unicode 转义

## 性能

| 测试 | 结果 |
|---|---|
| **本机 `wrk -t4 -c1000 -d10s`** | **35 万 QPS** |
| **跨机器 `bombardier -c100`** | 3 万 QPS |

**本机 `loopback`**——无网络开销——接近服务器能力上限。

## 技术栈

- **C++17**
- **epoll**
- **非阻塞 IO**
- **多 Reactor**
- **线程池**
- **运行时反射**
- **JSON**
- **MVC**

## 开发计划

- [x] Socket 封装
- [x] epoll 封装
- [x] EventLoop
- [x] 线程池
- [x] RingBuffer
- [x] Connection
- [x] MainReactor / SubReactor
- [x] HTTP 解析（GET / POST）
- [x] 表单提交（urlencoded + multipart）
- [x] 文件上传
- [x] 静态资源
- [x] JSON 序列化 / 反序列化
- [x] 反射
- [x] MVC（Router + Controller）
- [ ] 定时器 / 心跳
- [ ] 数据库（MySQL）
- [ ] 缓存（Redis）
- [ ] EPOLLET 优化
- [ ] 多进程 + SO_REUSEPORT

