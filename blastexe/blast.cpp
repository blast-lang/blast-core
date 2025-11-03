#include <iostream>

#include "CLI11.hpp"
#include "dyload/dyload.hpp"
#include "core/core.hpp"

int main(int argc, char** argv) {
    CLI::App app{"blast"};
    //std::cout << "Hello World" << std::endl;
    blast::load();
}