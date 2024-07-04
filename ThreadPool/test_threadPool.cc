#include <iostream>
#include <chrono>
#include <thread>
#include <unordered_set>

#include "connectionpool.h"
#include "threadpool.h"
#include "logger.h"

#include <unistd.h>
#include <fmt/core.h>

// A simple test
int fds[2];
using namespace std;
class Test {
private:
	int i_;
public:
	Test(int i) : i_(i) {}
	void process() {
		write(fds[1], &i_, sizeof(i_));
	}
};

int main() {
	Logger::getInstance()->init("./", 500000, 800);
	ThreadPool<Test> threadPool(ConnectionPool::getInstance(), 4, 2000);
	unordered_set<int> target;
	pipe(fds);
	for (int i = 0; i < 1000; i++) {
		threadPool.append(make_unique<Test>(i));
		target.insert(i);
	}
	fmt::print("Target set completed, its size() = {}\n", target.size());
	std::this_thread::sleep_for(std::chrono::seconds(1));
	for (int j = 0; j < 1000; j++) {
		int num;
		read(fds[0], &num, sizeof(num));
		target.erase(num);
	}
	if (target.size() != 0) {
		fmt::print("test failed\n");
		exit(1);
	}
	fmt::print("test successfully\n");
	return 0;
}
