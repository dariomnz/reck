#include "maps_parser.hpp"
#include <iostream>
#include <unistd.h>

using namespace RECK;

int main(void) {

    pid_t pid = getpid();

    std::vector<memory_map> maps = maps_parser::get_maps(pid);

    for (const auto& map : maps) {
        std::cout << map << std::endl;
    }

    return 0;
}