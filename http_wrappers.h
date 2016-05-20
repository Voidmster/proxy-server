#ifndef PROXY_HTTP_WRAPPERS_H
#define PROXY_HTTP_WRAPPERS_H


#include <unordered_map>
#include <string>
#include <sstream>
#include <regex>
#include <iostream>

class http_wrapper {
public:
    enum state_t {
        BAD = -1, BEFORE = 0, FIRST_LINE = 1, HEADERS = 2, PARTIAL_BODY = 3, FULL_BODY = 4
    };

    static std::string BAD_REQUEST() {
        std::string request = std::string("HTTP/1.1 400 Bad Request\r\n");
        request.append("Server: lol\r\n");
        request.append("Content-Type: text/html; charset=utf-8\r\n");
        request.append("Content-Length: 163\r\n");
        request.append("Connection: close\r\n\r\n");
        request.append("<html>\r\n");
        request.append("<head><title>400 Bad Request</title></head>\r\n");
        request.append("<body bgcolor=\"white\">\r\n");
        request.append("<center><h1>400 Bad Request</h1></center>\r\n");
        request.append("<hr><center>proxy</center>\r\n");
        request.append("</body>\r\n");
        request.append("</html>");

        return request;
    }

    static std::string NOT_FOUND() {
        std::string request = std::string("HTTP/1.1 404 Not found\r\n");
        request.append("Server: lol\r\n");
        request.append("Content-Type: text/html; charset=utf-8\r\n");
        request.append("Content-Length: 159\r\n");
        request.append("Connection: close\r\n\r\n");
        request.append("<html>\r\n");
        request.append("<head><title>404 Not found</title></head>\r\n");
        request.append("<body bgcolor=\"white\">\r\n");
        request.append("<center><h1>404 Not found</h1></center>\r\n");
        request.append("<hr><center>proxy</center>\r\n");
        request.append("</body>\r\n");
        request.append("</html>");

        return request;
    }

    http_wrapper(std::string input) : text(input) { };
    virtual ~http_wrapper() { };

    int get_state();
    void add_part(std::string);
    void append_header(std::string name, std::string value);
    std::string get_header(std::string) const;
    std::string get_text() const;
    state_t state = BEFORE;
protected:
    void update_state();
    void check_body();
    void parse_headers();
    virtual void parse_first_line() = 0;

    size_t body_start = 0;
    std::string text;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
};

struct http_request: public http_wrapper {
    http_request(std::string text);

    std::string get_URI();
    std::string get_host();
    std::string get_request_text();

    bool is_validating() const;
private:
    void parse_first_line() override;

    std::string method;
    std::string URI;
    std::string http_version;
    std::string host = "";
};

struct http_response: public http_wrapper {
    http_response(std::string text);
    http_response(const http_response&);
    bool is_cacheable() const;
    std::string get_code() const;
    bool checkCacheControl() const;
private:
    void parse_first_line() override;

    std::string code;
    std::string http_version;
};

#endif //PROXY_HTTP_WRAPPERS_H
