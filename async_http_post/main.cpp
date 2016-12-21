#include <iostream>
#include <string>

#include <boost/foreach.hpp>
#include <boost/thread/thread.hpp>
#include <boost/filesystem.hpp>

#include <easycpp/helpers/json.h>
#include <easycpp/libraries/exception.h>
#include <easycpp/models/redis.h>
#include <easycpp/helpers/http.h>
#include <easycpp/helpers/log.h>
#include <easycpp/helpers/string.h>
#include <easycpp/helpers/file.h>

using namespace std;
using namespace easycpp;

/// 变量定义
string redisHost;
int redisPort;
string redisAuth;
string redisListKey;
vector<libraries::JsonObject> postUrls;
int httpTimeout;
string nowPid;
string cacheDirPath;
string logDirPath;

/// 获取配置文件
void init_config(string file)
{
    string confStr = helpers::file_get_contents(file);
    if(confStr == ""){
        string msg = "配置文件 " + file + " 读取错误";
        throw libraries::Exception(msg);
    }
    try{
        libraries::JsonObject jsonObj = helpers::json_init(confStr);
        redisHost = helpers::json_get_string(jsonObj, "redis_host");
        redisPort = helpers::json_get_int(jsonObj, "redis_port");
        redisAuth = helpers::json_get_string(jsonObj, "redis_auth");
        redisListKey = helpers::json_get_string(jsonObj, "redis_list_key");
        postUrls = helpers::json_get_array(jsonObj, "post_urls");
        httpTimeout = helpers::json_get_int(jsonObj, "http_timeout");
    } catch (exception& /* ex */) {
        string msg = "配置文件 " + file + " 参数错误";
        throw libraries::Exception(msg);
    }
}

/// post数据
void post(string url, libraries::JsonObject params, string paramsStr, int * postErrCode)
{
    string response;
    int errCode = helpers::http_post(url, params, response, httpTimeout);

    // 写入log文件
    string msg = url + " | " + paramsStr + " | " + response;
    helpers::log_info("http_post", msg, logDirPath);

    if (errCode == 200) {
        // 成功
        *postErrCode = 200;
    } else {
        // 失败
        // 写入log文件
        string msg = url + " | " + paramsStr + " | " + response;
        helpers::log_error("post", msg, logDirPath);
    }
}

/// 启动
void start()
{
    vector<boost::thread*> postThread;
    vector<int*> postErrCode;

    try {
        models::RedisModel rs(redisHost, redisPort, redisAuth);
        // 将当前进程的缓存重新压入队列
        string cacheStr = helpers::file_get_contents(cacheDirPath + "/" + nowPid);
        if(!cacheStr.empty()){
            rs.pushList(redisListKey, cacheStr); // 数据重新压入队列
        }
        // 从redis取出数据
        while (true) {
            string paramsStr = rs.pullList(redisListKey, 3600); // 堵塞1小时
            if(!paramsStr.empty()){
                // 存入缓存
                helpers::file_put_contents(cacheDirPath + "/" + nowPid, paramsStr, FILE_REPLACE);
                // 转换为json
                libraries::JsonObject params = helpers::json_init(paramsStr);
                // 循环发送给push_list里面的url
                BOOST_FOREACH (libraries::JsonObject &item, postUrls) {
                    string url = helpers::json_get_string(item, "url");                    
                    // 启动多线程
                    postErrCode.push_back(new int(-3));
                    postThread.push_back(new boost::thread(boost::bind(&post, url, params, paramsStr, postErrCode.back())));
                }
                // 等待全部线程完成
                BOOST_FOREACH (boost::thread *item, postThread) {
                    item->join();
                }
                // 检查执行结果
                bool allPostState = true;
                BOOST_FOREACH (int *value, postErrCode) {
                    if(*value != 200){
                        allPostState = false;
                    }
                }
                // 释放内存
                BOOST_FOREACH (boost::thread *item, postThread) {
                    delete item;
                }
                BOOST_FOREACH (int *value, postErrCode) {
                    delete value;
                }
                // 清空容器
                postThread.clear();
                postErrCode.clear();
                // post失败
                if(!allPostState){
                    rs.pushList(redisListKey, paramsStr); // 数据重新压入队列
                }
            }
        }
    } catch (exception& ex) {
        // 将错误写入log文件
        helpers::log_error("start", ex.what(), logDirPath);
        // 出错后堵塞一段时间
        sleep(60);
    }
}

/// 读取所有缓存
void read_all_cache()
{
    // 将所有缓存重新压入队列
    models::RedisModel rs(redisHost, redisPort, redisAuth);
    vector<string> filenames = helpers::readdir(cacheDirPath);
    BOOST_FOREACH (string item, filenames) {
        // 读取缓存
        string filename = string(item.c_str());
        string cacheStr = helpers::file_get_contents(filename);
        if(!cacheStr.empty()){
            rs.pushList(redisListKey, cacheStr); // 数据重新压入队列
        }
        // 删除缓存
        boost::filesystem::remove(filename);
    }
}

int main(int argc, char *argv[])
{
    // fork新进程， 使终端可关闭
    int pid = fork();
    if(pid == -1){
        // fork出错
        cout << "程序fork进程出错" << endl;
        return 1;
    }
    if(pid > 0){
        // 结束父进程
        cout << "程序在进程 " << pid << " 启动成功" << endl;
        return 0;
    }

    // 初始化参数
    try {
        init_config(argc >= 2 ? argv[1] : "default.conf");
    } catch (exception& ex) {
        cout << "INIT CONFIG ERROR: " << ex.what() << endl;
        return 1;
    }

    // 获取当前进程id
    nowPid = helpers::strval(getpid());

    // 配置目录
    string configFileName = argc >= 2 ? argv[1] : "default.conf";
    configFileName += ".DATA";
    cacheDirPath = configFileName + "/cache";
    logDirPath = configFileName + "/log";

    // 读取所有缓存
    try {
        read_all_cache();
    } catch (exception& ex) {
        cout << "READ ALL CACHE ERROR: " << ex.what() << endl;
        return 2;
    }

    // 循环启动
    while (true) {
        start();
    }

    return 0;
}
