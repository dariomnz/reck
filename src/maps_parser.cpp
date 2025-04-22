// #define DEBUG

#include "maps_parser.hpp"

#include <algorithm>
#include <sstream>
#include <fstream>

#include "debug.hpp"
namespace RECK {

std::ostream &operator<<(std::ostream &os, const memory_map &region) {
    os << std::hex << region.start_address << "-" << std::hex << region.end_address;
    os << " (size " << std::dec << region.size() << ") ";
    if (region.prot & PROT_READ) {
        os << "r";
    } else {
        os << "-";
    }
    if (region.prot & PROT_WRITE) {
        os << "w";
    } else {
        os << "-";
    }
    if (region.prot & PROT_EXEC) {
        os << "x";
    } else {
        os << "-";
    }
    if (region.flags & MAP_PRIVATE) {
        os << "p";
    } else if (region.flags & MAP_SHARED) {
        os << "s";
    }

    os << " " << std::hex << region.offset;
    os << " " << region.device_mayor << ":" << region.device_minor;
    os << " " << std::dec << region.inode;
    if (region.pathname[0] != '\0') {
        os << " " << region.pathname;
    }
    return os;
}

std::vector<memory_map> maps_parser::get_maps(pid_t pid) {
    std::vector<memory_map> memory_maps;
    std::string maps_file_path = "/proc/" + std::to_string(pid) + "/maps";
    std::ifstream maps_file(maps_file_path);
    std::string line;

    if (maps_file.is_open()) {
        while (std::getline(maps_file, line)) {
            memory_maps.push_back(std::move(parse_line(line)));
            debug_msg(memory_maps[memory_maps.size() - 1]);
        }
        maps_file.close();
    } else {
        std::cerr << "Error: Could not open the file: " << maps_file_path << std::endl;
    }

    return memory_maps;
}


memory_map maps_parser::parse_line(const std::string_view &line_view) {
    memory_map map_entry = {};

    debug_msg("Line to parse: '" << line_view << "'");
    auto current = line_view.cbegin();
    const auto end = line_view.cend();

    // 1. Address (start-end)
    auto space1 = std::find(current, line_view.cend(), ' ');
    if (space1 != end) {
        std::string_view address_sv(current, std::distance(current, space1));
        auto hyphen = std::find(address_sv.cbegin(), address_sv.cend(), '-');
        if (hyphen != address_sv.cend()) {
            std::string_view start_sv(address_sv.cbegin(), std::distance(address_sv.cbegin(), hyphen));
            std::string_view end_sv((hyphen + 1), std::distance(hyphen + 1, address_sv.cend()));
            debug_msg("Start " << start_sv);
            debug_msg("End " << start_sv);
            if (auto start_addr = parse_ulong(start_sv, 16); start_addr.has_value()) {
                map_entry.start_address = start_addr.value();
                if (auto end_addr = parse_ulong(end_sv, 16); end_addr.has_value()) {
                    map_entry.end_address = end_addr.value();
                    current = space1 + 1;
                } else {
                    std::cerr << "Warning: Error parsing end address: " << line_view << std::endl;
                    return map_entry;
                }
            } else {
                std::cerr << "Warning: Error parsing start address: " << line_view << std::endl;
                return map_entry;
            }
        } else {
            std::cerr << "Warning: Hyphen not found in address: " << line_view << std::endl;
            return map_entry;
        }
    } else {
        std::cerr << "Warning: Missing space after address: " << line_view << std::endl;
        return map_entry;
    }

    // 2. Permissions
    auto space2 = std::find(current, line_view.cend(), ' ');
    if (space2 != end) {
        std::string_view permisions_sv(current, std::distance(current, space2));
        debug_msg("Permissions " << permisions_sv);
        current = space2 + 1;
        if (permisions_sv[0] == 'r') {
            map_entry.prot |= PROT_READ;
        }
        if (permisions_sv[1] == 'w') {
            map_entry.prot |= PROT_WRITE;
        }
        if (permisions_sv[2] == 'x') {
            map_entry.prot |= PROT_EXEC;
        }
        if (permisions_sv[3] == 'p') {
            map_entry.flags |= MAP_PRIVATE;
        } else if (permisions_sv[3] == 's') {
            map_entry.flags |= MAP_SHARED;
        }
    } else {
        std::cerr << "Warning: Missing space after permissions: " << line_view << std::endl;
        return map_entry;
    }

    // 3. Offset
    auto space3 = std::find(current, line_view.cend(), ' ');
    if (space3 != end) {
        std::string_view offset_sv(current, std::distance(current, space3));
        debug_msg("Offset " << offset_sv);
        if (auto offset = parse_ulong(offset_sv, 16); offset.has_value()) {
            map_entry.offset = offset.value();
            current = space3 + 1;
        } else {
            std::cerr << "Warning: Error parsing offset: " << line_view << std::endl;
            return map_entry;
        }
    } else {
        std::cerr << "Warning: Missing space after offset: " << line_view << std::endl;
        return map_entry;
    }

    // 4. Device
    auto space4 = std::find(current, line_view.cend(), ' ');
    if (space4 != end) {
        std::string_view device_sv(current, std::distance(current, space4));
        debug_msg("Device " << device_sv);
        current = space4 + 1;

        map_entry.device_mayor = parse_ulong(device_sv.substr(0, 2), 16).value_or(0);
        map_entry.device_minor = parse_ulong(device_sv.substr(3, 2), 16).value_or(0);
    } else {
        std::cerr << "Warning: Missing space after device: " << line_view << std::endl;
        return map_entry;
    }

    // 5. Inode
    auto space5 = std::find(current, line_view.cend(), ' ');
    if (space5 != end) {
        std::string_view inode_sv(current, std::distance(current, space5));
        debug_msg("Inode " << inode_sv);
        if (auto inode = parse_ulong(inode_sv); inode.has_value()) {
            map_entry.inode = inode.value();
            current = space5 + 1;
        } else {
            std::cerr << "Warning: Error parsing inode: " << line_view << std::endl;
            return map_entry;
        }
    } else {
        std::cerr << "Warning: Missing space after inode: " << line_view << std::endl;
        return map_entry;
    }

    // 6. Pathname (remaining part of the line)
    std::string_view pathname_sv(current, std::distance(current, end));
    size_t first_non_space = pathname_sv.find_first_not_of(' ');
    if (first_non_space != std::string_view::npos) {
        pathname_sv = pathname_sv.substr(first_non_space);
    }
    if (pathname_sv.size() > 0) {
        debug_msg("Pathname " << pathname_sv);
        pathname_sv.copy(map_entry.pathname, std::min(pathname_sv.size(), sizeof(map_entry.pathname)));
    }

    return map_entry;
}
}  // namespace RECK