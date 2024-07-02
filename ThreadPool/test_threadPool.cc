#include <iostream>
#include <chrono>
#include <thread>

#include "connectionpool.h"
#include "threadpool.h"

using namespace std;
class Test {
public:
	void process() {
		;
	}
};

int main() {
	ThreadPool<Test> threadPool(ConnectionPool::getInstance(), 4, 2000);
	for (int i = 0; i < 1000; i++) {
		threadPool.append(std::move(make_unique<Test>()));
	}
	std::this_thread::sleep_for(std::chrono::seconds(3));
}
