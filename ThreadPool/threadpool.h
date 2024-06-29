#include <memory>
#include <thread>
#include <condition_variable>
#include <mutex>

#include <list>
#include <vector>

class ConnectionPool;
class Semaphore {
private:
    size_t count_;
    std::mutex mtx_;
    std::condition_variable cv_;
public:
    Semaphore(int count = 0) : count_(count) { }
    ~Semaphore() = default;

    void notify() {
        std::unique_lock<std::mutex> lock(mtx_);
        count_++;
        cv_.notify_one();
    }

    void wait() {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this](){ return count_ > 0; });
        --count_;
    }
};

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
    std::list<std::unique_ptr<T>> work_queue_;

    // Lock
    std::mutex mtx_;

    // Semaphore to synchronize
    Semaphore queue_stat;

    // Is stopped
    bool stop_;

    // Database connections pool
    std::unique_ptr<ConnectionPool>& p_connPool;

private:
    static void worker(void* args);

    void run();

public:
    ThreadPool(std::unique_ptr<ConnectionPool>& connPool, size_t thread_num = 8, size_t max_requests = 10000);
    ~ThreadPool() = default;

    bool append(std::unique_ptr<T>& request);
};

template<typename T>
ThreadPool<T>::ThreadPool(std::unique_ptr<ConnectionPool>& connPool, size_t thread_num, size_t max_requests) :
    thread_num_(thread_num),
    max_requests_(max_requests),
    arr_threads_(thread_num),
    queue_stat(0),
    stop_(false),
    p_connPool(connPool) {
    if (thread_num == 0 || max_requests == 0)
        throw std::exception();
    
    // should wait for sucesses of initializing private members
    // then create worker threads
    for (int i = 0; i < thread_num; i++) {
        arr_threads_[i] = std::thread(worker, this).detach();
    }
}

template<typename T>
bool ThreadPool<T>::append(std::unique_ptr<T>& request) {
    std::unique_lock<std::mutex> lock(mtx_);

    if (work_queue_.size() >= max_requests_)
        return false;

    work_queue_.push_back(request);
    lock.unlock();
    queue_stat.notify();
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
    while (stop_ == false) {
        // semaphore wait
        queue_stat.wait();

        std::unique_lock<std::mutex> lock(mtx_);
        if (work_queue_.empty())
            continue;
        
        std::unique_ptr<T>& request = work_queue_.front();
        work_queue_.pop_front();
        lock.unlock();

        if (request == nullptr)
            continue;
        
        // request->mysql = p_connPool->getConnection();

        // request->process();

        // p_connPool->releaseConnection(request->mysql);
    }
}

