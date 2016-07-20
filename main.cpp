#include <iostream>
#include <string>

#include <easycpp/helpers/json.hpp>
#include <easycpp/libraries/exception.hpp>
#include <easycpp/models/redis.hpp>
#include <easycpp/helpers/http.hpp>
#include <easycpp/helpers/log.hpp>

using namespace easycpp;
using namespace std;

/* 配置信息 */
string redis_ip;
int redis_port;
string redis_passwd;
string redis_queue_key;
libraries::JsonObject push_list;

/// 获取配置文件
void init_config(string file)
{
    string conf_str = helpers::file_get_contents(file);
    if(conf_str == ""){
        string msg = "配置文件 " + file + " 读取错误";
        throw libraries::Exception(msg);
    }
    try{
        libraries::JsonObject json_obj;
        helpers::json_init(json_obj, conf_str);
        redis_ip = helpers::json_get_string(json_obj, "redis_ip");
        redis_port = helpers::json_get_int(json_obj, "redis_port");
        redis_passwd = helpers::json_get_string(json_obj, "redis_passwd");
        redis_queue_key = helpers::json_get_string(json_obj, "redis_queue_key");
        helpers::json_get_object(json_obj, push_list, "push_list");
    } catch (exception& ex) {
        string msg = "配置文件 " + file + " 解析错误";
        throw libraries::Exception(msg);
    }
}

/// http发送数据
void send_data()
{
    try {
        models::RedisModel rs(redis_ip, redis_port, redis_passwd);
        // 从redis取出数据
        while (true) {
            string json_str = rs.pullList(redis_queue_key, 3600); // 堵塞1小时
            if(json_str != ""){
                libraries::JsonObject json_obj;
                helpers::json_init(json_obj, json_str);
                string query_str = helpers::http_build_query(json_obj);
                // 循环发送给push_list里面的url
                BOOST_FOREACH(libraries::JsonValue &v, push_list){
                    string key = v.first;
                    string val = helpers::json_get_string(push_list, key);
                    // http发送
                    string reponse_data;
                    int err_code = helpers::http_post(val, query_str, reponse_data, 30);
                    if(err_code != 0){
                        // 发送失败
                        rs.pushList(redis_queue_key, json_str); // 数据重新压入队列
                        // 写入log文件
                        string msg = val + "; " + query_str + "; " + reponse_data + ";";
                        helpers::log_error("http_post", msg);
                    }
                }
            }
        }
    } catch (exception& ex) {
        // 将错误写入log文件
        helpers::log_error("send_data", ex.what());
        // 出错后堵塞一段时间
        sleep(60);
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
        init_config(argc >=2 ? argv[1] : "default.conf");
    } catch (exception& ex) {
        cout << "ERROR: " << ex.what() << endl;
        return 1;
    }

    // 循环发送数据
    while (true) {
        send_data();
    }

    return 0;
}
