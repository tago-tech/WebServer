// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include <getopt.h>

#include <iostream>
#include <memory>

#include "Engine.h"

// 可执行的二进制文件入口，即进程入口
int main(int argc, char *argv[]) {
  std::cout << "main started." << std::endl;
  // 这里主要是初始化一个Engine对象
  // 并用shared_ptr指向它，make_shared<XXX>() 的方式可以避免一次
  // 对象拷贝，具体可从网上学习c++智能指针章节，智能智能包括 weak_ptr,
  // shared_ptr, unqie_ptr三种
  std::shared_ptr<Engine> engine = std::make_shared<Engine>(9002, 2);
  // 启动engine对象，并一直会在Start方法中无限循环，直到异常后退出
  engine->Start();
  return 0;
}
