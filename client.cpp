#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <assert.h>
#include <string>
#include <vector>

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


static int32_t read_full(int fd, uint8_t *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error, or unexpected EOF
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t write_all(int fd, const uint8_t *buf, size_t n) {
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


static void buf_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len) {
    buf.insert(buf.end(), data, data + len);
}

static int32_t send_req(int fd, const uint8_t *text, size_t len) {
    if (len > k_max_msg) {
        return -1;
    }

    std::vector<uint8_t> wbuf;
    buf_append(wbuf, (const uint8_t *)&len, 4);
    buf_append(wbuf, text, len);
    return write_all(fd, wbuf.data(), wbuf.size());
}

static int32_t read_res(int fd) {
    // 4 bytes header
    std::vector<uint8_t> rbuf;
    rbuf.resize(4);
    errno = 0;
    int32_t err = read_full(fd, &rbuf[0], 4);
    if (err) {
        if (errno == 0) {
            msg("EOF");
        } else {
            msg("read() error");
        }
        return err;
    }

    uint32_t len = 0;
    memcpy(&len, rbuf.data(), 4);  // assume little endian
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    // reply body
    rbuf.resize(4 + len);
    err = read_full(fd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }

    // do something
    printf("len:%u data:%.*s\n", len, len < 100 ? len : 100, &rbuf[4]);
    return 0;
}

// Main function.
int main() {
    // Define a socket.
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    // Connect to the server.
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // 127.0.0.1
    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("connect");
    }

    // messages[3] = (const char *)malloc(1024*4 + 1);
    // memset((void *)messages[3], 'a', 1024*4 + 1);

    // for (const std::string &msg_to_send : messages) {
    //     msg("attempting to send:");
    //     msg(msg_to_send.data());
    //     int32_t err = query(fd, msg_to_send.data());
    //     if (err) {
    //         msg("error");
    //         goto L_DONE;
    //     }
    //     msg("sent successfully");
    //     msg("");
    // }

    const std::vector<std::string> messages = {
        "I can write",
        "I am a client",
        "This is a test",
        // "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890a",
        std::string(k_max_msg, 'z'), // requires multiple event loop iterations
        "hello5",
    };

    for (const std::string &s : messages) {
        int32_t err = send_req(fd, (uint8_t *)s.data(), s.size());
        if (err) { 
            msg("send error");
            msg(s.data());
            goto L_DONE;
         }
    }
    for (size_t i = 0; i < messages.size(); ++i) {
        int32_t err = read_res(fd);
        if (err) {
            msg("read error");
            goto L_DONE;
        }
    }
    // int32_t err = query(fd, "hello1");
    // if (err) {
    //     goto L_DONE;
    // }
    // err = query(fd, "hello2");
    // if (err) {
    //     goto L_DONE;
    // }
    // err = query(fd, "hello3");
    // if (err) {
    //     goto L_DONE;
    // }

L_DONE:
    close(fd);
    return 0;
}