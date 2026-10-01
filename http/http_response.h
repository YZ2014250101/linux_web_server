#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

class HttpResponse {
public:
    // 状态码
    int status_code_ = 200;
    std::map<std::string, std::string> headers_;
    std::string body_;

    HttpResponse() {
        headers_["Server"] = "CppWebFrame/1.0";
        headers_["Connection"] = "keep-alive";
    }

    void SetStatus(int code) { status_code_ = code; }
    void SetHeader(const std::string& key, const std::string& value) {
        headers_[key] = value;
    }
    void SetBody(const std::string& body) { body_ = body; }

    void SetHtml(const std::string& html) {
        body_ = html;
        headers_["Content-Type"] = "text/html; charset=utf-8";
    }

    void SetText(const std::string& text) {
        body_ = text;
        headers_["Content-Type"] = "text/plain; charset=utf-8";
    }

    // 设置静态文件
    bool SetStaticFile(const std::string& file_path) {
        struct stat st;
        if (stat(file_path.c_str(), &st) != 0) return false;
        if (!S_ISREG(st.st_mode)) return false; 

        std::ifstream ifs(file_path, std::ios::binary); 
        if (!ifs.is_open()) return false;

        std::ostringstream oss;
        oss << ifs.rdbuf();
        body_ = oss.str();

        size_t dot = file_path.find_last_of('.');
        std::string ext = (dot != std::string::npos) ? file_path.substr(dot) : "";
        headers_["Content-Type"] = GetContentType(ext);
        return true;
    }

    // 序列化为 HTTP 响应字符串
    std::string ToString() const {
        std::ostringstream oss;
        oss << "HTTP/1.1 " << status_code_ << " " << GetStatusMsg() << "\r\n";

        // 自动 Content-Length
        bool hasLength = headers_.find("Content-Length") != headers_.end();
        if (!hasLength) {
            oss << "Content-Length: " << body_.size() << "\r\n";
        }

        for (auto& p : headers_) {
            oss << p.first << ": " << p.second << "\r\n";
        }

        oss << "\r\n" << body_;
        return oss.str();
    }

    // 常用响应
    static HttpResponse NotFound() {
        HttpResponse resp;
        resp.SetStatus(404);
        resp.SetText("404 Not Found");
        return resp;
    }

    static HttpResponse BadRequest() {
        HttpResponse resp;
        resp.SetStatus(400);
        resp.SetText("400 Bad Request");
        return resp;
    }

    static HttpResponse Ok(const std::string& body = "") {
        HttpResponse resp;
        resp.SetText(body);
        return resp;
    }
    void SetJson(const Json& j) {
        body_ = j.dump();
        headers_["Content-Type"] = "application/json; charset=utf-8";
    }
private:
    std::string GetStatusMsg() const {
        switch (status_code_) {
            case 200: return "OK";
            case 201: return "Created";
            case 204: return "No Content";
            case 301: return "Moved Permanently";
            case 302: return "Found";
            case 304: return "Not Modified";
            case 400: return "Bad Request";
            case 401: return "Unauthorized";
            case 403: return "Forbidden";
            case 404: return "Not Found";
            case 405: return "Method Not Allowed";
            case 500: return "Internal Server Error";
            case 502: return "Bad Gateway";
            case 503: return "Service Unavailable";
            default:  return "Unknown";
        }
    }

    static std::string GetContentType(const std::string& ext) {
        if (ext == ".html" || ext == ".htm") return "text/html; charset=utf-8";
        if (ext == ".css")  return "text/css";
        if (ext == ".js")   return "application/javascript";
        if (ext == ".json") return "application/json; charset=utf-8";
        if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
        if (ext == ".png")  return "image/png";
        if (ext == ".gif")  return "image/gif";
        if (ext == ".svg")  return "image/svg+xml";
        if (ext == ".ico")  return "image/x-icon";
        if (ext == ".txt")  return "text/plain; charset=utf-8";
        if (ext == ".pdf")  return "application/pdf";
        return "application/octet-stream";
    }
};

#endif // HTTP_RESPONSE_H