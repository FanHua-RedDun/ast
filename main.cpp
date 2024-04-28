#include <charconv>
#include <iostream>
#include <nlohmann/json.hpp>
#include <cstdio>
#include <random>
#include <curl/curl.h>
#include <chrono>
#include <thread>
#include <unicode/ucnv.h>
#include <locale>
#include <codecvt>
#include <fstream>
using namespace std;

// libcurl回调处理
size_t libcurl_request_data_return(void *contents, const size_t size, const size_t nmemb, std::stringstream *userp) {
    const size_t realsize = size * nmemb;
    userp->write(static_cast<char *>(contents), realsize);
    return realsize;
}

// 定义方法
class Request {
public:
    // 定义动态方法
    // == 请求头 》 赋值请求头
    void header(const nlohmann::json &headers) { this->header_json = headers; }
    // == GEI请求 》 记录请求编号
    void get(const string &url) {
        this->request_type = 0;
        this->request_url = url;
    }

    // == POST请求 》 传递请求数据 》 记录请求编号
    void post(const string &url, const nlohmann::json &data) {
        this->request_type = 1;
        this->request_url = url;
        this->post_data = data;
    }

    // == 请求停顿 》 赋值停顿时间
    void seelp(const int time = 0) { this->run_seelp = time; }

    // == 运行请求 》 逻辑处理区
    [[nodiscard]] auto run() const {
        // 设置暂停器
        using namespace std::chrono;
        // 定义变量 》 返回数据 - 废弃
        stringstream request_data;
        // 初始化 CURL 句柄
        CURL *_curl = curl_easy_init();
        // 设置请求地址
        curl_easy_setopt(_curl, CURLOPT_URL, request_url.c_str());
        // 设置回调函数 - 废弃方案
        curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, &libcurl_request_data_return);
        // 传递回调 - 废弃方案
        curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &request_data);
        // 创建header链表
        struct curl_slist *headers = nullptr;
        // 定义字符串数组变量 》 空间（请求头json大小） 》 请求头字段
        string headers_key[header_json.size()];
        // 定义整型变量 》 请求头字段下标 》 初始化
        int headers_index = 0;
        // 循环 》 遍历json请求头
        for (auto &item: header_json.items()) {
            // 插入数组 》 请求头下标累加 》 json请求头Key字段值
            headers_key[headers_index++].append(item.key());
            // 修改数据 》 json请求头大小 等于 请求头字段下标长度 初始化请求头字段下标 否则 不执行任何操作
            headers_index == header_json.size() ? headers_index = 0 : false;
        }
        // 循环 》 遍历请求头字段
        for (auto &key: headers_key) {
            // 创建ostringstream流对象 》 请求头字符串拼接
            std::ostringstream headers_data;
            // 拼接字符串 》 请求头数据
            headers_data << "\"" << key << ":" << to_string(header_json[key]).substr(
                1, to_string(header_json[key]).length());
            // 添加header链表数据
            headers = curl_slist_append(headers, headers_data.str().data());
        }
        // 使用headers链表
        curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, headers);
        // if 判断 请求类型
        // ==（0:GET,1:POST）
        if (request_type == 0) {
            // 设置请求方法
            curl_easy_setopt(_curl, CURLOPT_CUSTOMREQUEST, "GET");
        } else if (request_type == 1) {
            // 设置请求方法
            curl_easy_setopt(_curl, CURLOPT_CUSTOMREQUEST, "POST");
            // 定义字符串变量 》 POST数据
            string post_datas;
            // 定义字符串数组变量 》 空间（POST数据大小） 》 请求头字段
            string post_key[post_data.size()];
            // 定义整型变量 》 POST数据字段下标 》 初始化
            int post_index = 0;
            // 循环 》 遍历POST数据
            for (auto &item: post_data.items()) {
                // 插入数组 》 POST数据下标累加 》 POST数据JSON的Key字段值
                post_key[post_index++].append(item.key());
                // 修改数据 》 POST数据大小 等于 POST数据字段下标长度 初始化请POST数据字段下标 否则 不执行任何操作
                post_index == post_data.size() ? post_index = 0 : false;
            }
            // 循环 》 遍历POST字段
            for (auto &key: post_key) {
                // 创建ostringstream流对象 》 POST数据字符串拼接
                std::ostringstream post_datass;
                // 拼接字符串 》 POST数据
                post_datass << key << "=" << to_string(post_data[key]).substr(1, to_string(post_data[key]).length() - 2)
                        << "&";
                // 赋值 》 累加 》 POST数据
                post_datas += post_datass.str();
                // POST数据字段 》 累加 》 自增
                post_index++;
                // 修改数据 》 POST数据大小 等于 POST数据字符截取修改（末尾&号去除，首端添加“） 否则 不执行任何操作
                post_index == post_data.size()
                    ? post_datas.replace(post_datas.length() - 1, post_datas.length(), "\"").insert(0, "\"")
                    : "";
                // 修改数据 》 POST数据大小 等于 POST数据字段下标长度 初始化请POST数据字段下标 否则 不执行任何操作
                post_index == post_data.size() ? post_index = 0 : false;
            }
            // 使用POST数据
            curl_easy_setopt(_curl, CURLOPT_POSTFIELDS, post_datas);
            // curl_slist *plist = curl_slist_append(NULL,"Content-Type:application/json;charset=UTF-8");
            // curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, plist);
        }

        // 执行请求
        curl_easy_perform(_curl);
        // 设置暂停时间
        std::this_thread::sleep_for(std::chrono::seconds(run_seelp));
        // 返回数据
        return request_data.str();
    }

    Request() {
        // 定义字符串数组 》 User-Agent标识列表
        const string user_agent_list[] = {
            // Opera
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/123.0.0.0 Safari/537.36 Edg/123.0.0.0",
            "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/39.0.2171.95 Safari/537.36 OPR/26.0.1656.60",
            "Opera/8.0 (Windows NT 5.1; U; en)",
            "Mozilla/5.0 (Windows NT 5.1; U; en; rv:1.8.1) Gecko/20061208 Firefox/2.0.0 Opera 9.50",
            "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; en) Opera 9.50",
            "Opera/9.80 (Macintosh; Intel Mac OS X 10.6.8; U; en) Presto/2.8.131 Version/11.11",
            "Opera/9.80 (Windows NT 6.1; U; en) Presto/2.8.131 Version/11.11",
            "Opera/9.80 (Android 2.3.4; Linux; Opera Mobi/build-1107180945; U; en-GB) Presto/2.8.149 Version/11.10",
            // Firefox
            "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:34.0) Gecko/20100101 Firefox/34.0",
            "Mozilla/5.0 (X11; U; Linux x86_64; zh-CN; rv:1.9.2.10) Gecko/20100922 Ubuntu/10.10 (maverick) Firefox/3.6.10",
            "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.6; rv,2.0.1) Gecko/20100101 Firefox/4.0.1",
            "Mozilla/5.0 (Windows NT 6.1; rv,2.0.1) Gecko/20100101 Firefox/4.0.1",
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:125.0) Gecko/20100101 Firefox/125.0",
            // Safari
            "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/534.57.2 (KHTML, like Gecko) Version/5.1.7 Safari/534.57.2",
            "Mozilla/5.0 (iPhone; U; CPU iPhone OS 4_3_3 like Mac OS X; en-us) AppleWebKit/533.17.9 (KHTML, like Gecko) Version/5.0.2 Mobile/8J2 Safari/6533.18.5",
            "Mozilla/5.0 (iPhone; U; CPU iPhone OS 4_3_3 like Mac OS X; en-us) AppleWebKit/533.17.9 (KHTML, like Gecko) Version/5.0.2 Mobile/8J2 Safari/6533.18.5",
            "Mozilla/5.0 (iPad; U; CPU OS 4_3_3 like Mac OS X; en-us) AppleWebKit/533.17.9 (KHTML, like Gecko) Version/5.0.2 Mobile/8J2 Safari/6533.18.5",
            // Chrome
            "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/39.0.2171.71 Safari/537.36",
            "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.11 (KHTML, like Gecko) Chrome/23.0.1271.64 Safari/537.11",
            "Mozilla/5.0 (Windows; U; Windows NT 6.1; en-US) AppleWebKit/534.16 (KHTML, like Gecko) Chrome/10.0.648.133 Safari/534.16",
            "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_7_0) AppleWebKit/535.11 (KHTML, like Gecko) Chrome/17.0.963.56 Safari/535.11",
            // 360浏览器
            "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36",
            "Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; rv:11.0) like Gecko",
            "Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1; 360SE)",
            // 淘宝浏览器
            "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/536.11 (KHTML, like Gecko) Chrome/20.0.1132.11 TaoBrowser/2.0 Safari/536.11",
            // 猎豹浏览器
            "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.1 (KHTML, like Gecko) Chrome/21.0.1180.71 Safari/537.1 LBBROWSER",
            "Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; WOW64; Trident/5.0; SLCC2; .NET CLR 2.0.50727; .NET CLR 3.5.30729; .NET CLR 3.0.30729; Media Center PC 6.0; .NET4.0C; .NET4.0E; LBBROWSER)",
            "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; QQDownload 732; .NET4.0C; .NET4.0E; LBBROWSER)",
            // QQ浏览器
            "Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; WOW64; Trident/5.0; SLCC2; .NET CLR 2.0.50727; .NET CLR 3.5.30729; .NET CLR 3.0.30729; Media Center PC 6.0; .NET4.0C; .NET4.0E; QQBrowser/7.0.3698.400)",
            "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; QQDownload 732; .NET4.0C; .NET4.0E)",
            // Sogou浏览器
            "Mozilla/5.0 (Windows NT 5.1) AppleWebKit/535.11 (KHTML, like Gecko) Chrome/17.0.963.84 Safari/535.11 SE 2.X MetaSr 1.0",
            "Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1; Trident/4.0; SV1; QQDownload 732; .NET4.0C; .NET4.0E; SE 2.X MetaSr 1.0)",
            "Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1; Trident/4.0; SE 2.X MetaSr 1.0; SE 2.X MetaSr 1.0; .NET CLR 2.0.50727; SE 2.X MetaSr 1.0)",
            // 傲游（Maxthon）浏览器
            "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Maxthon/4.4.3.4000 Chrome/30.0.1599.101 Safari/537.36",
            "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_7_0) AppleWebKit/535.11 (KHTML, like Gecko) Chrome/17.0.963.56 Safari/535.11",
            // UC浏览器
            "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/38.0.2125.122 UBrowser/4.0.3214.0 Safari/537.36",
            "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/55.0.2883.87 UBrowser/6.2.4094.1 Safari/537.36",
            // iPhone
            "Mozilla/5.0 (iPhone; U; CPU iPhone OS 4_3_3 like Mac OS X; en-us) AppleWebKit/533.17.9 (KHTML, like Gecko) Version/5.0.2 Mobile/8J2 Safari/6533.18.5",
            // IPod
            "Mozilla/5.0 (iPod; U; CPU iPhone OS 4_3_3 like Mac OS X; en-us) AppleWebKit/533.17.9 (KHTML, like Gecko) Version/5.0.2 Mobile/8J2 Safari/6533.18.5",
            // IPAD
            "Mozilla/5.0 (iPad; U; CPU OS 4_2_1 like Mac OS X; zh-cn) AppleWebKit/533.17.9 (KHTML, like Gecko) Version/5.0.2 Mobile/8C148 Safari/6533.18.5",
            "Mozilla/5.0 (iPad; U; CPU OS 4_3_3 like Mac OS X; en-us) AppleWebKit/533.17.9 (KHTML, like Gecko) Version/5.0.2 Mobile/8J2 Safari/6533.18.5",
            // BlackBerry
            "Mozilla/5.0 (BlackBerry; U; BlackBerry 9800; en) AppleWebKit/534.1+ (KHTML, like Gecko) Version/6.0.0.337 Mobile Safari/534.1+",
            // WebOS HP Touchpad
            "Mozilla/5.0 (hp-tablet; Linux; hpwOS/3.0.0; U; en-US) AppleWebKit/534.6 (KHTML, like Gecko) wOSBrowser/233.70 Safari/534.6 TouchPad/1.0",
            // IE
            "Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; Trident/5.0;",
            "Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 6.0)",
            "Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.0; Trident/4.0)",
            // 世界之窗（The World）
            "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)",
            "Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1)",
            "Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1; The World)",
        };
        // 初始化均匀随机分配
        uniform_int_distribution<unsigned> Random_number_range(0, user_agent_list->length() - 1);
        // 变量赋值 》 随机数机制
        this->Random_number_range = Random_number_range;
        // 变量赋值 》 写入数据 》 请求头 》 随机浏览器标识
        this->header_json["Sec-Ch-Ua"] = R"("Microsoft Edge";v="123","Not:A-Brand";v="8","Chromium";v="123")";
        this->header_json["Sec-Ch-Ua-Mobile"] = "?0";
        this->header_json["Sec-Ch-Ua-Platform"] = "Windows";
        this->header_json["Referer"] = this->request_url;
        // this->header_json["Accept-Encoding"] = "gzip, deflate, br";
        this->header_json["User-Agent"] = this->Random_number_range(this->default_random);
    }

    // 定义动态变量
private:
    // 定义整形变量 》 请求地址
    string request_url;
    // 定义整形变量 》 请求类型
    int request_type;
    // 定义json变量 》 POST数据
    nlohmann::json post_data = NULL;
    // 定义整形变量 》 停顿时间
    int run_seelp = 0;
    // 初始化均匀随机分配
    uniform_int_distribution<unsigned> Random_number_range;
    // 初始化随机数
    default_random_engine default_random;
    // 定义json变量 》 请求头
    nlohmann::json header_json;
};

// 
int main() {
    system("chcp 65001 > null");
    // 实例化 Request类
    Request request;
    // == 调用get方法（地址）
    request.get("http://hn216.api.yesapi.cn/");
    // nlohmann::json data;
    // data["s"] = "App.Table.Create";
    // data["return_data"] = "0";
    // request.post("/",data);
    // == 调用运行方法
    auto as = request.run();
    cout << as;
    cout << "你好,woshi那是我都很舍得花钱和我赌气文化的睡会啊" << endl;
    // return system("pause");
    return 0;
}
