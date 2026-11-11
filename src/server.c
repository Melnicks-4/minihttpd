#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "server.h"

#define MAX_EVENTS 64

static int set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int create_listen_socket(int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((unsigned short)port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(fd);
        return -1;
    }

    if (listen(fd, 128) < 0) {
        perror("listen");
        close(fd);
        return -1;
    }

    if (set_nonblocking(fd) < 0) {
        perror("fcntl");
        close(fd);
        return -1;
    }

    return fd;
}

static void close_client(int epfd, int fd)
{
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
}

static void handle_client(int epfd, int fd)
{
    char buf[4096];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    if (n == 0) {
        close_client(epfd, fd);
        return;
    }
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        perror("read");
        close_client(epfd, fd);
        return;
    }

    buf[n] = '\0';
    printf("--- fd %d envio %zd bytes ---\n%s", fd, n, buf);

    const char *body = "Hola desde MiniHTTPd (epoll)\n";
    char resp[512];
    int len = snprintf(resp, sizeof(resp),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        strlen(body), body);

    write(fd, resp, (size_t)len);
    close_client(epfd, fd);
}

int server_run(int port)
{
    int listen_fd = create_listen_socket(port);
    if (listen_fd < 0)
        return -1;

    int epfd = epoll_create1(0);
    if (epfd < 0) {
        perror("epoll_create1");
        close(listen_fd);
        return -1;
    }

    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev) < 0) {
        perror("epoll_ctl");
        close(listen_fd);
        close(epfd);
        return -1;
    }

    printf("Escuchando en puerto %d\n", port);

    struct epoll_event events[MAX_EVENTS];

    for (;;) {
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;

            if (fd == listen_fd) {

                for (;;) {
                    int cfd = accept(listen_fd, NULL, NULL);
                    if (cfd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;
                        perror("accept");
                        break;
                    }

                    set_nonblocking(cfd);

                    struct epoll_event cev;
                    memset(&cev, 0, sizeof(cev));
                    cev.events = EPOLLIN;
                    cev.data.fd = cfd;

                    if (epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &cev) < 0) {
                        perror("epoll_ctl add");
                        close(cfd);
                    }
                }
            } else {
                handle_client(epfd, fd);
            }
        }
    }

    close(listen_fd);
    close(epfd);
    return -1;
}
