# pragma once
#include <memory>
#include <string>
#include <iostream>
#include <fstream>
#include <mutex>
#include <fmt/core.h>

#include "blockqueue.h"
enum time_level { FULL, DATE, HOUR, MINUTE, SECOND };
std::string get_time(int version = FULL);

extern bool Log_closed;

class Logger {
private:
	// file
	std::string dir_name_;
	std::string log_name_;
	std::string today_;
	std::ofstream ofs_;
	std::mutex mtx_;

	// line
	size_t max_lines_;
	long long line_cnt_;

	// block queue
	bool is_async_;
	size_t max_queueSize_;
	std::unique_ptr<BlockQueue<std::string>> log_queue_;

private:
	Logger() = default; 

	virtual ~Logger() { Log_closed = true;
						log_queue_->close();
						log_queue_->push("");
						ofs_.flush(); }

	static void LoggerDeleter(Logger* ptr) {
		delete ptr;
	}

	void async_write_log();
public:
	enum log_level { INFO, WARN, DEBUG, ERROR };

	using Logger_ptr = std::unique_ptr<Logger, decltype(&LoggerDeleter)>;
public:
	static Logger_ptr& getInstance() {
		// In C++11, static object initialization is thread safe
		static Logger_ptr p(new Logger(), &LoggerDeleter);
		return p;
	}

	void init(const std::string& path, size_t max_lines, size_t max_queue_size);

	template<typename T>
	void write_log(int level, T&& msg);

	// thread work function should be static
	static void thread_doLog(Logger* p) {
		p->async_write_log();
		p->ofs_.flush();
	}

};

template<typename T>
void Logger::write_log(int level, T&& msg) {
	std::string str_level;
	switch(level) {
		case INFO:
			str_level = "[INFO] ";
			break;
		case WARN:
			str_level = "[WARN] ";
			break;
		case DEBUG:
			str_level = "[DEBUG]";
			break;
		case ERROR:
			str_level = "[ERROR]";
			break;
		default:
			str_level = "[INFO] ";
			break;
	}
	std::lock_guard<std::mutex> lock(mtx_);
	line_cnt_++;
	if (today_ != get_time(DATE) || line_cnt_ % max_lines_ == 0) {
		// a new day or reach the limited line number
		ofs_.flush();
		ofs_.close();
		if (today_ != get_time(DATE)) {
			today_ = log_name_ = get_time(DATE);
			ofs_ = std::ofstream(fmt::format("{}/{}", dir_name_, log_name_));
			line_cnt_ = 0;
		} else {
			ofs_ = std::ofstream(fmt::format("{}/{}_{}", dir_name_, log_name_, line_cnt_ / max_lines_));
		}
		if (!ofs_) {
			std::cerr << "Logger recreate a file falied" << std::endl;
			exit(1);
		}
	}
	std::string single_log = fmt::format("{}: {}   {}\n", str_level, get_time(), msg);
	if (is_async_ && log_queue_->full() == false) {
		if (log_queue_->push(single_log) == false) {
			std::cerr << "Fake pemitted to push" << std::endl;
		}
	} else {
		ofs_ << single_log;
		ofs_.flush();
	}
}

template<typename T>
void LOG_INFO(T&& msg) {
	Logger::getInstance()->write_log(Logger::INFO, std::forward<T>(msg));
}
template<typename T>
void LOG_WARN(T&& msg) {
	Logger::getInstance()->write_log(Logger::WARN, std::forward<T>(msg));
}
template<typename T>
void LOG_DEBUG(T&& msg) {
	Logger::getInstance()->write_log(Logger::DEBUG, std::forward<T>(msg));
}
template<typename T>
void LOG_ERROR(T&& msg) {
	Logger::getInstance()->write_log(Logger::ERROR, std::forward<T>(msg));
}

