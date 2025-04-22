#define DEBUG

#include "serializer.hpp"

#include <fcntl.h>
#include <sys/wait.h>

#include <algorithm>
#include <charconv>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "debug.hpp"
#include "defer.hpp"
#include "filesystem.hpp"

namespace RECK {

std::ostream& operator<<(std::ostream& os, const serializer::mdata& md) {
#define CASE_TYPE(name)                \
    case serializer::mdata_type::name: \
        os << #name;                   \
        break
    switch (md.type) {
        CASE_TYPE(REGS);
        CASE_TYPE(FPREGS);
        CASE_TYPE(MEMORY_MAP);
        default:
            os << "Unknown type (" << static_cast<int>(md.type) << ")";
            break;
    }
    os << " offset: " << md.offset;
    os << " size: " << md.size;
    return os;
}

ssize_t serializer::restore_serialized_file(const std::string_view& file_path) {
    ssize_t ret = 0;
    debug_msg("Begin");

    auto v_mdata = read_serialized_mdata(file_path);
    if (v_mdata.size() == 0) {
        std::cerr << "Error opening file " << file_path << strerror(errno) << std::endl;
        return -1;
    }

    std::string file_path_str{file_path};

    int fd = ::open(file_path_str.c_str(), O_RDONLY);
    defer({
        if (fd >= 0) ::close(fd);
    });

    if (fd < 0) {
        std::cerr << "Error opening file " << file_path << strerror(errno) << std::endl;
        return -1;
    }

    std::vector<user_regs_struct> v_regs;
    std::vector<user_fpregs_struct> v_fpregs;

    for (auto& md : v_mdata) {
        debug_msg(md);
        auto ret = ::lseek(fd, md.offset, SEEK_SET);
        if (ret < 0) {
            std::cerr << "Error lseek to data of file " << file_path << " " << strerror(errno) << std::endl;
            return -1;
        }
        if (md.type == mdata_type::REGS) {
            auto& regs = v_regs.emplace_back();
            ret = filesystem::read(fd, &regs, sizeof(regs));
            if (ret != sizeof(regs)) {
                std::cerr << "Error reading memory data of file " << file_path << " " << strerror(errno) << std::endl;
                return -1;
            }
        } else if (md.type == mdata_type::FPREGS) {
            auto& fpregs = v_fpregs.emplace_back();
            ret = filesystem::read(fd, &fpregs, sizeof(fpregs));
            if (ret != sizeof(fpregs)) {
                std::cerr << "Error reading memory data of file " << file_path << " " << strerror(errno) << std::endl;
                return -1;
            }
        } else if (md.type == mdata_type::MEMORY_MAP) {
            memory_map map;
            ret = filesystem::read(fd, &map, sizeof(map));
            if (ret != sizeof(map)) {
                std::cerr << "Error reading memory_map data of file " << file_path << " " << strerror(errno)
                          << std::endl;
                return -1;
            }
            debug_msg(map);

            void* addr = MAP_FAILED;
            if (std::strstr(map.pathname, "[stack]")) {
                addr = mmap(reinterpret_cast<void*>(map.start_address), map.size(), PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS | MAP_GROWSDOWN | MAP_STACK, -1, 0);
            } else {
                addr = mmap(reinterpret_cast<void*>(map.start_address), map.size(), PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
            }
            if (addr == MAP_FAILED) {
                std::cerr << "Error mapping memory_map " << map << " " << strerror(errno) << std::endl;
                return -1;
            }

            ret = filesystem::read(fd, reinterpret_cast<void*>(map.start_address), map.size());
            if (ret != static_cast<ssize_t>(map.size())) {
                std::cerr << "Error reading data to memory " << map << " " << strerror(errno) << std::endl;
                return -1;
            }

            ret = mprotect(reinterpret_cast<void*>(map.start_address), map.size(), map.prot);
            if (ret < 0) {
                std::cerr << "Error mprotect data " << map << " " << strerror(errno) << std::endl;
                return -1;
            }
        } else {
            std::cerr << "Error unknown type of mdata in file " << file_path << std::endl;
            return -1;
        }
    }

    // TODO: restore threads

    // Now change the registers with a child
    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "Error fork " << strerror(errno) << std::endl;
        return -1;
    }
    int status;
    if (pid) {
        ptracer::allow_pid();
        if (pid != wait(&status)) {
            std::cerr << "Error wait " << strerror(errno) << std::endl;
            return -1;
        }
        // This is unrechable because the child will change the registers
        return -1;
    } else {
        pid_t ppid = getppid();
        ptracer p{ppid};
        p.init();
        p.set_fpregs(v_fpregs);
        p.set_regs(v_regs);
        // p.detach();
        exit(0);
    }

    debug_msg("End");
    return ret;
}

std::vector<serializer::mdata> serializer::read_serialized_mdata(const std::string_view& file_path) {
    ssize_t ret = 0;
    std::vector<mdata> v_md;
    debug_msg("Begin");

    std::string file_path_str{file_path};

    int fd = ::open(file_path_str.c_str(), O_RDONLY);
    defer({
        if (fd >= 0) ::close(fd);
    });

    if (fd < 0) {
        std::cerr << "Error opening file " << file_path << " " << strerror(errno) << std::endl;
        return v_md;
    }

    header h;
    ret = filesystem::read(fd, &h, sizeof(h));
    if (ret != sizeof(h)) {
        std::cerr << "Error reading header of file " << file_path << " " << strerror(errno) << std::endl;
        return v_md;
    }
    if (h.m_num != header::get_default_magic_num()) {
        std::cerr << "Error magic number difers in header of file " << file_path << std::endl;
        return v_md;
    }

    do {
        mdata md;
        ret = filesystem::read(fd, &md, sizeof(md));
        if (ret != sizeof(md)) {
            break;
        }
        ::lseek(fd, md.offset + md.size, SEEK_SET);
        v_md.push_back(md);
    } while (ret == sizeof(mdata));

    debug_msg("End");
    return v_md;
}

ssize_t serializer::make_checkpoint(const std::string_view& file_path) {
    debug_msg("Begin");
    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "Error fork " << strerror(errno) << std::endl;
        return -1;
    }
    if (pid) {
        ptracer::allow_pid();
    } else {
        pid_t tracee = getppid();
        int ret = serializer::dump_serialized_file(tracee, file_path);
        if (ret < 0) {
            std::cerr << "Error dumping file " << file_path << std::endl;
        }
        exit(0);
    }
    debug_msg("End");
    return 0;
}

ssize_t serializer::dump_serialized_file(pid_t pid, const std::string_view& file_path) {
    ssize_t ret = 0;
    debug_msg("Begin");

    std::string file_path_str{file_path};

    int fd = ::open(file_path_str.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    defer({
        if (fd >= 0) ::close(fd);
    });

    if (fd < 0) {
        std::cerr << "Error opening file " << file_path << " " << strerror(errno) << std::endl;
        return fd;
    }

    header h;
    ret = filesystem::write(fd, &h, sizeof(h));
    if (ret != sizeof(h)) {
        std::cerr << "Error writing header to file " << file_path << " " << strerror(errno) << std::endl;
        return ret;
    }

    ptracer p{pid};
    ret = p.init();
    if (ret < 0) {
        std::cerr << "Error initiating ptracer for pid " << pid << std::endl;
        return ret;
    }

    auto v_regs = p.get_regs();
    if (v_regs.size() == 0) {
        std::cerr << "Error getting regs for pid " << pid << std::endl;
        return ret;
    }

    for (auto& regs : v_regs) {
        auto offset = ::lseek(fd, 0, SEEK_CUR);
        mdata md_regs = {.type = mdata_type::REGS, .offset = offset + sizeof(mdata), .size = sizeof(regs)};
        debug_msg(md_regs);

        ret = filesystem::write(fd, &md_regs, sizeof(md_regs));
        if (ret != sizeof(md_regs)) {
            std::cerr << "Error writing md_regs to file " << file_path << " " << strerror(errno) << std::endl;
            return ret;
        }
        ret = filesystem::write(fd, &regs, sizeof(regs));
        if (ret != sizeof(regs)) {
            std::cerr << "Error writing regs to file " << file_path << " " << strerror(errno) << std::endl;
            return ret;
        }
    }

    auto v_fpregs = p.get_fpregs();
    if (v_fpregs.size() == 0) {
        std::cerr << "Error getting fpregs for pid " << pid << std::endl;
        return ret;
    }

    for (auto& fpregs : v_fpregs) {
        auto offset = ::lseek(fd, 0, SEEK_CUR);
        mdata md_fpregs = {.type = mdata_type::FPREGS, .offset = offset + sizeof(mdata), .size = sizeof(fpregs)};
        debug_msg(md_fpregs);

        ret = filesystem::write(fd, &md_fpregs, sizeof(md_fpregs));
        if (ret != sizeof(md_fpregs)) {
            std::cerr << "Error writing md_fpregs to file " << file_path << " " << strerror(errno) << std::endl;
            return ret;
        }
        ret = filesystem::write(fd, &fpregs, sizeof(fpregs));
        if (ret != sizeof(fpregs)) {
            std::cerr << "Error writing fpregs to file " << file_path << " " << strerror(errno) << std::endl;
            return ret;
        }
    }

    auto v_maps = maps_parser::get_maps(pid);

    std::vector<char> buffer;
    for (auto& map : v_maps) {
        debug_msg(map);
        if (std::strstr(map.pathname, "[vdso]")) continue;
        if (std::strstr(map.pathname, "[vvar]")) continue;
        auto offset = ::lseek(fd, 0, SEEK_CUR);
        mdata md_map = {
            .type = mdata_type::MEMORY_MAP, .offset = offset + sizeof(mdata), .size = sizeof(map) + map.size()};
        debug_msg(md_map);

        ret = filesystem::write(fd, &md_map, sizeof(md_map));
        if (ret != sizeof(md_map)) {
            std::cerr << "Error writing md_map to file " << file_path << " " << strerror(errno) << std::endl;
            return ret;
        }
        ret = filesystem::write(fd, &map, sizeof(map));
        if (ret != sizeof(map)) {
            std::cerr << "Error writing map to file " << file_path << " " << strerror(errno) << std::endl;
            return ret;
        }

        buffer.resize(map.size());
        if (map.prot & PROT_READ) {
            ret = filesystem::remote_read(pid, reinterpret_cast<void*>(map.start_address), buffer.data(), map.size());
            if (ret != static_cast<ssize_t>(map.size())) {
                std::cerr << "Error reading remote data " << ret << " " << strerror(errno) << std::endl;
                return ret;
            }
        }

        ret = filesystem::write(fd, buffer.data(), buffer.size());
        if (ret != static_cast<ssize_t>(buffer.size())) {
            std::cerr << "Error writing remote data to file " << file_path << " " << strerror(errno) << std::endl;
            return ret;
        }
    }

    debug_msg("End");
    return ret;
}

}  // namespace RECK