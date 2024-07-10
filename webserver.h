#pragma once
#include <memory>
#include <string>
#include <sys/epoll.h>

#include "logger.h"
#include "threadpool.h"
#include "connectionpool.h"
#include "http.h"

const int MAX_EVENT_NUMBER = 1024;
const int MAX_HTTP_CONN_NUM = 1024;

struct Config {
	bool closeLog;
	std::uint16_t port;
	TRIGMode listenTRIG_;
	TRIGMode clientTRIG_;
	std::string root;

	// MYSQL connection pool
	std::string url;
	std::string user;
	std::string password;
	std::string DBName;
	unsigned SQLPort;
	size_t maxConn;

	// threadpool
	size_t threadNum;
	size_t maxRequest;
	
	// logger
	std::string path;
	size_t maxLines;
	size_t maxQueueSize;
};

Config generateConfig();

class WebServer {

private:
	std::unique_ptr<ThreadPool<HttpConn>> pThreadPool;
	int listenFd_;
	struct epoll_event events_[MAX_EVENT_NUMBER];
	std::array<std::shared_ptr<HttpConn>, MAX_HTTP_CONN_NUM> HttpConnArr_;
public:
	Config config;
	void init();
	void eventLoop();

private:
	void initLog();
	void initSQL();
	void initThreadPool();
	void initPort();
	void initHttpConn();
	bool dealClientConn();
};
