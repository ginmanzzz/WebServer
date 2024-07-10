
#include "webserver.h"
#include <unistd.h>
Config config;
WebServer wb;

int main() {
	wb.config = std::forward<Config>(generateConfig());
	wb.init();
	wb.eventLoop();
	return 0;
}
