#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/socket.h>

#include "files.h"
#include "http.h"
#include "mime.h"
#include "server.h"

#define MAX_EVENTS       64
#define MAX_FDS          4096
#define WRITE_TIMEOUT_MS 5000

/* Estado de cada cliente conectado */
struct client {
    int    fd;
    char   buf[MAX_REQUEST];   /* bytes recibidos que aun no se procesan */
    size_t len;                /* cuantos bytes hay en buf */
};

static struct client *clients[MAX_FDS];   /* estado de ese cliente */
static char root_real[PATH_MAX];          /* ruta real de la carpeta www */
static volatile sig_atomic_t running = 1;

static void on_sigint(int sig)
{
    (void)sig;
    running = 0;
}

/* Sockets */

static int set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/* passiveTCP: socket + bind + listen */
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

/* Envio de datos */

/* Espera hasta que el socket pueda recibir mas datos */
static int wait_writable(int fd)
{
    struct pollfd p;
    p.fd = fd;
    p.events = POLLOUT;
    p.revents = 0;
    return (poll(&p, 1, WRITE_TIMEOUT_MS) > 0) ? 0 : -1;
}

/* Envia todos los bytes, repitiendo si send() manda solo una parte */
static int send_all(int fd, const char *data, size_t len)
{
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, data + sent, len - sent, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (wait_writable(fd) < 0)
                    return -1;
                continue;
            }
            return -1;
        }
        sent += (size_t)n;
    }
    return 0;
}

/* Envia el archivo completo con sendfile (el kernel copia disco - socket) */
static int send_file(int sock, int file_fd, off_t size)
{
    off_t offset = 0;
    while (offset < size) {
        ssize_t n = sendfile(sock, file_fd, &offset, (size_t)(size - offset));
        if (n < 0) {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (wait_writable(sock) < 0)
                    return -1;
                continue;
            }
            return -1;
        }
        if (n == 0)
            return -1;
    }
    return 0;
}

static void send_error(int fd, int status, int keep_alive)
{
    char body[256];
    char header[512];

    int blen = snprintf(body, sizeof(body),
        "<html><body><h1>%d %s</h1></body></html>\n",
        status, http_status_text(status));
    int hlen = http_build_header(header, sizeof(header), status,
                                 "text/html", blen, keep_alive);
    if (hlen < 0)
        return;
    if (send_all(fd, header, (size_t)hlen) == 0)
        send_all(fd, body, (size_t)blen);
}

/* Clientes */

static void close_client(int epfd, int fd)
{
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
    free(clients[fd]);
    clients[fd] = NULL;
    close(fd);
}

/* Procesa una peticion completa. Devuelve 1 si hay que mantener la
 * conexion abierta, 0 si hay que cerrarla. */
static int process_request(struct client *c, size_t req_len)
{
    struct http_request req;

    int status = http_parse_request(c->buf, req_len, &req);
    if (status != 0) {                     /* 400 o 405 */
        send_error(c->fd, status, 0);
        return 0;
    }

    int file_fd;
    off_t size;
    char real[PATH_MAX];

    status = files_open(root_real, req.uri, &file_fd, &size, real, sizeof(real));
    if (status != 200) {                   /* 400, 403, 404 o 500 */
        send_error(c->fd, status, req.keep_alive);
        return req.keep_alive;
    }

    char header[1024];
    int hlen = http_build_header(header, sizeof(header), 200,
                                 mime_from_path(real), (long)size, req.keep_alive);
    if (hlen < 0) {
        close(file_fd);
        send_error(c->fd, 500, 0);
        return 0;
    }

    int ok = (send_all(c->fd, header, (size_t)hlen) == 0 &&
              send_file(c->fd, file_fd, size) == 0);
    close(file_fd);

    return ok ? req.keep_alive : 0;
}

/* Hay datos para leer de este cliente */
static void handle_client(int epfd, int fd)
{
    struct client *c = clients[fd];

    ssize_t n = read(fd, c->buf + c->len, sizeof(c->buf) - c->len);
    if (n == 0) {                          /* el cliente cerro */
        close_client(epfd, fd);
        return;
    }
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        close_client(epfd, fd);
        return;
    }
    c->len += (size_t)n;

    /* Puede haber una, ninguna o varias peticiones completas en el buffer */
    for (;;) {
        char *end = memmem(c->buf, c->len, "\r\n\r\n", 4);

        if (end == NULL) {
            if (c->len >= sizeof(c->buf)) {
                send_error(fd, 400, 0);
                close_client(epfd, fd);
            }
            return;
        }

        size_t req_len = (size_t)(end - c->buf) + 4;
        if (!process_request(c, req_len)) {
            close_client(epfd, fd);
            return;
        }

        /* Quitar del buffer la peticion ya atendida */
        memmove(c->buf, c->buf + req_len, c->len - req_len);
        c->len -= req_len;
    }
}

/* El socket de escucha tiene conexiones nuevas */
static void accept_clients(int epfd, int listen_fd)
{
    for (;;) {
        int cfd = accept(listen_fd, NULL, NULL);
        if (cfd < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
                perror("accept");
            return;
        }
        if (cfd >= MAX_FDS) {
            close(cfd);
            continue;
        }
        set_nonblocking(cfd);

        struct client *c = calloc(1, sizeof(*c));
        if (c == NULL) {
            close(cfd);
            continue;
        }
        c->fd = cfd;

        struct epoll_event ev;
        memset(&ev, 0, sizeof(ev));
        ev.events = EPOLLIN;
        ev.data.fd = cfd;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev) < 0) {
            perror("epoll_ctl");
            free(c);
            close(cfd);
            continue;
        }
        clients[cfd] = c;
    }
}

int server_run(int port, const char *root)
{
    if (realpath(root, root_real) == NULL) {
        perror("carpeta de archivos (www)");
        return -1;
    }

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

    signal(SIGINT, on_sigint);
    printf("MiniHTTPd en http://localhost:%d  (archivos: %s)\n", port, root_real);

    struct epoll_event events[MAX_EVENTS];

    while (running) {
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;

            if (fd == listen_fd)
                accept_clients(epfd, listen_fd);
            else if (clients[fd] != NULL)
                handle_client(epfd, fd);
        }
    }

    /* Limpieza al salir */
    for (int fd = 0; fd < MAX_FDS; fd++) {
        if (clients[fd] != NULL)
            close_client(epfd, fd);
    }
    close(listen_fd);
    close(epfd);
    printf("\nServidor detenido.\n");
    return 0;
}
