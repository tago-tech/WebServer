#include <getopt.h>

#include <iostream>
#include <memory>
#include <ostream>
#include <utility>

#include "Engine.h"
#include "SocketChannel.h"

int main(int argc, char *argv[]){
    std::cout<<"main started, tangsz."<<std::endl;
    std::shared_ptr<Engine> engine = std::make_shared<Engine>(9999,10);
    engine->Start();
    return  0;
}