#include "webserver.h"
#include <unistd.h>

int main() {
	std::unique_ptr<WebServer> pServer = std::make_unique<WebServer>();
	pServer->init();
	pServer->eventLoop();
	return 0;
}
