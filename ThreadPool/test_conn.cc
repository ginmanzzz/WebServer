#include "connectionpool.h"
#include <mysql/mysql.h>
#include <iostream>
#include <fmt/core.h>
#include <vector>


using SPtrSQL = ConnectionPool::SPtrSQL;
using namespace std;


// config
string url = "localhost",
		user = "root",
		password = "mj2012..",
		DBName = "ginmandb";
unsigned port = 9006; 
size_t maxConn = 5;
bool close_log = true;

// worker
bool printSearch(MYSQL* connection) {
	if (connection == nullptr)
		return false;

	if (mysql_query(connection, "SELECT * FROM user")) {
		std::cerr << "SELECT * FROM user failed. Error:" <<  mysql_error(connection) << std::endl;
		return false;
	}

	MYSQL_RES* res = mysql_store_result(connection);
	if (res == nullptr) {
		std::cerr << "mysql_store_result() failed. Error: " << mysql_error(connection) << std::endl;
        return false;
	}

	int num_fields = static_cast<int>(mysql_num_fields(res));
	MYSQL_ROW row;

	while ((row = mysql_fetch_row(res))) {
        for(int i = 0; i < num_fields; i++) {
            std::cout << (row[i] ? row[i] : "NULL") << " ";
        }
        std::cout << std::endl;
    }
    mysql_free_result(res);
	return true;
}

bool singleSearch() {
	SPtrSQL conn = nullptr;
	// RAII 
	connectionRAII connRAII(conn, ConnectionPool::getInstance());
	if (ConnectionPool::getInstance()->free_num() != maxConn - 1 || printSearch(conn.get()) == false) {
		fmt::print("Test0: singleSearch -> False\n");
		return false;
	}
	fmt::print("Test0: singleSearch -> OK\n");
	return true;
}

bool allUse() {
	vector<SPtrSQL> pointers(maxConn, nullptr);
	connectionRAII connRAII_0(pointers[0], ConnectionPool::getInstance());
	connectionRAII connRAII_1(pointers[1], ConnectionPool::getInstance());
	connectionRAII connRAII_2(pointers[2], ConnectionPool::getInstance());
	connectionRAII connRAII_3(pointers[3], ConnectionPool::getInstance());
	connectionRAII connRAII_4(pointers[4], ConnectionPool::getInstance());
	if (ConnectionPool::getInstance()->free_num() != 0) {
		fmt::print("Test1: all links used  -> False\n");
		return false;
	}
	for (size_t i = 0; i < maxConn; i++) {
		fmt::print("link{}, print:\n", i);
		printSearch(pointers[i].get());
	}
	fmt::print("Test1: all links used  -> OK\n");
	return true;
}
int main() {
	ConnectionPool::getInstance()->init(url, user, password, DBName, port, maxConn, close_log);
	singleSearch();
	allUse();
	if (ConnectionPool::getInstance()->free_num() != maxConn)
		fmt::print("Used links were not be given back\n");
	return 0;
}
