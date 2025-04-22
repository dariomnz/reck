#pragma once

#include <sys/user.h>
#include <cstring>

#include "maps_parser.hpp"
#include "ptracer.hpp"

namespace RECK {
class serializer {
   public:
    enum mdata_type {
        REGS,
        FPREGS,
        MEMORY_MAP,
    };

    struct header {
        struct magic_num {
            char data[4];

            bool operator==(const magic_num& other) const {
                return std::memcmp(data, other.data, sizeof(data)) == 0;
            }
            bool operator!=(const magic_num& other) const {
                return std::memcmp(data, other.data, sizeof(data)) != 0;
            }
        };
        magic_num m_num = {'R', 'E', 'C', 'K'};

        static magic_num get_default_magic_num() { return {'R', 'E', 'C', 'K'}; }
        
    };

    struct mdata {
        mdata_type type;
        size_t offset;
        size_t size;

        friend std::ostream& operator<<(std::ostream& os, const mdata& md);
    };

   public:
    static ssize_t restore_serialized_file(const std::string_view& file_path);
    static std::vector<mdata> read_serialized_mdata(const std::string_view& file_path);
    static ssize_t make_checkpoint(const std::string_view& file_path);

    // This need to be called in another process diferent to pid
    static ssize_t dump_serialized_file(pid_t pid, const std::string_view& file_path);
};

}  // namespace RECK