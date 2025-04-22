#define DEBUG
#include "ptracer.hpp"

#include <linux/prctl.h> /* Definition of PR_* constants */
#include <sys/prctl.h>
#include <wait.h>

#include <algorithm>
#include <filesystem>

#include "debug.hpp"
#include "maps_parser.hpp"

namespace RECK {

ptracer::ptracer(pid_t pid) : m_pid(pid) {
    debug_msg("Begin");
    debug_msg("End");
}

ptracer::~ptracer() {
    debug_msg("Begin");

    if (m_init) {
        detach();
    }

    debug_msg("End");
}

int ptracer::init() {
    int ret = 0;
    debug_msg("Begin");

    std::vector<pid_t> v_ptraced;
    auto all_in = [](const std::vector<pid_t>& v1, const std::vector<pid_t>& v2) -> bool {
        for (auto& pid : v1) {
            if (std::find(v2.begin(), v2.end(), pid) == v2.end()) return false;
        }
        return true;
    };

    // Recursive to ensure that all are stopped
    do {
        m_tasks = get_tasks();
        if (m_tasks.size() == 0) {
            std::cerr << "Error ptracer get tasks" << std::endl;
            return -1;
        }
        debug_msg("Tasks " << m_tasks);
        debug_msg("v_ptraced " << v_ptraced);

        for (auto& pid : m_tasks) {
            if (std::find(v_ptraced.begin(), v_ptraced.end(), pid) != v_ptraced.end()) continue;
            ret = attach(pid);
            if (ret < 0) {
                std::cerr << "Error ptracer attach" << std::endl;
                return -1;
            }
            v_ptraced.emplace_back(pid);
        }
    } while (!all_in(m_tasks, v_ptraced));

    m_init = true;

    debug_msg("End");
    return ret;
}

int ptracer::attach(pid_t pid) {
    int ret = 0;
    debug_msg("Begin (" << pid << ")");

    ret = ::ptrace(PTRACE_ATTACH, pid);
    if (ret < 0) {
        std::cerr << "Error PTRACE_ATTACH " << std::strerror(errno) << std::endl;
        return ret;
    }
    int status = 0;
    ret = ::waitpid(pid, &status, 0);
    if (ret == -1) {
        std::cerr << "Error waitpid " << std::strerror(errno) << std::endl;
        return ret;
    } else if (ret == pid) {
        if (!WIFSTOPPED(status)) {
            std::cerr << "Error waitpid not stopped " << std::strerror(errno) << std::endl;
            return ret;
        }
    } else {
        std::cerr << "Error waitpid ret " << ret << " " << std::strerror(errno) << std::endl;
        return ret;
    }

    debug_msg("End (" << pid << ")");
    return 0;
}

std::vector<pid_t> ptracer::get_tasks() {
    debug_msg("Begin");
    std::vector<pid_t> v_pid;

    namespace fs = std::filesystem;
    std::stringstream task_path;
    task_path << "/proc/" << m_pid << "/task";
    for (const fs::directory_entry& dir_entry : fs::directory_iterator(task_path.str())) {
        auto name = dir_entry.path().filename().string();
        if (auto task_pid = maps_parser::parse_ulong(name); task_pid.has_value()) {
            v_pid.emplace_back(task_pid.value());
        } else {
            return {};
        }
    }

    debug_msg("End");
    return v_pid;
}

std::vector<user_regs_struct> ptracer::get_regs() {
    int ret = 0;
    std::vector<user_regs_struct> v_regs;
    debug_msg("Begin");

    for (auto& pid : m_tasks) {
        auto& regs = v_regs.emplace_back();
        ret = ::ptrace(PTRACE_GETREGS, pid, nullptr, &regs);
        if (ret < 0) {
            std::cerr << "Error PTRACE_GETREGS " << std::strerror(errno) << std::endl;
            return {};
        }
    }

    debug_msg("End");
    return v_regs;
}

std::vector<user_fpregs_struct> ptracer::get_fpregs() {
    int ret = 0;
    std::vector<user_fpregs_struct> v_fpregs;
    debug_msg("Begin");

    for (auto& pid : m_tasks) {
        auto& regs = v_fpregs.emplace_back();
        ret = ::ptrace(PTRACE_GETFPREGS, pid, nullptr, &regs);
        if (ret < 0) {
            std::cerr << "Error PTRACE_GETFPREGS " << std::strerror(errno) << std::endl;
            return {};
        }
    }

    debug_msg("End");
    return v_fpregs;
}

int ptracer::set_regs(const std::vector<user_regs_struct>& v_regs) {
    int ret = 0;
    debug_msg("Begin");
    if (m_tasks.size() != v_regs.size()) {
        std::cerr << "Error set_regs cannot be with diferent size in tasks " << m_tasks.size() << " and regs "
                  << v_regs.size() << std::endl;
        return -1;
    }

    for (size_t i = 0; i < m_tasks.size(); i++) {
        auto& pid = m_tasks[i];
        auto& regs = v_regs[i];
        ret = ::ptrace(PTRACE_SETREGS, pid, nullptr, &regs);
        if (ret < 0) {
            std::cerr << "Error PTRACE_SETREGS " << std::strerror(errno) << std::endl;
            return -1;
        }
    }
    debug_msg("End");
    return 0;
}

int ptracer::set_fpregs(const std::vector<user_fpregs_struct>& v_fpregs) {
    int ret = 0;
    debug_msg("Begin");
    if (m_tasks.size() != v_fpregs.size()) {
        std::cerr << "Error set_fpregs cannot be with diferent size in tasks " << m_tasks.size() << " and fpregs "
                  << v_fpregs.size() << std::endl;
        return -1;
    }

    for (size_t i = 0; i < m_tasks.size(); i++) {
        auto& pid = m_tasks[i];
        auto& fpregs = v_fpregs[i];
        ret = ::ptrace(PTRACE_SETFPREGS, pid, nullptr, &fpregs);
        if (ret < 0) {
            std::cerr << "Error PTRACE_SETFPREGS " << std::strerror(errno) << std::endl;
            return -1;
        }
    }
    debug_msg("End");
    return 0;
}

int ptracer::detach() {
    ssize_t ret = 0;
    debug_msg("Begin");

    ret = ::ptrace(PTRACE_DETACH, m_pid, 0, 0);
    if (ret < 0) {
        std::cerr << "Error PTRACE_DETACH " << std::strerror(errno) << std::endl;
        return ret;
    }

    m_init = false;

    debug_msg("End");
    return ret;
}

int ptracer::allow_pid(pid_t pid) { return prctl(PR_SET_PTRACER, pid); }

}  // namespace RECK