#include "timer.h"

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <signal.h>
#include <cstring>
#include <assert.h>

int pipefd[2];

void sigHandler(int sig) {
	// save old errno, avoid to be interrupt and change errno,
	// so this function will not change errno
	int saveErrno = errno;
	int msg = sig;

	// the number of signals in linux is less than 255
	write(pipefd[1], reinterpret_cast<char*>(&msg), 1);
	
	errno = saveErrno;
}

void addSignal(int sig, sigHandleType handler, bool restart) {
	// restart means that the signal handler interrupt other syscall, and the syscall will continue
	// after handler finished. without restart, the syscall will be failed
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;

	if (restart)
		sa.sa_flags |= SA_RESTART;
	// accept all signals
	sigfillset(&sa.sa_mask);

	assert(sigaction(sig, &sa, nullptr) != -1);
}
