#pragma once
#include <memory>
#include <string>
#include <sys/epoll.h>

#include "logger.h"
#include "threadpool.h"
#include "connectionpool.h"
#include "http.h"

const int MAX_EVENT_NUMBER = 1024;
const int MAX_HTTP_CONN_NUM = 65536;

class WebServer {

private:
	std::unique_ptr<ThreadPool<HttpConn>> pThreadPool;
	int listenFd_;
	struct epoll_event events_[MAX_EVENT_NUMBER];
	std::array<std::shared_ptr<HttpConn>, MAX_HTTP_CONN_NUM> HttpConnArr_;
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
	bool dealClientConn();
	void dealWithRead(int sockfd);
	void dealWithWrite(int sockfd);
};
