#include "HttpClient.h"
#include <curl/curl.h>
#include <iostream>

HttpClient& HttpClient::instance() {
    static HttpClient client;
    return client;
}

HttpClient::~HttpClient() {
    cleanup();
}

bool HttpClient::init() {
    if (initialized_) return true;

    CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
    if (res != CURLE_OK) {
        std::cerr << "[HttpClient] curl_global_init失败: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    initialized_ = true;
    std::cout << "[HttpClient] 初始化成功" << std::endl;
    return true;
}

void HttpClient::cleanup() {
    if (initialized_) {
        curl_global_cleanup();
        initialized_ = false;
    }
}

size_t HttpClient::writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

std::string HttpClient::post(const std::string& url,
                              const std::string& body,
                              const std::string& contentType,
                              const std::map<std::string, std::string>& headers) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return "{\"error\":\"无法创建CURL句柄\"}";
    }

    std::string response;

    // 设置URL
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    // 设置POST数据
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));

    // 响应回调
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    // 超时设置
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

    // 构建headers
    struct curl_slist* headerList = nullptr;
    std::string ctHeader = "Content-Type: " + contentType;
    headerList = curl_slist_append(headerList, ctHeader.c_str());

    for (const auto& h : headers) {
        std::string headerStr = h.first + ": " + h.second;
        headerList = curl_slist_append(headerList, headerStr.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);

    // 执行请求
    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headerList);

    if (res != CURLE_OK) {
        std::cerr << "[HttpClient] 请求失败: " << curl_easy_strerror(res) << " URL: " << url << std::endl;
        curl_easy_cleanup(curl);
        return "{\"error\":\"HTTP请求失败: " + std::string(curl_easy_strerror(res)) + "\"}";
    }

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);

    if (httpCode != 200) {
        std::cerr << "[HttpClient] HTTP状态码: " << httpCode << std::endl;
    }

    return response;
}
