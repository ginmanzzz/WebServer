#include "http.h"
#include <cstring>
#include <unistd.h>

void generateHttpRquest(char* buf) {
    if (buf == nullptr) {
        return;
	}
	memset(buf, '\0', 1024);
    strcat(buf, "POST / HTTP/1.1\r\n");
    strcat(buf, "Host:www.test.com\r\n");
    strcat(buf, "Connection: Keep-Alive\r\n");
    strcat(buf, "Content-length: 5\r\n");
    strcat(buf, "\r\n");
	strcat(buf, "54321");
}

int main() {
	int fds[2];
	HttpConn conn;
	char buf[1024];
	pipe(fds);
	generateHttpRquest(buf);
	conn.init(fds[0]);
	write(fds[1], buf, strlen(buf));
	conn.readOnce();
	conn.processRead();
	testPrint(conn);
}
