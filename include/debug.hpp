#pragma once

#include <assert.h>

#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>
#include <cstring>

namespace RECK {

constexpr const char *file_name(const char *path) {
    const char *file = path;
    while (*path) {
        if (*path++ == '/') {
            file = path;
        }
    }
    return file;
}
struct time_stamp {
    friend std::ostream &operator<<(std::ostream &os, [[maybe_unused]] const time_stamp &logger) {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm local_tm = *std::localtime(&now_c);

        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::ostringstream oss;
        oss << std::setw(2) << std::setfill('0') << local_tm.tm_hour << ":" << std::setw(2) << std::setfill('0')
            << local_tm.tm_min << ":" << std::setw(2) << std::setfill('0') << local_tm.tm_sec << ":" << std::setw(3)
            << std::setfill('0') << milliseconds.count();

        os << oss.str();
        return os;
    }
};
class debug_lock {
   public:
    static std::mutex &get_lock() {
        static std::mutex mutex;
        return mutex;
    }
};

#define print_error(out_format)                                                                                 \
    {                                                                                                           \
        std::unique_lock internal_debug_lock(::RECK::debug_lock::get_lock());                                   \
        std::cout << std::dec << ::RECK::time_stamp() << " [ERROR] [" << ::RECK::file_name(__FILE__) << ":"     \
                  << __LINE__ << "] [" << __func__ << "] [" << std::this_thread::get_id() << "] " << out_format \
                  << " : " << std::strerror(errno) << std::endl;                                                \
    }

#ifdef DEBUG
#define debug_msg(out_format)                                                                                     \
    {                                                                                                             \
        std::unique_lock internal_debug_lock(::RECK::debug_lock::get_lock());                                     \
        std::cout << std::dec << ::RECK::time_stamp() << " [" << ::RECK::file_name(__FILE__) << ":" << __LINE__   \
                  << "] [" << __func__ << "] [" << std::this_thread::get_id() << "] " << out_format << std::endl; \
    }
#else
#define debug_msg(out_format)
#endif

#define print(out_format)                                                     \
    {                                                                         \
        std::unique_lock internal_debug_lock(::RECK::debug_lock::get_lock()); \
        std::cout << std::dec << out_format << std::endl;                     \
    }

#define TODO(msg) assert(false && "TODO" && msg)
}  // namespace RECK