#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "files.h"

int files_open(const char *root_real, const char *uri,
               int *fd_out, off_t *size_out,
               char *resolved, size_t resolved_size)
{
    char clean[2048];
    char path[PATH_MAX];
    char real[PATH_MAX];

    size_t i = 0;
    while (uri[i] != '\0' && uri[i] != '?' && uri[i] != '#') {
        if (i + 1 >= sizeof(clean))
            return 400;
        clean[i] = uri[i];
        i++;
    }
    clean[i] = '\0';

    /* "/" significa "/index.html" */
    if (strcmp(clean, "/") == 0)
        snprintf(clean, sizeof(clean), "/index.html");


    /* ruta = raiz + uri */
    int n = snprintf(path, sizeof(path), "%s%s", root_real, clean);
    if (n < 0 || (size_t)n >= sizeof(path))
        return 400;

    /* realpath comprueba que exista */
    if (realpath(path, real) == NULL) {
        if (errno == ENOENT || errno == ENOTDIR)
            return 404;
        if (errno == EACCES)
            return 403;
        return 500;
    }

    /* El resultado debe seguir dentro de la carpeta www */
    size_t rlen = strlen(root_real);
    if (strncmp(real, root_real, rlen) != 0 ||
        (real[rlen] != '/' && real[rlen] != '\0'))
        return 403;

    /* Abrir y verificar que sea un archivo normal */
    int fd = open(real, O_RDONLY);
    if (fd < 0)
        return (errno == EACCES) ? 403 : 500;

    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return 500;
    }
    if (!S_ISREG(st.st_mode)) {         /* directorios */
        close(fd);

        return 403;
    }

    if (snprintf(resolved, resolved_size, "%s", real) >= (int)resolved_size) {
        close(fd);
        return 500;
    }
    *fd_out = fd;
    *size_out = st.st_size;
    return 200;
}
