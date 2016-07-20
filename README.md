# async-http-server
高性能异步发送http请求的服务器程序，使用redis做队列，可接收php/java等其他语言的任务

## 依赖库 (libraries)
使用本人开发的C++ 敏捷开发框架开发 [EasyCpp](https://github.com/onanying/easycpp)

## 安装 (install)

```
# make
```

## 运行 (run)

```
# ./async_http_server default.conf
```

## 配置 (config)

```
# vi default.conf
```

```
{
  "redis_ip" : "127.0.0.1",
  "redis_port" : 6379,
  "redis_passwd" : "123456",

  "redis_queue_key" : "queue_user",
  "push_list" : {
    "A" : "http://9.9.9.9:8888/sync.php",
    "B" : "http://9.9.9.9:8888/sync.php",
    "C" : "http://9.9.9.9:8888/sync.php",
    "D" : "http://9.9.9.9:8888/sync.php",
    "E" : "http://9.9.9.9:8888/sync.php",
    "F" : "http://9.9.9.9:8888/sync.php"
  }
}
```
