#pragma once
#include "http.h"
#include "threadpool.h"

struct Config {
	bool closeLog_;
	std::uint16_t port_;
	TRIGMode listenTRIG_;
	TRIGMode clientTRIG_;
	const char* root_;

	// MYSQL connection pool
	const char* url_;
	const char* user_;
	const char* password_;
	const char* DBName_;
	unsigned SQLPort_;
	size_t maxConn_;

	// threadpool
	size_t threadNum_;
	size_t maxRequest_;
	
	// logger
	const char* path_;
	size_t maxLines_;
	size_t maxQueueSize_;

	// proactor OR reactor
	ConcurrencyMode modeConcurrency_;

	constexpr Config(bool cLog, std::uint16_t port, TRIGMode listenTRIG, TRIGMode clientTRIG, const char* root,
					const char* url, const char* user, const char* password, const char* DBName, unsigned SQLPort, size_t maxConn,
					size_t threadNum, size_t maxRequest,
					const char* path, size_t maxLines, size_t maxQueueSize,
					ConcurrencyMode modeConcurrency) :
		closeLog_(cLog), port_(port), listenTRIG_(listenTRIG), clientTRIG_(clientTRIG), root_(root),
		url_(url), user_(user), password_(password), DBName_(DBName), SQLPort_(SQLPort), maxConn_(maxConn),
		threadNum_(threadNum), maxRequest_(maxRequest),
		path_(path), maxLines_(maxLines), maxQueueSize_(maxQueueSize),
		modeConcurrency_(modeConcurrency) { }
};

constexpr Config generateConfig() {
	return Config{	
		true,    		// close log
	 	9006,  			// web port
		TRIGMode::ET,   // listen socket mode
		TRIGMode::ET,   // client socket mode
		"./root",       // the file's root 

		"localhost",    // MYSQL URL
		"root",         // MYSQL user name
		"mj2012..",     // MYSQL password
		"ginmandb",     // MYSQL table name
		3306,           // MYSQL port
		10,             // MYSQL connectionPool maxConnections

		4,              // threadPool maxConnections
		20000,          // threadPool maxRequest

		"./",           // log file path
		500000,         // maxLines in a log file
		0,              // The logger's blockQueue's max size, if size > 0, it will be asyn

		// ConcurrencyMode::REACTOR
		ConcurrencyMode::PROACTOR
	};
}

