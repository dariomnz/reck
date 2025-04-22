#pragma once

#include <fcntl.h>
#include <linux/prctl.h>
#include <sys/ptrace.h>
#include <sys/user.h>

#include <vector>

namespace RECK {

class ptracer {
   public:
    ptracer(pid_t pid);
    ~ptracer();

    int init();
    std::vector<user_regs_struct> get_regs();
    std::vector<user_fpregs_struct> get_fpregs();
    int set_regs(const std::vector<user_regs_struct>& v_regs);
    int set_fpregs(const std::vector<user_fpregs_struct>& v_fpregs);
    int detach();

    static int allow_pid(pid_t pid = static_cast<pid_t>(PR_SET_PTRACER_ANY));

   private:
    int attach(pid_t pid);
    std::vector<pid_t> get_tasks();

    pid_t m_pid;
    std::vector<pid_t> m_tasks;
    bool m_init = false;
};

}  // namespace RECK