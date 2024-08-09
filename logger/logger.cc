#include <chrono>
#include <filesystem>

#include <fmt/core.h>
#include <fmt/chrono.h>

#include "logger.h"

bool Log_closed = false;

namespace fs = std::filesystem;
std::string get_time(TimeLevel level) {
	auto now = std::chrono::system_clock::now();
    auto time_since_epoch = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(time_since_epoch);
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(time_since_epoch) - 
                        std::chrono::duration_cast<std::chrono::microseconds>(seconds);
    
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm* now_tm = std::localtime(&now_c);
    std::string ret;
	switch(level) {
		case TimeLevel::FULL:
			ret = fmt::format("{:%Y-%m-%d %H:%M:%S}.{:06}", *now_tm, microseconds.count());
			break;
		case TimeLevel::DATE:
			ret = fmt::format("{:%Y-%m-%d}", *now_tm);
			break;
		case TimeLevel::HOUR:
			ret = fmt::format("{:%H}", *now_tm);
			break;
		case TimeLevel::MINUTE:
			ret = fmt::format("{:%M}", *now_tm);
			break;
		case TimeLevel::SECOND:
			ret = fmt::format("{:%S}", *now_tm);
			break;
		case TimeLevel::MS:
			ret = fmt::format("{:%Y-%m-%d %H:%M:%S}.{:06}", *now_tm, microseconds.count());
            break;
		default:
			ret = fmt::format("{:%Y-%m-%d %H:%M:%S}", *now_tm);
			break;
	}
	return ret;
}

void Logger::init(const std::string& pathStr, size_t max_lines = 5000, size_t max_queue_size = 800) {
	fs::path path(pathStr);
	if (is_directory(path)) {
		dir_name_ = pathStr;
		log_name_ = get_time(TimeLevel::DATE);
	} else if (is_regular_file(path)) {
		dir_name_ = path.parent_path();
		log_name_ = path.filename();
	} else if (path.has_parent_path() && path.filename() != "." && path.filename() != "..") {
		dir_name_ = path.parent_path();
		log_name_ = path.filename();
	} else {
		std::cerr << "Unsupported file type" << std::endl;
		exit(1);
	}
	today_ = get_time(TimeLevel::DATE);
	// multiple slashs will be seem as a single slash
	ofs_ = std::ofstream(dir_name_ + "/" + log_name_);
	if (!ofs_) {
		std::cerr << "Logger init failed because open file failed" << std::endl;
		exit(1);
	}
	max_queueSize_ =  max_queue_size;
	max_lines_ = max_lines;
	line_cnt_ = 0;
	
	log_queue_ = std::make_unique<BlockQueue<std::string>>(max_queue_size);
	if (max_queue_size > 0)  {
		is_async_ = true;
		log_thread = std::thread(thread_doLog, this);
	}
}

void Logger::async_write_log() {
	std::string single_log;
	for (;;) {
		if (Log_closed == true && log_queue_->size() == 0)
			break;
		log_queue_->pop(single_log);
		ofs_ << single_log;
		ofs_.flush();
	}
}

void Logger::close() {
	Log_closed = true;
}
