#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "server.h"

int main(int argc, char **argv)
{
    int port = 8080;
    const char *root = "www";

    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Puerto invalido\n");
            return 1;
        }
    }
    if (argc > 2)
        root = argv[2];

    signal(SIGPIPE, SIG_IGN);

    return (server_run(port, root) == 0) ? 0 : 1;
}
