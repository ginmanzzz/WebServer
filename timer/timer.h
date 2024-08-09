#pragma once

#include <../http/http.h>

#include <time.h>

using sigHandleType = void(*)(int);

// signal handler
void sigHandler(int sig);

// add signal to observe
void addSignal(int sig, sigHandleType handler, bool restart = true);


class Timer;
struct ClientData;

using SPtrTimer = std::shared_ptr<Timer>;
using SPtrClientData = std::shared_ptr<ClientData>;

struct ClientData {
	sockaddr_in addr_;
	int sockfd_;
	SPtrTimer timer_;
};

class Timer {
public:
	SPtrClientData userData_;
	SPtrTimer prev_;
	SPtrTimer next_;

public:
	using TimerFuncPtrType = void(*)(SPtrClientData);
	static TimerFuncPtrType pFunc_;
	Timer(SPtrClientData userData, SPtrTimer prev = nullptr, SPtrTimer next = nullptr) :
		userData_(userData), prev_(prev), next_(next) { }
	// expire time
	time_t expire_;
	// real do function
	void expireAction() { pFunc_(userData_); }
};

class SortTimerList {
private:
	SPtrTimer head;
	SPtrTimer tail;
	void addTimer(SPtrTimer timer, SPtrTimer ListHead);
public:
	SortTimerList() : head(nullptr), tail(nullptr) { }

	void addTimer(SPtrTimer timer); 
	void updateTimer(SPtrTimer timer);
	void deleteTimer(SPtrTimer timer);
	void tick();
};
