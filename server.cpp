#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <iostream>

using namespace std;

// Error handling functions.
static void die(const char *msg) {
	int err = errno;
	fprintf(stderr, "[%d] %s\n", err, msg);
	abort();
}

// Print a message to stderr.
static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

// Do something with the client connection.
static void do_something(int connfd) {
	char rbuf[64] = {};
	ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);
	if (n < 0) {
		msg("read() error");
		return;
	}
	printf("client says: %s\n", rbuf);

	char wbuf[] = "world";
	write(connfd, wbuf, strlen(wbuf));
}

// Main function.
int main()
{

	// AF_INET: IPv4 Internet protocols
	// SOCK_STREAM: TCP - connection-based protocol
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		die("socket()");
	}

	// Set the socket option to reuse the address.
	// This allows the server to bind to the same address and port. 
	int val = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));


	// Bind the socket to an address and port.
	struct sockaddr_in addr = {}; // IPv4 port pair.
	addr.sin_family = AF_INET;
	addr.sin_port = htons(1234);        // port
	addr.sin_addr.s_addr = htonl(0);    // wildcard IP 0.0.0.0
	int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
	if (rv) {
		die("bind()");
	}


	// Listen for incoming connections.
	rv = listen(fd, 10);
	if (rv) {
		die("listen()");
	}

	while (true) {
		// Accept connection.
		struct sockaddr_in client_addr = {};
		socklen_t client_addr_len = sizeof(client_addr);
		int client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_addr_len);

		if (client_fd < 0) {
			continue; // error
		}

		// Do something with the client connection.
		do_something(client_fd);

		// Close the client connection.
		close(client_fd);
	}

	cout << "Program exited successfully." << endl;
	return 0;
}
