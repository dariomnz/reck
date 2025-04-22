#include <unistd.h>

#include <iostream>
#include <thread>

#include "assert.h"
#include "serializer.hpp"
#include "wait.h"

using namespace RECK;

int main(void) {
    std::string file_path = "/tmp/dump_data.reck";

    for (size_t i = 0; i < 5; i++) {
        if (i == 2) {
            int ret = serializer::make_checkpoint(file_path);
            if (ret < 0) {
                std::cerr << "Error make_checkpoint to file " << file_path << std::endl;
                return 1;
            }
            std::cout << "After make_checkpoint" << std::endl;
        }
        std::cout << i << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}