#pragma once
#include <string>
#include <map>

class HttpClient {
public:
    static HttpClient& instance();

    bool init();
    void cleanup();

    // 发送POST请求，返回响应body
    std::string post(const std::string& url,
                     const std::string& body,
                     const std::string& contentType = "application/json",
                     const std::map<std::string, std::string>& headers = {});

private:
    HttpClient() = default;
    ~HttpClient();
    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;

    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp);

    bool initialized_ = false;
};
