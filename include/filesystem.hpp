#pragma once

#include <sys/uio.h>
#include <unistd.h>

#include <debug.hpp>

namespace RECK {

class filesystem {
   public:
    static ssize_t write(int fd, const void* data, size_t len) {
        ssize_t r = 0;
        size_t l = len;
        ssize_t ret = 0;
        const char* buffer = static_cast<const char*>(data);
        debug_msg(">> Begin write(" << fd << ", " << data << ", " << len << ")");

        do {
            r = ::write(fd, buffer, l);
            if (r < 0) {         // fail once
                if (ret == 0) {
                    return r;    // return error if is the first
                } else {
                    return ret;  // return the size of already write
                }
            }
            if (r == 0) break;   // end of file

            l = l - r;
            buffer = buffer + r;
            ret = ret + r;

        } while ((l > 0) && (r > 0));

        debug_msg(">> End write(" << fd << ", " << data << ", " << len << ")= " << ret);
        return ret;
    }

    static ssize_t remote_write(pid_t pid, const void* remote_data, const void* local_data, size_t len) {
        ssize_t r = 0;
        ssize_t ret = 0;
        iovec local_iov = {};
        local_iov.iov_base = const_cast<void*>(local_data);
        local_iov.iov_len = len;
        iovec remote_iov = {};
        remote_iov.iov_base = const_cast<void*>(remote_data);
        remote_iov.iov_len = len;
        debug_msg(">> Begin remote_write(" << pid << ", " << remote_data << ", " << local_data << ", " << len << ")");

        do {
            r = ::process_vm_writev(pid, &local_iov, 1, &remote_iov, 1, 0);
            if (r < 0) {         // fail once
                if (ret == 0) {
                    return r;    // return error if is the first
                } else {
                    return ret;  // return the size of already write
                }
            }
            if (r == 0) break;   // end of file

            local_iov.iov_len -= r;
            remote_iov.iov_len -= r;
            local_iov.iov_base = static_cast<void*>(static_cast<char*>(local_iov.iov_base) + r);
            remote_iov.iov_base = static_cast<void*>(static_cast<char*>(remote_iov.iov_base) + r);
            ret = ret + r;

        } while ((local_iov.iov_len > 0) && (r > 0));

        debug_msg(">> End remote_write(" << pid << ", " << remote_data << ", " << local_data << ", " << len
                                         << ")= " << ret);
        return ret;
    }

    static ssize_t read(int fd, void* data, size_t len) {
        ssize_t r = 0;
        size_t l = len;
        ssize_t ret = 0;
        debug_msg(">> Begin read(" << fd << ", " << data << ", " << len << ")");
        char* buffer = static_cast<char*>(data);

        do {
            r = ::read(fd, buffer, l);
            if (r < 0) {         // fail once
                if (ret == 0) {
                    return r;    // return error if is the first
                } else {
                    return ret;  // return the size of already read
                }
            }
            if (r == 0) break;   // end of file

            l = l - r;
            buffer = buffer + r;
            ret = ret + r;

        } while ((l > 0) && (r > 0));

        debug_msg(">> End read(" << fd << ", " << data << ", " << len << ")= " << ret);
        return ret;
    }

    static ssize_t remote_read(pid_t pid, const void* remote_data, const void* local_data, size_t len) {
        ssize_t r = 0;
        ssize_t ret = 0;
        iovec local_iov = {};
        local_iov.iov_base = const_cast<void*>(local_data);
        local_iov.iov_len = len;
        iovec remote_iov = {};
        remote_iov.iov_base = const_cast<void*>(remote_data);
        remote_iov.iov_len = len;
        debug_msg(">> Begin remote_read(" << pid << ", " << remote_data << ", " << local_data << ", " << len << ")");

        do {
            r = ::process_vm_readv(pid, &local_iov, 1, &remote_iov, 1, 0);
            if (r < 0) {         // fail once
                if (ret == 0) {
                    return r;    // return error if is the first
                } else {
                    return ret;  // return the size of already read
                }
            }
            if (r == 0) break;   // end of file

            local_iov.iov_len -= r;
            remote_iov.iov_len -= r;
            local_iov.iov_base = static_cast<void*>(static_cast<char*>(local_iov.iov_base) + r);
            remote_iov.iov_base = static_cast<void*>(static_cast<char*>(remote_iov.iov_base) + r);
            ret = ret + r;

        } while ((local_iov.iov_len > 0) && (r > 0));

        debug_msg(">> End remote_read(" << pid << ", " << remote_data << ", " << local_data << ", " << len
                                        << ")= " << ret);
        return ret;
    }
};
}  // namespace RECK