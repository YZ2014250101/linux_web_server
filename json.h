#ifndef JSON_H
#define JSON_H

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <stdexcept>

class Json {
public:
    enum Type { Null, Bool, Number, String, Array, Object };

    Json() : type_(Null) {}
    Json(bool v) : type_(Bool), bool_val_(v) {}
    Json(int v) : type_(Number), num_val_(v) {}
    Json(double v) : type_(Number), num_val_(v) {}
    Json(const std::string& v) : type_(String), str_val_(v) {}
    Json(const char* v) : type_(String), str_val_(v) {}

    // 类型判断
    bool isNull()   const { return type_ == Null; }
    bool isBool()   const { return type_ == Bool; }
    bool isNumber() const { return type_ == Number; }
    bool isString() const { return type_ == String; }
    bool isArray()  const { return type_ == Array; }
    bool isObject() const { return type_ == Object; }

    // 取值
    bool asBool() const { return bool_val_; }
    double asDouble() const { return num_val_; }
    int asInt() const { return static_cast<int>(num_val_); }
    const std::string& asString() const { return str_val_; }

    // 大小
    size_t size() const {
        if (type_ == Array)  return arr_val_.size();
        if (type_ == Object) return obj_val_.size();
        return 0;
    }

    // 写对象
    Json& operator[](const std::string& key) {
        type_ = Object;
        return obj_val_[key];
    }

    // 读对象（const）
    const Json& operator[](const std::string& key) const {
        auto it = obj_val_.find(key);
        if (it == obj_val_.end()) return nullValue();
        return it->second;
    }

    // 写数组
    void push_back(const Json& item) {
        type_ = Array;
        arr_val_.push_back(item);
    }

    // 读数组
    const Json& operator[](size_t index) const {
        if (index >= arr_val_.size()) return nullValue();
        return arr_val_[index];
    }

    // 序列化
    std::string dump() const {
        std::ostringstream oss;
        switch (type_) {
            case Null:   oss << "null"; break;
            case Bool:   oss << (bool_val_ ? "true" : "false"); break;
            case Number: oss << num_val_; break;
            case String: oss << '"' << escape(str_val_) << '"'; break;
            case Array: {
                oss << '[';
                for (size_t i = 0; i < arr_val_.size(); ++i) {
                    if (i > 0) oss << ',';
                    oss << arr_val_[i].dump();
                }
                oss << ']';
                break;
            }
            case Object: {
                oss << '{';
                size_t i = 0;
                for (auto& p : obj_val_) {
                    if (i > 0) oss << ',';
                    oss << '"' << escape(p.first) << "\":" << p.second.dump();
                    ++i;
                }
                oss << '}';
                break;
            }
        }
        return oss.str();
    }

    // 反序列化
    static Json parse(const std::string& str) {
        size_t pos = 0;
        Json result = parseValue(str, pos);
        skipWhitespace(str, pos);
        if (pos != str.size())
            throw std::runtime_error("JSON parse error: trailing characters");
        return result;
    }

private:
    Type type_ = Null;
    bool bool_val_ = false;
    double num_val_ = 0;
    std::string str_val_;
    std::vector<Json> arr_val_;
    std::map<std::string, Json> obj_val_;

    static const Json& nullValue() {
        static Json null_json;
        return null_json;
    }

    static std::string escape(const std::string& s) {
        std::string res;
        res.reserve(s.size());
        for (char c : s) {
            switch (c) {
                case '"':  res += "\\\""; break;
                case '\\': res += "\\\\"; break;
                case '\n': res += "\\n";  break;
                case '\r': res += "\\r";  break;
                case '\t': res += "\\t";  break;
                default:   res += c;
            }
        }
        return res;
    }

    static void skipWhitespace(const std::string& s, size_t& pos) {
        while (pos < s.size() &&
               (s[pos] == ' ' || s[pos] == '\t' || s[pos] == '\n' || s[pos] == '\r'))
            ++pos;
    }

    static Json parseValue(const std::string& s, size_t& pos) {
        skipWhitespace(s, pos);
        if (pos >= s.size()) throw std::runtime_error("unexpected end");
        char c = s[pos];
        switch (c) {
            case '{': return parseObject(s, pos);
            case '[': return parseArray(s, pos);
            case '"': return Json(parseString(s, pos));
            case 't': case 'f': case 'n': return parseKeyword(s, pos);
            default:
                if (c == '-' || (c >= '0' && c <= '9')) return parseNumber(s, pos);
                throw std::runtime_error("unexpected character");
        }
    }

    static Json parseObject(const std::string& s, size_t& pos) {
        Json obj;
        obj.type_ = Object;
        ++pos;
        skipWhitespace(s, pos);
        if (pos < s.size() && s[pos] == '}') { ++pos; return obj; }
        while (pos < s.size()) {
            skipWhitespace(s, pos);
            if (s[pos] != '"') throw std::runtime_error("expected key");
            std::string key = parseString(s, pos);
            skipWhitespace(s, pos);
            if (pos >= s.size() || s[pos] != ':') throw std::runtime_error("expected ':'");
            ++pos;
            obj.obj_val_[key] = parseValue(s, pos);
            skipWhitespace(s, pos);
            if (pos >= s.size()) throw std::runtime_error("unterminated object");
            if (s[pos] == ',') { ++pos; continue; }
            if (s[pos] == '}') { ++pos; break; }
            throw std::runtime_error("expected ',' or '}'");
        }
        return obj;
    }

    static Json parseArray(const std::string& s, size_t& pos) {
        Json arr;
        arr.type_ = Array;
        ++pos;
        skipWhitespace(s, pos);
        if (pos < s.size() && s[pos] == ']') { ++pos; return arr; }
        while (pos < s.size()) {
            arr.arr_val_.push_back(parseValue(s, pos));
            skipWhitespace(s, pos);
            if (pos >= s.size()) throw std::runtime_error("unterminated array");
            if (s[pos] == ',') { ++pos; continue; }
            if (s[pos] == ']') { ++pos; break; }
            throw std::runtime_error("expected ',' or ']'");
        }
        return arr;
    }

    static std::string parseString(const std::string& s, size_t& pos) {
        ++pos;
        std::string out;
        while (pos < s.size()) {
            char c = s[pos];
            if (c == '"') { ++pos; return out; }
            if (c == '\\') {
                ++pos;
                if (pos >= s.size()) throw std::runtime_error("unterminated escape");
                char esc = s[pos++];
                switch (esc) {
                    case '"':  out += '"';  break;
                    case '\\': out += '\\'; break;
                    case '/':  out += '/';  break;
                    case 'n':  out += '\n'; break;
                    case 'r':  out += '\r'; break;
                    case 't':  out += '\t'; break;
                    case 'b':  out += '\b'; break;
                    case 'f':  out += '\f'; break;
                    default: throw std::runtime_error("unknown escape");
                }
            } else {
                out += c;
                ++pos;
            }
        }
        throw std::runtime_error("unterminated string");
    }

    static Json parseNumber(const std::string& s, size_t& pos) {
        const char* start = s.c_str() + pos;
        char* end = nullptr;
        double val = strtod(start, &end);
        if (end == start) throw std::runtime_error("invalid number");
        pos += static_cast<size_t>(end - start);
        return Json(val);
    }

    static Json parseKeyword(const std::string& s, size_t& pos) {
        if (s.compare(pos, 4, "true")  == 0) { pos += 4; return Json(true);  }
        if (s.compare(pos, 5, "false") == 0) { pos += 5; return Json(false); }
        if (s.compare(pos, 4, "null")  == 0) { pos += 4; return Json();      }
        throw std::runtime_error("invalid literal");
    }
};

#endif // JSON_H