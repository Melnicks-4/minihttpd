#include <stdio.h>
#include "server.h"

int main(void)
{
    printf("Iniciando MiniHTTPd...\n");

    if (server_run(8080) < 0) {
        fprintf(stderr, "Error al iniciar el servidor\n");
        return 1;
    }

    return 0;
}
