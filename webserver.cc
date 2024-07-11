#include "webserver.h"

Config generateConfig() {
	Config ret;
	ret.closeLog = false;
	ret.port = 9006;
	ret.listenTRIG_ = TRIGMode::ET;
	ret.clientTRIG_ = TRIGMode::ET;
	ret.root = "./root";

	ret.url = "localhost";
	ret.user = "root";
	ret.password = "mj2012..";
	ret.DBName = "ginmandb";
	ret.SQLPort = 3306;
	ret.maxConn = 10;

	ret.threadNum = 8;
	ret.maxRequest = 10000;

	ret.path = "./";
	ret.maxLines = 500000;
	ret.maxQueueSize = 0;
	return ret;
}

void WebServer::initLog() {
	Logger::getInstance()->init(config.path, config.maxLines, config.maxQueueSize);
}

void WebServer::initThreadPool() {
 	pThreadPool = std::make_unique<ThreadPool<HttpConn>>(ConnectionPool::getInstance(), config.threadNum, config.maxRequest);
}

void WebServer::initSQL() {
	ConnectionPool::getInstance()->init(config.url,
										config.user,
										config.password,
										config.DBName,
										config.SQLPort,
										config.maxConn,
										config.closeLog);
}

void WebServer::initHttpConn() {
	for (size_t i = 0; i < MAX_HTTP_CONN_NUM; i++) {
		HttpConnArr_[i] = std::make_shared<HttpConn>();
	}
}

void WebServer::init() {
	initLog();
	initSQL();
	initThreadPool();
	initHttpConn();
	initPort();
	initSQLResult(ConnectionPool::getInstance());
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
	address.sin_port = htons(config.port);
	int flag = 1;
	setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
	if (bind(listenFd_, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) < 0) {
		fmt::print("bind socket failed\n");
		exit(1);
	}
	if (listen(listenFd_, 128) < 0) {
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
		HttpConnArr_[static_cast<size_t>(connFd)]->init(connFd, clientAddress, config.root, config.clientTRIG_,
													   config.closeLog, config.user, config.password, config.DBName);
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
			HttpConnArr_[static_cast<size_t>(connFd)]->init(connFd, clientAddress, config.root, config.clientTRIG_,
													   		config.closeLog, config.user, config.password, config.DBName);
		}
	}
	return true;
}

void WebServer::eventLoop() {
	while (true) {
		int number = epoll_wait(HttpConn::epollFd, events_, MAX_EVENT_NUMBER, -1);
		if (number < 0 && errno == EINTR) {
			LOG_ERROR("epoll_wait failed\n");
			break;
		}
		for (int i = 0; i < number; i++) {
			int sockfd = events_[i].data.fd;
			if (sockfd == listenFd_) {
				dealClientConn();
			} else if (events_[i].events & EPOLLIN) {
				pThreadPool->append(HttpConnArr_[static_cast<unsigned>(sockfd)], STATE_RW::READ);
			} else if (events_[i].events & EPOLLOUT) {
				pThreadPool->append(HttpConnArr_[static_cast<unsigned>(sockfd)], STATE_RW::WRITE);
			}
		}
	}
}
