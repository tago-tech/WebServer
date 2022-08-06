// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <sys/epoll.h>
#include <unistd.h>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include "SocketChannel.h"


class EventsController;

// 定义集中处理状态，当前处理到哪了，一步一步来
enum ProcessState {
  STATE_PARSE_URI = 1,
  STATE_PARSE_HEADERS,
  STATE_RECV_BODY,
  STATE_ANALYSIS,
  STATE_FINISH
};

// uri分析到哪一步了
enum URIState {
  PARSE_URI_AGAIN = 1,
  PARSE_URI_ERROR,
  PARSE_URI_SUCCESS,
};

// header分析到哪一步了
enum HeaderState {
  PARSE_HEADER_SUCCESS = 1,
  PARSE_HEADER_AGAIN,
  PARSE_HEADER_ERROR
};

// 分析http请求到结果，是否分析成功了
enum AnalysisState { ANALYSIS_SUCCESS = 1, ANALYSIS_ERROR };

enum ParseState {
  H_START = 0,
  H_KEY,
  H_COLON,
  H_SPACES_AFTER_COLON,
  H_VALUE,
  H_CR,
  H_LF,
  H_END_CR,
  H_END_LF
};

// 当前tcp链接到状态，已连接，断开中，已断开
enum ConnectionState { H_CONNECTED = 0, H_DISCONNECTING, H_DISCONNECTED };

// http的方法类型
enum HttpMethod { METHOD_POST = 1, METHOD_GET, METHOD_HEAD };

// http协议的版本
enum HttpVersion { HTTP_10 = 1, HTTP_11 };

// MIME,全称为“Multipurpose Internet Mail Extensions”,比较确切的中文名称为“通用互联网邮件扩充”。
class MimeType {
 private:
  static void init();
  static std::unordered_map<std::string, std::string> mime;
  MimeType();
  MimeType(const MimeType &m);

 public:
  static std::string getMime(const std::string &suffix);

 private:
  static pthread_once_t once_control;
};

// 一个HttpRequestContext代表了http请求处理的过程，如何处理，处理到哪一步了
// 一个HttpRequestContext对应于一个socket channel，因为http请求实际是发生在某个socket上的。
// 创建SocketChannel时就把该HttpRequestContext的handleRead、handleWrite方法注册到Socket channel上，这样当Socket channel执行handleEvents处理事件时，就会实际执行HttpRequestContext中所定义的方法。
// 这部分代码比较复杂，你只需要了解过程即可，就说是学长/同学写的，他们负责的这块
// 为什么抽象出来一个HttpRequestContext，是因为http协议是应用层协议，把这一块抽象出来。该项目主要还是为了解决高并发、超多链接的问题，是tcp、传输层的问题，http协议只是一种处理方法，在事件处理器、事件控制器这些玩意搭建起来的模型上，以后可以跑各种各样的应用层协议。
// 所以在socket channel可以设置读写事件的回调方法，将来可以实现， XXX-RequestContext，或者YYY-RequestContext，因为这些都是基于tcp协议的。
class HttpRequestContext : std::enable_shared_from_this<HttpRequestContext>{
 public:
  HttpRequestContext(EventsController* loop, int connfd, std::shared_ptr<SocketChannel> channel);
  ~HttpRequestContext() { close(fd_); }
  void reset();

  std::shared_ptr<SocketChannel> getChannel() { return channel_; }
  EventsController* getEventsController() { return events_controller_; }
  void handleClose(); // 当遇到错误，需要关闭该http链接时，执行这个方法
  void newEvent();

  // 实际会把HttpRequestContext的这三个方法分别赋值给SocketChannel，所以SocketChannel有读写事件发生时，实际执行的是HttpRequestContext的方法
  void handleRead();
  void handleWrite();
  void handleConn();

 private:
  EventsController* events_controller_; //一个HttpRequestContext只能被一个事件控制器操控
  std::shared_ptr<SocketChannel> channel_; 
  int fd_;
  std::string inBuffer_; // http的读缓冲区，因为数据可能一次读不完，因为数据不是一下子全到，都是一到一部分，一到一部分， 正因为如此才需要这个类，来记录上次处理到哪了，下一步该处理啥。
  std::string outBuffer_;
  bool error_;
  ConnectionState connectionState_;

  HttpMethod method_;
  HttpVersion HTTPVersion_;
  std::string fileName_;
  std::string path_;
  int nowReadPos_;
  ProcessState state_;
  ParseState hState_;
  bool keepAlive_;
  std::map<std::string, std::string> headers_;
  void Logit();
  void handleError(int fd, int err_num, std::string short_msg); // 当错误发生时，怎么处理
  URIState parseURI(); //下面这三个方法跟http协议密切相关，可以研读一下。
  HeaderState parseHeaders();
  AnalysisState analysisRequest();
};