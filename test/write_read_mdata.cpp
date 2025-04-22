#include <unistd.h>

#include <iostream>

#include "assert.h"
#include "serializer.hpp"
#include "wait.h"

using namespace RECK;

int main(void) {
    std::string file_path = "/tmp/dump_data_w_r_mdata.reck";

    pid_t pid = fork();
    assert(pid != -1);
    int status;
    if (pid) {
        printf("parent: child pid is %d\n", pid);
        ptracer::allow_pid();
        assert(pid == wait(&status));
        assert(0 == status);

    } else {
        pid_t tracee = getppid();
        printf("child: parent pid is %d\n", tracee);

        int ret = serializer::dump_serialized_file(tracee, file_path);
        if (ret < 0) {
            std::cerr << "Error dumping file " << file_path << std::endl;
            return 1;
        }
        exit(0);
    }

    auto mdatas = serializer::read_serialized_mdata(file_path);
    if (mdatas.size() == 0) {
        std::cerr << "Error reading dump file " << file_path << std::endl;
        return 1;
    }
    for (const auto& mdata : mdatas) {
        std::cout << mdata << std::endl;
    }

    return 0;
}