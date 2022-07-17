#include <getopt.h>

#include <iostream>
#include <memory>
#include <ostream>
#include <utility>

#include "Engine.h"

int main(int argc, char *argv[]){
    std::cout<<"main started, tangaolin."<<std::endl;
    std::shared_ptr<Engine> engine = std::make_shared<Engine>();
    return  0;
}