#ifndef FILES_H
#define FILES_H

#include <stddef.h>
#include <sys/types.h>

/* Busca y abre el archivo pedido dentro de root_real.
 * Devuelve el codigo HTTP: 200, 400, 403, 404 o 500.
 * Si devuelve 200: *fd_out = archivo abierto, *size_out = su tamanio,
 * resolved = ruta real del archivo  */

int files_open(const char *root_real, const char *uri,
               int *fd_out, off_t *size_out,
               char *resolved, size_t resolved_size);

#endif
