#include "webserver.h"
#include "config.h"

constexpr Config config = generateConfig();

void WebServer::initLog() {
	Logger::getInstance()->init(config.path_, config.maxLines_, config.maxQueueSize_);
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
	address.sin_port = htons(config.port_);
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
		HttpConnArr_[static_cast<size_t>(connFd)]->init(connFd, clientAddress, config.root_, config.clientTRIG_,
													   config.closeLog_, config.user_, config.password_, config.DBName_);
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
		HttpConnArr_[static_cast<unsigned>(sockfd)]->readOnce();
		pThreadPool->append(HttpConnArr_[static_cast<unsigned>(sockfd)]);
	}
}

void WebServer::dealWithWrite(int sockfd) { 
	if (config.modeConcurrency_ == ConcurrencyMode::REACTOR) {
		// reactor
		pThreadPool->append(HttpConnArr_[static_cast<unsigned>(sockfd)], STATE_RW::WRITE);
	} else {
		// proactor
		HttpConnArr_[static_cast<unsigned>(sockfd)]->writeOnce();
	}
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
				dealWithRead(sockfd);
			} else if (events_[i].events & EPOLLOUT) {
				dealWithWrite(sockfd);
			}
		}
	}
}

