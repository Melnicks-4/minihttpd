#ifndef FILES_H
#define FILES_H

#include <stddef.h>

int file_read(const char *uri, char *buffer, size_t capacity, size_t *file_size);

#endif
