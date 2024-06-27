#include <fmt/core.h>
#include <fmt/chrono.h>
#include <chrono>
#include <filesystem>
#include "logger.h"

bool Log_closed = false;

namespace fs = std::filesystem;
std::string get_time(int version) {
	auto now = std::chrono::system_clock::now();
    // 将时间点转换为 time_t 类型
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    // 将 time_t 类型转换为 tm 结构
    std::tm* now_tm = std::localtime(&now_c);
	std::string ret;
	switch(version) {
		case FULL:
			ret = fmt::format("{:%Y-%m-%d %H:%M:%S}", *now_tm);
			break;
		case DATE:
			ret = fmt::format("{:%Y-%m-%d}", *now_tm);
			break;
		case HOUR:
			ret = fmt::format("{:%H}", *now_tm);
			break;
		case MINUTE:
			ret = fmt::format("{:%M}", *now_tm);
			break;
		case SECOND:
			ret = fmt::format("{:%S}", *now_tm);
			break;
		default:
			ret = fmt::format("{:%Y-%m-%d %H:%M:%S}", *now_tm);
			break;
	}
	return ret;
}

void Logger::init(const std::string& pathStr, size_t max_lines = 5000, size_t max_queue_size = 800) {
	fs::path path(pathStr);
	if (is_regular_file(path)) {
		dir_name_ = path.parent_path();
		log_name_ = path.filename();
	} else if (is_directory(path)) {
		dir_name_ = pathStr;
		log_name_ = get_time(DATE);
	} else {
		std::cerr << "Unsupported file type" << std::endl;
		exit(1);
	}
	today_ = get_time(DATE);
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
		std::thread log_thread(thread_doLog, this);
		log_thread.detach();
	}

}

void Logger::async_write_log() {
	std::string single_log;
	while (log_queue_->pop(single_log) && Log_closed == false) {
		std::lock_guard<std::mutex> guard(mtx_);
		ofs_ << single_log;
	}
}
