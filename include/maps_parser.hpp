#pragma once

#include <linux/limits.h>
#include <sys/mman.h>

#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <charconv>

namespace RECK {
struct memory_map {
    unsigned long start_address;
    unsigned long end_address;
    unsigned int prot;
    unsigned int flags;
    unsigned long offset;
    unsigned int device_mayor;
    unsigned int device_minor;
    unsigned long inode;
    char pathname[PATH_MAX];

    size_t size() const { return end_address - start_address; }

    friend std::ostream &operator<<(std::ostream &os, const memory_map &region);
};

class maps_parser {
   public:
    static std::vector<memory_map> get_maps(pid_t pid);

    static inline std::optional<unsigned long> parse_ulong(std::string_view sv, int base = 10) {
        unsigned long value = 0;
        auto result = std::from_chars(sv.data(), sv.data() + sv.size(), value, base);
        if (result.ec == std::errc() && result.ptr == sv.data() + sv.size()) {
            return value;
        }
        return std::nullopt;
    }

   private:
    static memory_map parse_line(const std::string_view &line);
};
}  // namespace RECK