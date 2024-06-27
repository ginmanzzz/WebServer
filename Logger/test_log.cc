#include <iostream>
#include "logger.h"
#include <thread>
#include <chrono>
using namespace std;
extern bool Log_closed;

int main() {
	string path = "../Logs/";
	Logger::getInstance()->init(path, 500000, 8000);
	for (int i = 0; i <= 20000; i++) {
		LOG_INFO(to_string(i));
	}
	this_thread::sleep_for(std::chrono::seconds(3));
	return 0;
}
