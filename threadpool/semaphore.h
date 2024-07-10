#pragma once
#include <mutex>
#include <condition_variable>

class Semaphore {
private:
    size_t count_;
	// mutex and condition_variable aren't copiable
    std::mutex mtx_;
    std::condition_variable cv_;
    bool stop_;
public:
    Semaphore(size_t count = 0) : 
        count_(count),
        stop_(false) { }
    ~Semaphore() = default;
	Semaphore(const Semaphore&) = delete;
	Semaphore& operator=(const Semaphore&) = delete;


    void notify() {
        std::unique_lock<std::mutex> lock(mtx_);
        count_++;
        cv_.notify_one();
    }

    void wait() {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this](){ return count_ > 0 || stop_; });
        if (!stop_)
            count_--;
    }
    
    void stop() {
        std::unique_lock<std::mutex> lock(mtx_);
        stop_ = true;
        cv_.notify_all();
    }

    bool isStop() {
        return stop_;
    }
};