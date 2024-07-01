#pragma once

#include <memory>
#include <mutex>
#include <list>
#include <string>
#include <mysql/mysql.h>

#include "threadpool.h"  // Semaphore


struct MYSQLDeletor {
	void operator()(MYSQL* conn) const {
		if (conn)
			mysql_close(conn);
	}
};

class ConnectionPool {
private:
	using string = std::string;

public:
	using SPtrSQL = std::shared_ptr<MYSQL>;
	static std::unique_ptr<ConnectionPool>& getInstance();
	void init(string url, string user, string password, string DBName, unsigned port, size_t maxConn, bool close_log);
	SPtrSQL getConnection();
	void releaseConnection(SPtrSQL conn);
	void clear();
	size_t free_num();

private:
	size_t max_connections_;
	size_t free_connections_;
	std::mutex mtx_;
	std::list<SPtrSQL> conn_list;
	Semaphore reserve_;

	string url_;
	string user_;
	string password_;
	string DB_name_;
	unsigned port_;
	bool sql_log;

private:
	ConnectionPool();
	~ConnectionPool() = default;
};

class connectionRAII {
public:
	connectionRAII(std::shared_ptr<MYSQL> conn, std::unique_ptr<ConnectionPool>& connPool);
	~connectionRAII();
private:
	std::shared_ptr<MYSQL> conn_; 
	std::unique_ptr<ConnectionPool>& connPool_;
};
