#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <filesystem>
#include <sys/mman.h>
#include <fmt/core.h>
#include <mutex>
#include <unordered_map>
#include <sys/uio.h>

#include "http.h"
#include "logger.h"

int HttpConn::epollFd = -1;
size_t HttpConn::userCount = 0; 
std::mutex idxMtx;

std::string ok200Title = 	"OK";
std::string error400Title = "Bad Request";
std::string error400Form = 	"Your request has bad syntax or is inherently impossible to staisfy.\n";
std::string error403Title = "Forbidden";
std::string error403Form = 	"You do not have permission to get file from the server.\n";
std::string error404Title = "Not found";
std::string error404Form = 	"The request file was not found on this server.\n";
std::string error500Title = "Internal Error";
std::string error500Form = 	"There was an unusal problem serving the request file.\n";

namespace fs = std::filesystem;

using HTTP_CODE = HttpConn::HTTP_CODE;
using METHOD = HttpConn::METHOD;
using CHECK_STATE = HttpConn::CHECK_STATE;
using LINE_STATUS = HttpConn::LINE_STATUS;

// mutex to synchronize SQL modification
std::mutex SQL_Mutex;
std::unordered_map<std::string, std::string> user2Password;

void initSQLResult(ConnectionPool::UPtrConnPool& connPool) {
	ConnectionPool::SPtrSQL p = nullptr;
	connectionRAII RAII(p, connPool);
	if (mysql_query(p.get(), "SELECT username, passwd FROM user")) {
		LOG_ERROR(fmt::format("SELECT error:{}\n", mysql_error(p.get())));
	}
	std::unique_lock<std::mutex> lock(SQL_Mutex);
	MYSQL_RES* res = mysql_store_result(p.get());
	while (MYSQL_ROW row = mysql_fetch_row(res)) {
		user2Password[row[0]] = row[1];
	}
}

int setNonBlock(int fd) {
	// F_GETFL to get old flags
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	// F_SETFL to set new flags
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}

// register fd into epoll event
void addfd(int epollfd, int fd, bool one_shot, TRIGMode mode) {
	epoll_event event;
	event.data.fd = fd;

	if (mode == TRIGMode::LT)  {
		event.events = EPOLLIN | EPOLLRDHUP;
	} else {
		event.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
	}
	if (one_shot)
		event.events |= EPOLLONESHOT;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
	setNonBlock(fd);
}

void removefd(int epollfd, int fd) {
	epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
	close(fd);
}

// reset one shot
void modfd(int epollfd, int fd, unsigned ev, TRIGMode mode) {
	epoll_event event;
	event.data.fd = fd;

	if (mode == TRIGMode::LT)
		event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
	else
		event.events = ev | EPOLLONESHOT | EPOLLRDHUP | EPOLLET;
	
	epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}


// read data into buf and increased readIdx
bool HttpConn::readOnce() {
	if (readIdx_ >= READ_BUFFER_SIZE)
		return false;
	ssize_t bytes_read = 0;
	if (triggerMode_ == TRIGMode::ET) {
		while (true) {
			bytes_read = recv(sockFd_, readBuf_+readIdx_, static_cast<size_t>(READ_BUFFER_SIZE-readIdx_), 0);
			// bytes_read = read(sockFd_, readBuf_+readIdx_, static_cast<size_t>(READ_BUFFER_SIZE-readIdx_));
			if (bytes_read == -1) {
				// Non-block should read data as much as possible
				// EAGAIN equal to EWOULDBLOCK, depend on OS
				if (errno == EAGAIN || EWOULDBLOCK)
					break;
				return false;
			} else if (bytes_read == 0) 
				// == 0 means that the connection has been closed
				return false;
			readIdx_ += static_cast<int>(bytes_read);
		}
	} else {
		bytes_read = recv(sockFd_, readBuf_+readIdx_, static_cast<size_t>(READ_BUFFER_SIZE-readIdx_), 0);
		// bytes_read = read(sockFd_, readBuf_+readIdx_, static_cast<size_t>(READ_BUFFER_SIZE-readIdx_));
		readIdx_ += static_cast<int>(bytes_read);
		if (bytes_read <= 0)
			return false;
	}
	LOG_INFO(fmt::format("received client request:\n{}", readBuf_));
	return true;
}

// process data in buf, increase checkedIdx
HTTP_CODE HttpConn::processRead() {
	LINE_STATUS lineStatus = LINE_STATUS::LINE_OK;
	HTTP_CODE ret = HTTP_CODE::NO_REQUEST;
	char* text = 0;
	while ( (checkState_ == CHECK_STATE::CHECK_CONTENT && lineStatus == LINE_STATUS::LINE_OK) ||
			(lineStatus = parseLine()) == LINE_STATUS::LINE_OK) {
		text = getLine();
		startLine_ = checkedIdx_;
		
		switch (checkState_) {
			case CHECK_STATE::CHECK_REQUESTLINE:
				ret = parseRequestLine(text);
				if (ret == HTTP_CODE::BAD_REQUEST)
					return HTTP_CODE::BAD_REQUEST;
				break;
			case CHECK_STATE::CHECK_HEADER:
				ret = parseHeaders(text);
				if (ret == HTTP_CODE::BAD_REQUEST)
					return HTTP_CODE::BAD_REQUEST;
				else if (ret == HTTP_CODE::GET_REQUEST)
					return doRequest();
				break;
			case CHECK_STATE::CHECK_CONTENT:
				ret = parseContent(text);
				if (ret == HTTP_CODE::GET_REQUEST)
					return doRequest();
				lineStatus = LINE_STATUS::LINE_OPEN;
				break;
			default:
				return HTTP_CODE::INTERNAL_ERROR;
		}
	}
	return HTTP_CODE::NO_REQUEST;
}

// strpbrk is used to look for the first blank or tab
// strspn is used to pass redundant blanks and tabs
HTTP_CODE HttpConn::parseRequestLine(char* text) {
	url_ = strpbrk(text, " \t");
	if (url_ == nullptr)
		return HTTP_CODE::BAD_REQUEST;
	*url_++ = 0;

	char* method = text;
	if (strcasecmp(method, "GET") == 0)
		method_ = METHOD::GET;
	else if (strcasecmp(method, "POST") == 0)
		method_ = METHOD::POST;
	else
		return HTTP_CODE::BAD_REQUEST;
	
	url_ += strspn(url_, " \t");

	version_ = strpbrk(url_, " \t");
	if (version_ == nullptr)
		return HTTP_CODE::BAD_REQUEST;
	
	*version_++ = '\0';
	version_ += strspn(version_, " \t");

	if (strcasecmp(version_, "HTTP/1.1") != 0)
		return HTTP_CODE::BAD_REQUEST;
	
	if (strlen(url_) == 1)
		strcat(url_, "judge.html");
	
	checkState_ = CHECK_STATE::CHECK_HEADER;
	return HTTP_CODE::NO_REQUEST;
}

// parse a line, replace "\r\n" with "\0\0"
LINE_STATUS HttpConn::parseLine() {
	char temp;
	for (; checkedIdx_ < readIdx_; checkedIdx_++) {
		temp = readBuf_[checkedIdx_];
		if (temp == '\r') {
			if (checkedIdx_ + 1 == readIdx_)
				return LINE_STATUS::LINE_OPEN;
			else if (readBuf_[checkedIdx_ + 1] == '\n') {
				readBuf_[checkedIdx_++] = '\0';
				readBuf_[checkedIdx_++] = '\0';
				return LINE_STATUS::LINE_OK;
			}
			return LINE_STATUS::LINE_BAD;
		} else if (temp == '\n') {
			if (checkedIdx_ > 1 && readBuf_[checkedIdx_ - 1] == '\r') {
				readBuf_[checkedIdx_ - 1] = '\0';
				readBuf_[checkedIdx_++] = '\0';
				return LINE_STATUS::LINE_OK;
			}
			return LINE_STATUS::LINE_BAD;
		}
	}
	return LINE_STATUS::LINE_OPEN;
}

HTTP_CODE HttpConn::parseHeaders(char* text) {
	// blank line, can switch to check content
	if (text[0] == '\0') {
		if (contentLength_ != 0) {
			checkState_ = CHECK_STATE::CHECK_CONTENT;
			return HTTP_CODE::NO_REQUEST;
		}
		return HTTP_CODE::GET_REQUEST;
	} else if (strncasecmp(text, "Connection:", 11) == 0) {
		text += 11;
		text += strspn(text, " \t");
		if (strcasecmp(text, "keep-alive") == 0)
			keepAlive_ = true;
	} else if (strncasecmp(text, "Host:", 5) == 0) {
		text += 5;
		text += strspn(text, " \t");
		host_ = text;
	} else if (strncasecmp(text, "Content-length:", 15) == 0) {
		text += 15;
		text += strspn(text, " \t");
		contentLength_ = atol(text);
	} else
		printf("unknown header: %s\n", text);
	return HTTP_CODE::NO_REQUEST;
}

HTTP_CODE HttpConn::parseContent(char* text) {
	if (readIdx_ >= (contentLength_ + checkedIdx_)) {
		text[contentLength_] = '\0';
		content_ = text;
		return HTTP_CODE::GET_REQUEST;
	}
	return HTTP_CODE::NO_REQUEST;
}

void testPrint(HttpConn& conn) {
	std::cout << "URL = " << conn.url_ << std::endl;
	std::cout << "Version = " << conn.version_ << std::endl;
	std::cout << "Host = " << conn.host_ << std::endl;
	std::cout << "ContentLength = " << conn.contentLength_ << std::endl;
	std::cout << "KeepAlive = " << conn.keepAlive_ << std::endl;
	std::cout << "Content = " << conn.content_ << std::endl;
}

void HttpConn::init(int sockFd, const sockaddr_in& addr, const string root, TRIGMode mode,
					int closeLog, const string& user, const string& password, const string& sqlName) {
	sockFd_ = sockFd;
	address_ = addr;
	docRoot_ = root;
	triggerMode_ = mode;
	closeLog_ = closeLog;
	sqlUser_ = user;
	sqlPassword_ = password;
	sqlName_ = sqlName;
	addfd(epollFd, sockFd, true, mode);
	if (mode == TRIGMode::ET)
		setNonBlock(sockFd_);
	init();
	std::lock_guard<std::mutex> guard(idxMtx);
	userCount++;
}

// private init
void HttpConn::init() {
	readIdx_ = 0;
	checkedIdx_ = 0;
	startLine_ = 0;
	writeIdx_ = 0;
	checkState_ = CHECK_STATE::CHECK_REQUESTLINE;
	url_ = nullptr;
	version_ = nullptr;
	host_ = nullptr;
	content_ = nullptr;
	contentLength_ = 0;
	keepAlive_ = false;
	bytesToSend_ = 0;
	bytesHaveSend_ = 0;
	fileAddress_ = nullptr;

	memset(readBuf_, '\0', READ_BUFFER_SIZE);
	memset(writeBuf_, '\0', WRITE_BUFFER_SIZE);
	memset(realFile_, '\0', FILENAME_LEN);
}

HTTP_CODE HttpConn::doRequest() {
	strcpy(realFile_, docRoot_.c_str());
	size_t len = docRoot_.size();
	const char* p = strrchr(url_, '/');
	// log in or register check
	if (method_ == METHOD::GET && (*(p+1) == '2' || *(p+1) == '3')) {
		;
	}
	if (*(p+1) == '0') {
		// register page
		string tmp = "/register.html";
		strncpy(realFile_ + len, tmp.c_str(), tmp.size());
	} else if (*(p+1) == '1') {
		// log in page
		string tmp = "/log.html";
		strncpy(realFile_ + len, tmp.c_str(), tmp.size());
	} else {
		// file request
		strncpy(realFile_ + len, url_, FILENAME_LEN - len - 1);
	}
	auto status = fs::status(realFile_);
	if (status.type() == fs::file_type::not_found)
		return HTTP_CODE::NO_RESOURCE;
	// can't read
	if ((status.permissions() & fs::perms::owner_read) == fs::perms::none)
		return HTTP_CODE::FORBIDDEN_REQUEST;
	// only can read regular file
	if (status.type() != fs::file_type::regular)
		return HTTP_CODE::BAD_REQUEST;
	int fd = open(realFile_, O_RDONLY);
	fileSize_ = static_cast<size_t>(fs::file_size(realFile_));
	// mmap will be continously valid, even if fd has been closed
	fileAddress_ = static_cast<char*>(mmap(nullptr, fileSize_, PROT_READ, MAP_PRIVATE, fd, 0));
	close(fd);
	return HTTP_CODE::FILE_REQUEST;
} 

bool HttpConn::addStatusLine(int status, const string& title) {
	return addResponse(fmt::format("HTTP/1.1 {} {}\r\n", status, title));
}

bool HttpConn::addHeaders(int contentLength) {
	return addContentLength(contentLength) && addLinkStatus() && addBlankLine();
}

bool HttpConn::addContentType() {
	return addResponse(fmt::format("Content-Type:text/html\r\n"));
}

bool HttpConn::addLinkStatus() {
	return addResponse(fmt::format("Connection:{}\r\n", keepAlive_ == true? "keep-alive" : "close"));
}

bool HttpConn::addBlankLine() {
	return addResponse(fmt::format("\r\n"));
}

bool HttpConn::addContent(const string& s) {
	return addResponse(s);
}

bool HttpConn::addContentLength(int contentLength) {
	return addResponse(fmt::format("Content-Length:{}\r\n", contentLength));
}

bool HttpConn::processWrite(HTTP_CODE ret) {
	switch(ret) {
		case HTTP_CODE::INTERNAL_ERROR:
			addStatusLine(500, error500Title);
			addHeaders(static_cast<int>(error500Form.size()));
			if (!addContent(error500Form))
				return false;
			break;
		case HTTP_CODE::BAD_REQUEST:
			addStatusLine(400, error400Title);
			addHeaders(static_cast<int>(error400Form.size()));
			if (!addContent(error400Form))
				return false;
			break;
		case HTTP_CODE::FORBIDDEN_REQUEST:
			addStatusLine(403, error403Title);
			addHeaders(static_cast<int>(error403Form.size()));
			if (!addContent(error403Form))
				return false;
			break;
		case HTTP_CODE::NO_RESOURCE:
			addStatusLine(404, error404Title);
			addHeaders(static_cast<int>(error404Form.size()));
			if (!addContent(error404Form))
				return false;
			break;
		case HTTP_CODE::FILE_REQUEST:
			addStatusLine(200, ok200Title);
			if (fileSize_ != 0) {
				addHeaders(static_cast<int>(fileSize_));
				ioArr_[0].iov_base = writeBuf_;
				ioArr_[0].iov_len = static_cast<size_t>(writeIdx_);
				ioArr_[1].iov_base = fileAddress_;
				ioArr_[1].iov_len = fileSize_;
				ioCnt = 2;
				bytesToSend_ = static_cast<size_t>(writeIdx_) + fileSize_;
				return true;
			} else {
				const string emptyHTML = "<html><body></body></html>";
				addHeaders(static_cast<int>(emptyHTML.size()));
				if (!addContent(emptyHTML))
					return false;
			}
			break;
		default:
			return false;
	}
	ioArr_[0].iov_base = writeBuf_;
	ioArr_[0].iov_len = static_cast<size_t>(writeIdx_);
	ioCnt = 1;
	bytesToSend_ = static_cast<size_t>(writeIdx_);
	return true;
}

void HttpConn::process() {
	HTTP_CODE ret = processRead();
	if (ret == HTTP_CODE::NO_REQUEST) {
		modfd(epollFd, sockFd_, EPOLLIN, triggerMode_);
		return;
	}
	if (processWrite(ret) == false) {
		LOG_ERROR(fmt::format("The write buffer is too small\n"));
		exit(1);
	}
	modfd(epollFd, sockFd_, EPOLLOUT, triggerMode_);
}

void HttpConn::closeConn() {
	if (sockFd_ == -1)
		return;
	removefd(epollFd, sockFd_);
	sockFd_ = -1;
	std::lock_guard<std::mutex> guard(idxMtx);
	userCount--;
}
bool HttpConn::writeOnce() {
	ssize_t temp = 0;
	if (bytesToSend_ == 0) {
		modfd(epollFd, sockFd_, EPOLLIN, triggerMode_);
		init();
		return true;
	}
	while (1) {
		temp = writev(sockFd_, ioArr_, ioCnt);
		if (temp < 0) {
			if (errno == EAGAIN) {
				modfd(epollFd, sockFd_, EPOLLOUT, triggerMode_);
				return true;
			}
			fmt::print("write failed\n");
			unmap();
			return false;
		}
		bytesHaveSend_ += static_cast<size_t>(temp);
		bytesToSend_ -= static_cast<size_t>(temp);
		if (bytesHaveSend_ >= ioArr_[0].iov_len) {
			ioArr_[0].iov_len = 0;
			ioArr_[1].iov_base = fileAddress_ + bytesHaveSend_ - writeIdx_;
			ioArr_[1].iov_len = bytesToSend_;
		} else {
			ioArr_[0].iov_base = writeBuf_ + bytesHaveSend_;
			ioArr_[0].iov_len -= bytesHaveSend_;
		}
		if (bytesToSend_ <= 0) {
			unmap();
			modfd(epollFd, sockFd_, EPOLLIN, triggerMode_);
			if (keepAlive_) {
				init();
				return true;
			}
			else
				return false;
		}
	}
	return false;
}

void HttpConn::unmap() {
	if (fileAddress_ != nullptr) {
		munmap(fileAddress_, fileSize_);
		fileAddress_ = nullptr;
	}
}
