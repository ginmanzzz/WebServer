#include "timer.h"

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <signal.h>
#include <cstring>
#include <assert.h>

int pipefd[2];

Timer::TimerFuncPtrType Timer::pFunc_ = nullptr;

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


void SortTimerList::addTimer(SPtrTimer timer, SPtrTimer listHead) {
	// listHead should be SortTimerList::head
	SPtrTimer cur = listHead->next_;
	SPtrTimer prev = listHead;
	while (cur != nullptr) {
		// found the position to insert
		if (timer->expire_ < cur->expire_) {
			prev->next_ = timer;
			timer->next_ = cur;
			cur->prev_ = timer;
			timer->prev_ = prev;
			break;
		}
		// not found
		prev = cur;
		cur = cur->next_;
	}
	// arrive at tail, not found, insert to tail
	if (cur == nullptr) {
		prev->next_ = timer;
		timer->prev_ = prev;
		timer->next_ = nullptr;
		tail = timer;
	}
}

void SortTimerList::addTimer(SPtrTimer timer) {
	// invalid insert
	if (timer == nullptr)
		return;
	// list is empty, head and tail are the new timer
	if (head == nullptr) {
		head = tail = timer;
		return;
	}
	// the expire time is earlier than the head, insert to head
	if (timer->expire_ < head->expire_) {
		timer->next_ = head;
		head->prev_ = timer;
		head = timer;
		return;
	}

	addTimer(timer, head);
}

void SortTimerList::updateTimer(SPtrTimer timer) {
	if (timer == nullptr)
		return;
	SPtrTimer tmp = timer->next_;
	// timer is tail, or the next timer is later than this timer
	// no need to change
	if (tmp == nullptr || (timer->expire_ < tmp->expire_)) {
		return;
	}
	// if this timer is head, remove it and insert it again
	if (timer == head) {
		head = head->next_;
		head->prev_ = nullptr;
		timer->next_ = nullptr;
		addTimer(timer, head);
	} else {
		// remove it and insert it again
		timer->prev_->next_ = timer->next_;
		timer->next_->prev_ = timer->prev_;
		addTimer(timer, head);
	}
			
}

void SortTimerList::deleteTimer(SPtrTimer timer) {
	if (timer == nullptr)
		return;
	// if there is only one timer
	if ( (timer == head) && (timer == tail) ) {
		head = nullptr;
		tail = nullptr;
		return;
	}

	// if this timer is the head
	if (timer == head) {
		head = head->next_;
		head->prev_ = nullptr;
		return;
	}

	// if this timer is the tail
	if (timer == tail) {
		tail = tail->prev_;
		tail->next_ = nullptr;
		return;
	}

	timer->prev_->next_ = timer->next_;
	timer->next_->prev_ = timer->prev_;
}

void SortTimerList::tick() {
	if (head == nullptr)
		return;
	time_t curTime = time(nullptr);
	SPtrTimer tmp = head;
	// continously remove head timer 
	while (tmp != nullptr) {
		if (curTime < tmp->expire_)
			break;
		tmp->expireAction();
		head = tmp->next_;
		if (head != nullptr)
			head->prev_ = nullptr;
		tmp = head;
	}
}
