#define DEBUG
#include "ptracer.hpp"

#include <linux/prctl.h> /* Definition of PR_* constants */
#include <sys/prctl.h>
#include <wait.h>

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

    ret = attach(m_pid);
    if (ret < 0) {
        std::cerr << "Error ptracer attach" << std::endl;
    }

    m_tasks = get_tasks();
    if (m_tasks.size() == 0) {
        std::cerr << "Error ptracer get tasks" << std::endl;
    }

    m_init = true;

    debug_msg("End");
    return ret;
}

int ptracer::attach(pid_t pid) {
    int ret = 0;
    debug_msg("Begin");

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

    debug_msg("End");
    return 0;
}

std::vector<pid_t> ptracer::get_tasks() {
    int ret = 0;
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
        ret = ::ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs);
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
        ret = ::ptrace(PTRACE_GETFPREGS, m_pid, nullptr, &regs);
        if (ret < 0) {
            std::cerr << "Error PTRACE_GETFPREGS " << std::strerror(errno) << std::endl;
            return {};
        }
    }

    debug_msg("End");
    return v_fpregs;
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