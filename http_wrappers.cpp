#include "http_wrappers.h"

void http_wrapper::add_part(std::string string) {
    text.append(string);
    update_state();
}

void http_wrapper::update_state() {
    if (state == 0 && text.find("\r\n") != std::string::npos) {
        state = FIRST_LINE;
        parse_first_line();
    }

    if (state == FIRST_LINE && (body_start == 0 || body_start == std::string::npos)) {
        body_start = text.find("\r\n\r\n");
    }

    if (state == FIRST_LINE && body_start != std::string::npos && body_start != 0) {
        state = HEADERS;
        body_start += 4;
        parse_headers();
    }

    if (state >= HEADERS) {
        check_body();
    }
}

void http_wrapper::parse_headers() {
    auto headers_start = std::find_if(text.begin(), text.end(), [](char a) { return a == '\n'; })++;
    auto headers_end = headers_start + 1;

    while (headers_end != text.end() && *headers_end != '\r') {
        auto space = std::find_if(headers_end, text.end(), [](char a) { return a == ':'; });
        auto crlf = std::find_if(space + 1, text.end(), [](char a) { return a == '\r'; });

        headers.insert({{headers_end, space}, {space + 2, crlf}});
        headers_end = crlf + 2;
    };
}

void http_wrapper::append_header(std::string name, std::string value) {
    if (headers.find(name) == headers.end()) {
        headers[name] = value;
    }
}

std::string http_wrapper::get_header(std::string name) const {
    if (headers.find(name) != headers.end()) {
        auto value = headers.at(name);
        return value;
    }

    return "";
}

void http_wrapper::check_body() {
    body = text.substr(body_start);

    if (get_header("Content-Length") != "") {
        if (body.size() == static_cast<size_t>(std::stoi(get_header("Content-Length")))) {
            state = FULL_BODY;
        } else {
            state = PARTIAL_BODY;
        }
    } else if (get_header("Transfer-Encoding") == "chunked") {
        if (std::string(body.end() - 7, body.end()) == "\r\n0\r\n\r\n") {
            state = FULL_BODY;
        } else {
            state = PARTIAL_BODY;
        }
    } else if (body.size() == 0) {
        state = FULL_BODY;
    } else {
        state = BAD;
    }
}

int http_wrapper::get_state() {
    return state;
}

std::string http_wrapper::get_text() const {
    return text;
}

http_request::http_request(std::string text) : http_wrapper(text) {
    update_state();
}

std::string http_request::get_URI()
{
    if (URI.find(host) != -1) {
        URI = URI.substr(URI.find(host) + host.size());
    }
    return URI;
}

std::string http_request::get_host()
{
    if (host == "") {
        host = get_header("Host");
    }
    if (host == "") {
        host = get_header("host");
    }
    if (host == "") {
        throw std::runtime_error("empty host");
    }
    return host;
}

void http_request::parse_first_line()
{
    auto first_space = std::find_if(text.begin(), text.end(), [](char a) { return a == ' '; });
    auto second_space = std::find_if(first_space + 1, text.end(), [](char a) { return a == ' '; });
    auto crlf = std::find_if(second_space + 1, text.end(), [](char a) { return a == '\r'; });

    if (first_space == text.end() || second_space == text.end() || crlf == text.end()) {
        state = BAD;
        return;
    }

    method = {text.begin(), first_space};
    URI = {first_space + 1, second_space};
    http_version = {second_space + 1, crlf};

    if (method != "POST" && method != "GET") {
        state = BAD;
        return;
    }
    if (URI == "") {
        state = BAD;
        return;
    }
    if (http_version != "HTTP/1.1" && http_version != "HTTP/1.0") {
        state = BAD;
        return;
    }
}

std::string http_request::get_request_text() {
    get_host();
    std::string first_line = method + " " + get_URI() + " " + http_version + "\r\n";
    std::string headers;

    for (auto it : this->headers) {
        if (it.first != "Proxy-Connection") {
            headers.append(it.first + ": " + it.second + "\r\n");
        }
    }

    headers += "\r\n";
    return first_line + headers + body;
}

bool http_request::is_validating() const {
    return (get_header("If-Match") != ""
            || get_header("If-Modified-Since") != ""
            || get_header("If-None-Match") != ""
            || get_header("If-Range") != ""
            || get_header("If-Unmodified-Since") != "");
}

http_response::http_response(const http_response &r) : http_wrapper(r.text) {
    code = r.code;
    http_version = r.http_version;
}

http_response::http_response(std::string text) : http_wrapper(text) {
    update_state();
}

std::string http_response::get_code() const {
    return code;
}

void http_response::parse_first_line() {
    auto first_space = std::find_if(text.begin(), text.end(), [](char a) { return a == ' '; });
    auto second_space = std::find_if(first_space + 1, text.end(), [](char a) { return a == ' '; });
    auto crlf = std::find_if(second_space + 1, text.end(), [](char a) { return a == '\r'; });

    if (first_space == text.end() || second_space == text.end() || crlf == text.end()) {
        state = BAD;
        return;
    }

    http_version = {text.begin(), first_space};
    code = {first_space + 1, second_space};

    if (http_version != "HTTP/1.1" && http_version != "HTTP/1.0") {
        state = BAD;
        return;
    }
}

bool http_response::is_cacheable() const {
    return (state == FULL_BODY
            && checkCacheControl()
            && get_header("ETag") != ""
            && get_header("Vary") == ""
            && get_code() == "200");
}


bool http_response::checkCacheControl() const {
    auto target = get_header("Cache-Control");
    return (target == ""
            || (target.find("private") == target.npos
            && target.find("no-cache") == target.npos
            && target.find("no-store") == target.npos));
}




