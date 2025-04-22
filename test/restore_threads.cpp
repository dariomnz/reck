#include <unistd.h>

#include <iostream>

#include "assert.h"
#include "serializer.hpp"
#include "wait.h"

using namespace RECK;

int main(void) {
    std::string file_path = "/tmp/dump_data_threads.reck";

    auto ret = serializer::restore_serialized_file(file_path);
    if (ret < 0) {
        std::cerr << "Error restoring dump file " << file_path << std::endl;
        return 1;
    }

    return 0;
}