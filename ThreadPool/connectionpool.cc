#include <iostream>

#include "connectionpool.h"

ConnectionPool::UPtrConnPool& ConnectionPool:: getInstance() {
	static UPtrConnPool p = UPtrConnPool(new ConnectionPool, &ConnectionPoolDeletor);
	return p;
}

ConnectionPool::ConnectionPool() : 
	max_connections_(0),
	free_connections_(0) { }


void ConnectionPool::init(string url, string user, string password, string DBName, unsigned port, size_t maxConn, bool close_log) {
	std::unique_lock<std::mutex> lock(mtx_);
	url_ = url;
	user_ = user;
	password_ = password;
	DB_name_ = DBName;
	port_ = port;
	max_connections_ = maxConn;
	sql_log = close_log;

	for (size_t i = 0; i < maxConn; i++) {
		MYSQL* conn = mysql_init(nullptr);
		if (conn == nullptr) {
			std::cerr << "mysql_init() failed" << std::endl;
			exit(1);
		}
		if (mysql_real_connect(conn, url_.c_str(), user_.c_str(), password_.c_str(), DB_name_.c_str(), port_, nullptr, 0) == nullptr) {
			std::cerr << "mysql_real_connect() failed" << std::endl;
			exit(1);
		}
		conn_list.push_back(SPtrSQL(conn, MYSQLDeletor()));
		free_connections_++;
		reserve_.notify();
	}
	this->max_connections_ = free_connections_;
}


ConnectionPool::SPtrSQL ConnectionPool::getConnection() {
	reserve_.wait();
	std::unique_lock<std::mutex> lock(mtx_);
	if (conn_list.empty())
		return nullptr;
	SPtrSQL conn = conn_list.front();
	conn_list.pop_front();
	return conn;
}

void ConnectionPool::releaseConnection(SPtrSQL conn) {
	if (conn == nullptr)
		return;
	std::unique_lock<std::mutex> lock(mtx_);
	conn_list.push_back(std::move(conn));
	reserve_.notify();
}

size_t ConnectionPool::free_num() {
	return free_connections_;
}

void ConnectionPool::clear() {
	std::unique_lock<std::mutex> lock(mtx_);
	conn_list.clear();
	max_connections_ = free_connections_ = 0;
}

connectionRAII::connectionRAII(std::shared_ptr<MYSQL> conn, std::unique_ptr<ConnectionPool>& connPool) :
	conn_(conn),
	connPool_(connPool) { }

connectionRAII::~connectionRAII() {
	connPool_->releaseConnection(conn_);
}
