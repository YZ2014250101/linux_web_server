#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include "json.h"
// 上传文件结构
struct UploadFile {
    std::string filename;
    std::string content_type;
    std::string content;
};

class HttpRequest {
public:
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> query_params;
    std::map<std::string, std::string> form_params;
    std::vector<UploadFile> files;
    std::string body;

    // 解析原始 HTTP 请求
    // 返回：1 = 成功，0 = 不完整，-1 = 出错
    int Parse(const std::string& raw_data) {
        // 1. 找 "\r\n\r\n"
        size_t header_end = raw_data.find("\r\n\r\n");
        if (header_end == std::string::npos) return 0;   // 不完整

        // 2. 取头部 + body
        std::string header_part = raw_data.substr(0, header_end);
        std::string rest = raw_data.substr(header_end + 4);

        // 3. 解析请求行
        size_t line_end = header_part.find("\r\n");
        std::string request_line = (line_end == std::string::npos)
                                   ? header_part
                                   : header_part.substr(0, line_end);
        if (!ParseRequestLine(request_line)) return -1;

        // 4. 解析头部
        if (line_end != std::string::npos) {
            ParseHeaders(header_part.substr(line_end + 2));
        }

        // 5. 解析 body（按 Content-Length）
        int content_length = 0;
        auto it = headers.find("content-length");
        if (it != headers.end()) {
            try { content_length = std::stoi(it->second); }
            catch (...) { content_length = 0; }
        }

        if (content_length > 0) {
            if (rest.size() < static_cast<size_t>(content_length)) return 0;  // body 不完整
            body = rest.substr(0, content_length);
        } else {
            body = rest;
        }

        // 6. POST 解析表单
        if (method == "POST") ParseBody();

        return 1;
    }

    // 取 header（大小写不敏感）
    std::string GetHeader(const std::string& key) const {
        std::string lower_key = key;
        std::transform(lower_key.begin(), lower_key.end(),
                       lower_key.begin(), ::tolower);
        auto it = headers.find(lower_key);
        return it != headers.end() ? it->second : "";
    }

    // 取 query 参数
    std::string GetQuery(const std::string& key) const {
        auto it = query_params.find(key);
        return it != query_params.end() ? it->second : "";
    }

    // 取 form 参数
    std::string GetForm(const std::string& key) const {
        auto it = form_params.find(key);
        return it != form_params.end() ? it->second : "";
    }
    Json GetJson() const {
        return Json::parse(body);
    }

private:
    // 解析请求行：GET /path?key=val HTTP/1.1
    bool ParseRequestLine(const std::string& line) {
        std::istringstream iss(line);
        if (!(iss >> method >> path >> version)) return false;

        // 分离 path 和 query
        size_t query_pos = path.find('?');
        if (query_pos != std::string::npos) {
            std::string query = path.substr(query_pos + 1);
            path = path.substr(0, query_pos);
            ParseQueryString(query, query_params);
        }
        return true;
    }

    // 解析头部
    void ParseHeaders(const std::string& header_str) {
        std::istringstream iss(header_str);
        std::string line;
        while (std::getline(iss, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;

            size_t pos = line.find(':');
            if (pos == std::string::npos) continue;

            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            // 去掉前导空格
            while (!value.empty() && value.front() == ' ') value.erase(0, 1);
            // 去掉尾部空格
            while (!value.empty() && value.back() == ' ') value.pop_back();

            // key 转小写
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            headers[key] = value;
        }
    }

    // 解析 query string：a=1&b=2&c=3
    static void ParseQueryString(const std::string& query,
                                 std::map<std::string, std::string>& out) {
        size_t start = 0;
        while (start < query.size()) {
            size_t eq = query.find('=', start);
            size_t amp = query.find('&', start);

            if (eq == std::string::npos) break;

            std::string key = query.substr(start, eq - start);
            std::string val = query.substr(
                eq + 1,
                (amp == std::string::npos ? query.size() : amp) - eq - 1
            );

            out[UrlDecode(key)] = UrlDecode(val);

            if (amp == std::string::npos) break;
            start = amp + 1;
        }
    }

    // 解析 body（表单）
    void ParseBody() {
        auto it = headers.find("content-type");
        if (it == headers.end()) return;

        std::string content_type = it->second;

        if (content_type.find("application/x-www-form-urlencoded") != std::string::npos) {
            ParseQueryString(body, form_params);   // ✅ 直接写 form_params
        } else if (content_type.find("multipart/form-data") != std::string::npos) {
            size_t bpos = content_type.find("boundary=");
            if (bpos != std::string::npos) {
                std::string boundary = content_type.substr(bpos + 9);
                // 去掉可能的引号
                if (!boundary.empty() && boundary.front() == '"') boundary.erase(0, 1);
                if (!boundary.empty() && boundary.back()  == '"') boundary.pop_back();
                ParseMultipartFormData(body, boundary);
            }
        }
    }

    // 解析 multipart/form-data
    void ParseMultipartFormData(const std::string& body,
                                const std::string& boundary) {
        std::string delimiter = "--" + boundary;
        size_t pos = 0;

        while ((pos = body.find(delimiter, pos)) != std::string::npos) {
            pos += delimiter.size();
            // 结束标志 "--"
            if (pos + 1 < body.size() && body[pos] == '-' && body[pos + 1] == '-') break;

            // 跳过 "\r\n"
            if (pos + 1 < body.size() && body[pos] == '\r' && body[pos + 1] == '\n') {
                pos += 2;
            }

            size_t part_end = body.find(delimiter, pos);
            if (part_end == std::string::npos) break;

            std::string part = body.substr(pos, part_end - pos);

            size_t header_end = part.find("\r\n\r\n");
            if (header_end == std::string::npos) { pos = part_end; continue; }

            std::string part_headers = part.substr(0, header_end);
            std::string part_content = part.substr(header_end + 4);

            // 去掉末尾的 "\r\n"
            if (part_content.size() >= 2 &&
                part_content.substr(part_content.size() - 2) == "\r\n") {
                part_content = part_content.substr(0, part_content.size() - 2);
            }

            // 解析 Content-Disposition
            size_t name_pos = part_headers.find("name=\"");
            if (name_pos == std::string::npos) { pos = part_end; continue; }
            name_pos += 6;
            size_t name_end = part_headers.find("\"", name_pos);
            if (name_end == std::string::npos) { pos = part_end; continue; }
            std::string field_name = part_headers.substr(name_pos, name_end - name_pos);

            // 看是否文件字段
            size_t file_pos = part_headers.find("filename=\"");
            if (file_pos != std::string::npos) {
                file_pos += 10;
                size_t file_end = part_headers.find("\"", file_pos);
                std::string filename = (file_end == std::string::npos)
                                     ? ""
                                     : part_headers.substr(file_pos, file_end - file_pos);

                // 解析 Content-Type
                std::string ct = "application/octet-stream";
                size_t ct_pos = part_headers.find("Content-Type:");
                if (ct_pos != std::string::npos) {
                    ct_pos += 13;
                    while (ct_pos < part_headers.size() && part_headers[ct_pos] == ' ') ++ct_pos;
                    size_t ct_end = part_headers.find("\r\n", ct_pos);
                    ct = part_headers.substr(ct_pos,
                        (ct_end == std::string::npos ? part_headers.size() : ct_end) - ct_pos);
                }

                files.push_back({filename, ct, part_content});
            } else {
                // 普通表单字段
                form_params[field_name] = part_content;
            }

            pos = part_end;
        }
    }

    // URL 解码：%XX → char，+ → 空格
    static std::string UrlDecode(const std::string& s) {
        std::string result;
        result.reserve(s.size());
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '+') {
                result += ' ';
            } else if (s[i] == '%' && i + 2 < s.size()) {
                int hex = 0;
                bool ok = true;
                for (int j = 1; j <= 2; ++j) {
                    char c = s[i + j];
                    hex <<= 4;
                    if      (c >= '0' && c <= '9') hex |= (c - '0');
                    else if (c >= 'a' && c <= 'f') hex |= (c - 'a' + 10);
                    else if (c >= 'A' && c <= 'F') hex |= (c - 'A' + 10);
                    else { ok = false; break; }
                }
                if (ok) {
                    result += static_cast<char>(hex);
                    i += 2;
                } else {
                    result += s[i];
                }
            } else {
                result += s[i];
            }
        }
        return result;
    }
};

#endif // HTTP_REQUEST_H