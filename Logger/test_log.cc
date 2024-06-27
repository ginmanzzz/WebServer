#include <iostream>
#include "logger.h"
using namespace std;

int main() {
	string path = "../Logs/";
	Logger::getInstance()->init(path, 6000000, 100);
	for (int i = 0; i <= 100000; i++) {
		LOG_INFO(to_string(i));
	}
	return 0;
}
