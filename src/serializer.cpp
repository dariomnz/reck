// #define DEBUG

#include "serializer.hpp"

#include <fcntl.h>

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

ssize_t serializer::read_serialized_file(const std::string_view& file_path) {
    ssize_t ret = 0;
    debug_msg("Begin");

    std::string file_path_str{file_path};

    int fd = ::open(file_path_str.c_str(), O_RDONLY);

    if (fd < 0) {
        std::cerr << "Error opening file " << file_path << strerror(errno) << std::endl;
    }

    TODO("read_serialize_file");

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