# async-http-post
多线程异步http post数据发送程序，使用redis做队列，可接收php/java等其他语言的异步任务。

## 使用场景

例如：用户信息的修改后需将修改后的用户信息同步到N个不同机房的服务器  
异步模式：用户修改信息接口将要发送的数据压入redis队列，async-http-post在后台监听redis队列，当有消息时，按队列顺序多线程发送数据至N个不同机房的服务器。

## 特点

- 消息必达：当接收的服务器中有任何一个未返回http状态码200时，消息重新压入队列，等待下一次重试。
- 消息不丢失：没一次待发送的数据都有缓存到本地，即便进程被kill，下次启动程序数据将从缓存重新压入队列。
- 多线程：每一个url将启动一个单独的线程来处理，性能更高。
- 多进程：支持多个进程同时运行，当数据量太大时，只需多运行几次，产生多个进程即可。

## 依赖库 (libraries)
使用本人开发的C++ 敏捷开发框架开发 [EasyCpp](https://github.com/onanying/easycpp)

## 安装 (install)

```
# make
```

## 运行 (run)

```
# ./async_http_post default.conf
```

## 配置 (config)

redis_list_key: redis内list类型的key (“queue_user_token” 实际存储key为 “list:queue_user_token”)  
http_timeout: http请求的超时时间，单位:秒  
post_urls: 接收数据的url清单，POST请求  

```
{

  "redis_host" : "127.0.0.1",

  "redis_port" : 6379,

  "redis_auth" : "123456",

  "redis_list_key" : "queue_user_token",

  "http_timeout" : 30,

  "post_urls" : [
    {"url" : "http://114.119.4.115:8888/test.php"},
    {"url" : "http://114.119.4.115:8888/test.php"}
  ]

}
```

## 日志

日志在log目录里，每一次http请求与请求的结果都有一行记录，error.log单独记录了错误日志，info.log记录了所有日志，日志文件没有做循环减小的机制，请定期手动清理日志。

## 缓存

缓存在cache目录里，程序会自动管理，请勿删除，否则会丢失数据。




