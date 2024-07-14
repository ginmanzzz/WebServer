#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <cstring>
#include <unordered_map>

#include "connectionpool.h"

enum class TRIGMode { ET, LT };
void initSQLResult(ConnectionPool::UPtrConnPool& connPool);
int setNonBlock(int fd);
void addfd(int epollfd, int fd, bool one_shot, TRIGMode mode);
void removefd(int epollfd, int fd);
void modfd(int epollfd, int fd, unsigned ev, TRIGMode mode);

class HttpConn {
private:
    using string = std::string;
public:
	constexpr static const int FILENAME_LEN = 200;

    constexpr static const int READ_BUFFER_SIZE = 2048;

    constexpr static const int WRITE_BUFFER_SIZE = 1024;
    // HTTP request method
    enum class METHOD { 
        GET, 
        POST, 
        HEAD, 
        PUT, 
        DELETE, 
        TRACE, 
        OPTIONS, 
        CONNECT, 
        PATCH
    };

    // Main state machine's state
    enum class CHECK_STATE { 
        CHECK_REQUESTLINE, 
        CHECK_HEADER, 
        CHECK_CONTENT 
    };

    // Result of parse request
    enum class HTTP_CODE { 
        NO_REQUEST, 
        GET_REQUEST, 
        BAD_REQUEST, 
        NO_RESOURCE, 
        FORBIDDEN_REQUEST, 
        FILE_REQUEST, 
        INTERNAL_ERROR, 
        CLOSED_CONNECTION 
    };

    // Slave state machine's state
    enum class LINE_STATUS { 
        LINE_OK, 
        LINE_BAD, 
        LINE_OPEN 
    };

public:
	// void init(int sockFd, const sockaddr_in& addr);
	void init(int sockFd, const sockaddr_in& addr, const string root, TRIGMode mode,
			int closeLog, const string& user, const string& password, const string& sqlName);
	
	bool readOnce();
	void process();
	bool writeOnce();
	void closeConn();

    // Main machine
    HTTP_CODE processRead();
    HTTP_CODE parseRequestLine(char* text);
    HTTP_CODE parseHeaders(char* text);
    HTTP_CODE parseContent(char* text);
    
    // Slave machine
    LINE_STATUS parseLine();

	// Only used in test
	friend void testPrint(HttpConn& conn);

private:
    int sockFd_;
    sockaddr_in address_;
    TRIGMode triggerMode_;
    bool closeLog_;
	// files directory
	string docRoot_;

	// READ RELATED
	// Read buf
    char readBuf_[READ_BUFFER_SIZE];
    // The next position of the last byte buffered in read_buf
    int readIdx_;
    // The first index we need to check, indices before we have checked
    int checkedIdx_;
	// The first index of a line
	int startLine_;

	// Main machine state
	CHECK_STATE checkState_;
	// Request method
	METHOD method_;

	// Request parse result
	char realFile_[FILENAME_LEN];
	char* url_;
	char* version_;
	char* host_;
	long long contentLength_;
	bool keepAlive_; 
	char* content_;

    // MYSQL
    string sqlUser_;
    string sqlPassword_;
    string sqlName_;
	
	// WRITE RELATED
	// Write buf
	char writeBuf_[WRITE_BUFFER_SIZE];
	// The length of write buf
	int writeIdx_;
	size_t bytesToSend_;
	size_t bytesHaveSend_;
	// Mmap region
	char* fileAddress_;
	size_t fileSize_;
	struct iovec ioArr_[2];
	int ioCnt;

public:
	static int epollFd;
	static size_t userCount;
	STATE_RW state_rw;
    ConnectionPool::SPtrSQL pSQL;
	static std::mutex idxMtx;

private:
    char* getLine() { return readBuf_ + startLine_; }
    HTTP_CODE doRequest();
	void init();
    template<typename T>
	bool addResponse(T&& s);
	bool addStatusLine(int status, const string& title);
	bool addHeaders(int contentLength);
	bool addContentLength(int contentLength);
	bool addContentType();
	bool addLinkStatus();
	bool addBlankLine();
	bool addContent(const string& s);
    bool processWrite(HTTP_CODE ret);
	void unmap();
};

template<typename T>
bool HttpConn::addResponse(T&& s) { 
    const string& str = std::forward<T>(s);
	if (static_cast<size_t>(writeIdx_) + str.size() > WRITE_BUFFER_SIZE - 1)
		return false;
    memcpy(writeBuf_ + writeIdx_, str.c_str(), str.size());
	writeIdx_ += static_cast<int>(str.size());
	return true;
}
