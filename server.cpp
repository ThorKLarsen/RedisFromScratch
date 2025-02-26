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
#include <assert.h>

using namespace std;

const size_t k_max_msg = 4096;

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

static int32_t read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
		errno = 0;
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
			if (errno == EINTR) { // interrupted by signal, not an error
				clog << "interrupted by signal" << endl;
				continue;
			}
            return -1;  // error, or unexpected EOF
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t write_all(int fd, const char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t one_request(int connfd) {
    // 4 bytes header
    char rbuf[4 + k_max_msg];
	// set errno to 0 before calling read_full
    errno = 0;
    int32_t err = read_full(connfd, rbuf, 4);
    if (err) {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }


    uint32_t len = 0;
    memcpy(&len, rbuf, 4);  // assume little endian
    if (len > k_max_msg) {
        msg("too long");
		cout << "len: " << len << endl;
		cout << rbuf << endl;
        return -1;
    }

    // request body
    err = read_full(connfd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }
    // do something
    printf("client says: %.*s\n", len, &rbuf[4]);
    // reply using the same protocol
    const char reply[] = "world";
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply, len);
    return write_all(connfd, wbuf, 4 + len);
}

// Main function.
int main()
{
	// Define a socket.
	// AF_INET: IPv4 Internet protocols
	// SOCK_STREAM: TCP - connection-based protocol
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		die("socket()");
	}

	// Set the socket option to reuse the address.
	// This allows the server to bind to the same address and port. 
	int val = 1;
	int sockopt_rv = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
	if (sockopt_rv) {
		die("setsockopt()");
	}


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

		while (true) {
			int32_t err = one_request(client_fd);
			if (err) {
                break;
            }
		}

		// Close the client connection.
		close(client_fd);
	}
}
