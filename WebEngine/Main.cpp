// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include <getopt.h>

#include <iostream>
#include <memory>

#include "Engine.h"


int main(int argc, char *argv[]) {
  std::cout << "main started." << std::endl;
  std::shared_ptr<Engine> engine = std::make_shared<Engine>(9002, 2);
  engine->Start();
  return 0;
}
