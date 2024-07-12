#pragma once

#include <memory>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <iostream>

#include <list>
#include <vector>

#include "semaphore.h"
#include "connectionpool.h"

enum class ConcurrencyMode { PROACTOR, REACTOR };

template<typename T>
class ThreadPool {
private:
    // The number of threads in the pool
    size_t thread_num_;

    // The max number of requests
    size_t max_requests_;

    // Vector containing threads
    std::vector<std::thread> arr_threads_;

    // Request queue
    std::list<std::shared_ptr<T>> work_queue_;

    // Lock
    std::mutex mtx_;

    // Semaphore to synchronize
    Semaphore queue_stat;

    // Is stopped
    bool stop_;

    // Database connections pool
    ConnectionPool::UPtrConnPool& p_connPool;

    std::vector<unsigned> completed_cnt;

private:
    static void worker(void* args);

    void run();

public:
    ThreadPool(ConnectionPool::UPtrConnPool& connPool, size_t thread_num = 8, size_t max_requests = 10000);
    ~ThreadPool();

    bool append(std::shared_ptr<T> request);
    bool append(std::shared_ptr<T> request, STATE_RW state);
	void terminate();

public:
	ConcurrencyMode modeConcurrency_;
};

template<typename T>
ThreadPool<T>::ThreadPool(ConnectionPool::UPtrConnPool& connPool, size_t thread_num, size_t max_requests) :
    thread_num_(thread_num),
    max_requests_(max_requests),
    arr_threads_(thread_num),
    queue_stat(0),
    stop_(false),
    p_connPool(connPool),
    completed_cnt(thread_num, 0) {
    if (thread_num == 0 || max_requests == 0)
        throw std::exception();
    
    // should wait for sucesses of initializing private members
    // then create worker threads
    for (size_t i = 0; i < thread_num; i++) {
        arr_threads_[i] = std::thread(worker, this);
    }
}

template<typename T>
ThreadPool<T>::~ThreadPool() {
    terminate();
    for (size_t i = 0; i < thread_num_; i++) {
        if (arr_threads_[i].joinable()) {
            std::cout << "Thread<" << arr_threads_[i].get_id() << ">: Completed " << completed_cnt[i] << " requests" << std::endl;
        }
        arr_threads_[i].join();
    }
}
template<typename T>
bool ThreadPool<T>::append(std::shared_ptr<T> request) {
    std::unique_lock<std::mutex> lock(mtx_);

    if (work_queue_.size() >= max_requests_)
        return false;

    work_queue_.push_back(std::move(request));
    lock.unlock();
    queue_stat.notify();
    return true;
}
template<typename T>
bool ThreadPool<T>::append(std::shared_ptr<T> request, STATE_RW state) {
	request->state_rw = state;
	if (append(request) == false)
		return false;
	return true;
}


template<typename T>
void ThreadPool<T>::worker(void* args) {
    ThreadPool* pool = static_cast<ThreadPool*>(args);
    pool->run();
    return;
}

template<typename T>
void ThreadPool<T>::run() {
    size_t thread_index = 0;
    for (; thread_index < thread_num_; thread_index++) {
        if (arr_threads_[thread_index].get_id() == std::this_thread::get_id())
            break;
    }
    while (stop_ == false) {
        // semaphore wait
        queue_stat.wait();
        if (queue_stat.isStop())
            break;

        std::unique_lock<std::mutex> lock(mtx_);
        if (work_queue_.empty())
            continue;
        
        std::shared_ptr<T> request = work_queue_.front();
        work_queue_.pop_front();
        lock.unlock();

        if (request == nullptr)
            continue;
        
		if (modeConcurrency_ == ConcurrencyMode::REACTOR) {
			// reactor
			if (request->state_rw == STATE_RW::READ) {
				request->readOnce();
				connectionRAII mysqlConn(request->pSQL, p_connPool);
				request->process();
			} else {
				request->writeOnce();
			}
		} else {
			// proactor
			connectionRAII mysqlConn(request->pSQL, p_connPool);
			request->process();
		}
        completed_cnt[thread_index]++;
    }
}

template<typename T>
void ThreadPool<T>::terminate() {
	stop_ = true;
    queue_stat.stop();
}
