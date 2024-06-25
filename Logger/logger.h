# pragma once
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <condition_variable>
#include <iostream>

template<typename T>
class BlockQueue {
private:
	size_t max_size_;
	size_t size_;
	size_t r_index_;
	size_t w_index_;
	std::vector<T> buf_;
	std::mutex mtx_;
	std::condition_variable cv_;
public:
	BlockQueue(size_t max_size);
	~BlockQueue() = default;
	template<typename U>
	bool push(U&& item);
	bool pop(T& item);
	bool pop(T& item, size_t ms_timeout);
	size_t size() const;
};

class Logger {
private:
	Logger() = default;
	virtual ~Logger() = default;

	static void LoggerDeleter(Logger* ptr) {
		delete ptr;
	}

public:
	using Logger_ptr = std::unique_ptr<Logger, decltype(&LoggerDeleter)>;

	static Logger_ptr& getInstance() {
		// In C++11, static object initialization is thread safe
		static Logger_ptr p(new Logger(), &LoggerDeleter);
		return p;
	}
	static void log() {
		std::cout << "log" << std::endl;
	}
};

template<typename T>
BlockQueue<T>::BlockQueue(size_t max_size):
	max_size_(max_size),
	size_(0),
	r_index_(0),
	w_index_(0),
	buf_(max_size) {}

template<typename T>
template<typename U>
bool BlockQueue<T>::push(U&& item) {
	std::unique_lock<std::mutex> lock(mtx_);
	if (size() >= max_size_) {
		cv_.notify_all();
		return false;
	}
	buf_[w_index_] = std::forward<U>(item);
	size_++;
	w_index_ = (w_index_ + 1) % max_size_;
	cv_.notify_all();
	return true;
}

template<typename T>
bool BlockQueue<T>::pop(T& item) {
	std::unique_lock<std::mutex> lock(mtx_);
	// cv.wait will block, it use lambda to check size
	cv_.wait(lock, [this](){ return size() > 0;});
	item = buf_[r_index_];
	r_index_ = (r_index_ + 1) % max_size_;
	size_--;
	cv_.notify_all();
	return true;
}

template<typename T>
bool BlockQueue<T>::pop(T& item, size_t ms_timeout) {
	std::unique_lock<std::mutex> lock(mtx_);
	while (size() == 0) {
		// cv_.wait_for won't block, but it need to check size manually
		if (cv_.wait_for(lock, std::chrono::milliseconds(ms_timeout)) == std::cv_status::timeout)
			return false;
	}
	item = buf_[r_index_];
	r_index_ = (r_index_ + 1) % max_size_;
	size_--;
	cv_.notify_all();
	return true;
}

template<typename T>
size_t BlockQueue<T>::size() const {
	return size_;
}
