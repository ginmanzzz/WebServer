#pragma once
#include <memory>
#include <string>
#include <sys/epoll.h>

#include "logger.h"
#include "threadpool.h"
#include "connectionpool.h"
#include "http.h"
#include "timer.h"

constexpr int MAX_EVENT_NUMBER = 65536;
constexpr int MAX_HTTP_CONN_NUM = 65536;
constexpr unsigned TIME_SLOT = 30;

class WebServer {

private:
	std::unique_ptr<ThreadPool<HttpConn>> pThreadPool;
	int listenFd_;
	struct epoll_event events_[MAX_EVENT_NUMBER];
	std::array<std::shared_ptr<HttpConn>, MAX_HTTP_CONN_NUM> HttpConnArr_;
	std::array<std::shared_ptr<ClientData>, MAX_HTTP_CONN_NUM> userDataArr_;
	SortTimerList sortTimerList_;
public:
	void init();
	void eventLoop();
	ConcurrencyMode getConcurrencyMode();

private:
	void initLog();
	void initSQL();
	void initThreadPool();
	void initPort();
	void initHttpConn();
	void initTimer();
	void setTimer(sockaddr_in& addr, int sockfd);
	bool dealClientConn();
	void dealWithRead(int sockfd);
	void dealWithWrite(int sockfd);
	bool dealWithSignal(bool& timeout, bool& stopServer);
	void dealWithClose(int sockfd);
};

void expireFunction(SPtrClientData pData);
