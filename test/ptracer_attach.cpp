#include "ptracer.hpp"
#include <iostream>
#include <unistd.h>
#include <wait.h>
#include <assert.h>

using namespace RECK;

int main(void) {

	pid_t pid = fork();
	assert(pid != -1);
	int status;
	if (pid)
	{
		printf("parent: child pid is %d\n", pid);
        ptracer::allow_pid();
		assert(pid == wait(&status));
		assert(0 == status);
	}
	else
	{
		pid_t tracee = getppid();
		printf("child: parent pid is %d\n", tracee);

        ptracer p{tracee};
        assert(0 == p.init());
        assert(0 != p.get_regs().size());
        assert(0 != p.get_fpregs().size());
        assert(0 == p.detach());
	}
    return 0;
}