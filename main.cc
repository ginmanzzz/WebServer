#include "webserver.h"
#include <unistd.h>

WebServer wb;

int main() {
	wb.init();
	wb.eventLoop();
	return 0;
}
