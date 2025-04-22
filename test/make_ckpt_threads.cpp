#include <unistd.h>

#include <iostream>
#include <thread>

#include "assert.h"
#include "serializer.hpp"
#include "wait.h"

using namespace RECK;

const std::string file_path = "/tmp/dump_data_threads.reck";

int main(void) {
    std::vector<std::thread> v_theads;
    for (size_t i = 0; i < 3; i++) {
        v_theads.emplace_back([id = i]() {
            for (size_t j = 0; j < 5; j++) {
                if (j == 2 && id == 0) {
                    int ret = serializer::make_checkpoint(file_path);
                    if (ret < 0) {
                        std::cerr << "Error make_checkpoint to file " << file_path << std::endl;
                        return 1;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    std::cout << "After make_checkpoint" << std::endl;
                }
                std::cout << "Thread " << std::this_thread::get_id() << " " << j << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            return 0;
        });
    }

    for (auto &t : v_theads) {
        t.join();
    }

    return 0;
}