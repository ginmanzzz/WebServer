#include "webserver.h"
#include "config.h"

#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <arpa/inet.h>

constexpr Config config = generateConfig();

// defind in timer.cc
extern int pipefd[2];

void WebServer::initLog() {
	Logger::getInstance()->init(config.path_, config.maxLines_, config.maxQueueSize_);
	if (config.closeLog_)
		Logger::getInstance()->close();
}

void WebServer::initThreadPool() {
 	pThreadPool = std::make_unique<ThreadPool<HttpConn>>(ConnectionPool::getInstance(), config.threadNum_, config.maxRequest_);
	pThreadPool->modeConcurrency_ = config.modeConcurrency_;
}

void WebServer::initSQL() {
	ConnectionPool::getInstance()->init(config.url_,
										config.user_,
										config.password_,
										config.DBName_,
										config.SQLPort_,
										config.maxConn_,
										config.closeLog_);
}

void WebServer::initHttpConn() {
	for (size_t i = 0; i < MAX_HTTP_CONN_NUM; i++) {
		HttpConnArr_[i] = std::make_shared<HttpConn>();
		userDataArr_[i] = std::make_shared<ClientData>();
	}
}

void WebServer::init() {
	initLog();
	initSQL();
	initThreadPool();
	initHttpConn();
	initPort();
	initSQLResult(ConnectionPool::getInstance());
	initTimer();
	LOG_INFO(fmt::format("Server init successfully\n"));
}

void WebServer::initPort() {
	listenFd_ = socket(PF_INET, SOCK_STREAM, 0);
	if (listenFd_ < 0) {
		fmt::print("listen socket init failed\n");
		exit(1);
	}
	struct sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(config.port_);
	int flag = 1;
	setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
	if (bind(listenFd_, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) < 0) {
		fmt::print("bind socket failed\n");
		exit(1);
	}
	if (listen(listenFd_, 1024) < 0) {
		fmt::print("listen socket failed\n");
		exit(1);
	}
	HttpConn::epollFd = epoll_create1(0);
	if (HttpConn::epollFd == -1) {
		fmt::print("epoll create failed\n");
		exit(1);
	}
	setNonBlock(listenFd_);
	struct epoll_event event;
	event.data.fd = listenFd_;
	event.events = EPOLLIN;
	if (epoll_ctl(HttpConn::epollFd, EPOLL_CTL_ADD, listenFd_, &event) < 0) {
		fmt::print("epoll register listen socket failed\n");
		exit(1);
	}
}

void WebServer::initTimer() {
	assert(pipe(pipefd) != -1);
	// set pipefd nonblock, write fd can't block, it will affect the main task
	// read fd can't block, because it should be register in epoll event
	setNonBlock(pipefd[1]);
	// addfd will set nonblock
	addfd(HttpConn::epollFd, pipefd[0], false, TRIGMode::ET);
	addSignal(SIGALRM, sigHandler, true);
	addSignal(SIGTERM, sigHandler, true);
	// set expire function member
	Timer::pFunc_ = expireFunction;
	// start the timer
	alarm(TIME_SLOT);
}

void WebServer::setTimer(sockaddr_in& addr, int sockfd) {
	SPtrClientData pd = userDataArr_[static_cast<unsigned>(sockfd)];
	pd->addr_ = addr;
	pd->sockfd_ = sockfd;
	// default timer
	SPtrTimer pt = std::make_shared<Timer>(pd);
	pt->userData_ = pd;
	pd->timer_ = pt;
	time_t curTime = time(nullptr);
	pt->expire_ = curTime + 3 * TIME_SLOT;
	sortTimerList_.addTimer(pt);
}

bool WebServer::dealClientConn() {
	struct sockaddr_in clientAddress;
	socklen_t clientAddressLength = sizeof(clientAddress);
	if (config.listenTRIG_ == TRIGMode::LT) {
		int connFd = accept(listenFd_, (struct sockaddr*)&clientAddress, &clientAddressLength);
		if (connFd < 0) {
			LOG_ERROR(fmt::format("accept error: errno is :{}\n", errno));
			return false;
		}
		if (HttpConn::userCount >= MAX_HTTP_CONN_NUM) {
			LOG_ERROR(fmt::format("server is too busy, failed to connect new client\n"));
			return false;
		}
		HttpConnArr_[static_cast<size_t>(connFd)]->init(connFd, clientAddress, config.root_, config.clientTRIG_,
													   config.closeLog_, config.user_, config.password_, config.DBName_);
		LOG_INFO(fmt::format("New client:{} connected, port is {}, sockfd is {}\n", 
			inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port), connFd));
		setTimer(clientAddress, connFd);
	} else {
		while (1) {
			int connFd = accept(listenFd_, (struct sockaddr*)&clientAddress, &clientAddressLength);
			if (connFd < 0) {
				if (errno != EAGAIN)
					LOG_ERROR(fmt::format("accept error: errno is :{}\n", errno));
				return false;
			}
			if (HttpConn::userCount >= MAX_HTTP_CONN_NUM) {
				LOG_ERROR(fmt::format("server is too busy, failed to connect new client\n"));
				return false;
			}
			HttpConnArr_[static_cast<size_t>(connFd)]->init(connFd, clientAddress, config.root_, config.clientTRIG_,
													   		config.closeLog_, config.user_, config.password_, config.DBName_);
			LOG_INFO(fmt::format("New client:{} connected, port is {}, sockfd is {}\n", 
				inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port), connFd));
			setTimer(clientAddress, connFd);
		}
	}
	return true;
}

void WebServer::dealWithRead(int sockfd) {
	if (config.modeConcurrency_ == ConcurrencyMode::REACTOR) {
		// reactor
		pThreadPool->append(HttpConnArr_[static_cast<unsigned>(sockfd)], STATE_RW::READ);
	} else {
		// proactor
		if (HttpConnArr_[static_cast<unsigned>(sockfd)]->readOnce() == false)
			dealWithClose(sockfd);
		pThreadPool->append(HttpConnArr_[static_cast<unsigned>(sockfd)]);
	}
}

void WebServer::dealWithWrite(int sockfd) { 
	if (config.modeConcurrency_ == ConcurrencyMode::REACTOR) {
		// reactor
		pThreadPool->append(HttpConnArr_[static_cast<unsigned>(sockfd)], STATE_RW::WRITE);
	} else {
		// proactor
		if (HttpConnArr_[static_cast<unsigned>(sockfd)]->writeOnce() == false)
			dealWithClose(sockfd);
	}
}

bool WebServer::dealWithSignal(bool& timeout, bool& stopServer) {
	ssize_t ret = 0;
	char signals[128];
	ret = read(pipefd[0], signals, sizeof(signals));
	if (ret == -1 || ret == 0) {
		return false;
	}
	for (int i = 0; i < ret; i++) {
		switch (signals[i]) {
			case SIGALRM:
				timeout = true;
				break;
			case SIGTERM:
				stopServer = true;
				break;
		}
	}
	return true;
}

void WebServer::dealWithClose(int sockfd) {
	auto pTimer = userDataArr_[static_cast<unsigned>(sockfd)]->timer_;
	// remove fd from epollfd, and close fd
	pTimer->expireAction();
	sortTimerList_.deleteTimer(pTimer);
}

void WebServer::eventLoop() {
	bool timeout = false;
	bool stopServer = false;
	while (!stopServer) {
		int number = epoll_wait(HttpConn::epollFd, events_, MAX_EVENT_NUMBER, -1);
		if (number < 0 && errno != EINTR) {
			// errno == EINTR because the signal will interrupt epoll
			LOG_ERROR("epoll_wait failed\n");
			break;
		}
		for (int i = 0; i < number; i++) {
			int sockfd = events_[i].data.fd;
			if (sockfd == listenFd_) {
				dealClientConn();
			} else if (sockfd == pipefd[0] && events_[i].events & EPOLLIN) {
				dealWithSignal(timeout, stopServer);
			} else if (events_[i].events & EPOLLIN) {
				dealWithRead(sockfd);
			} else if (events_[i].events & EPOLLOUT) {
				dealWithWrite(sockfd);
			} else if (events_[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
				dealWithClose(sockfd);
			}
			if (timeout) {
				// timer handler:
				// remove expired timers
				sortTimerList_.tick();
				timeout = false;
				// restart the alarm
				alarm(TIME_SLOT);
			}
		}
	}
}

void expireFunction(SPtrClientData pData) {
	// equal to http::closeConn
	removefd(HttpConn::epollFd, pData->sockfd_);
	std::lock_guard<std::mutex> guard(HttpConn::idxMtx);
	HttpConn::userCount--;
}
