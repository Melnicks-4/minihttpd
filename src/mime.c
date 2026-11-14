#include <string.h>
#include <strings.h>
#include "mime.h"

struct mime_entry {
    const char *ext;
    const char *type;
};

static const struct mime_entry table[] = {
    {".html", "text/html"},
    {".css",  "text/css"},
    {".js",   "application/javascript"},
    {".png",  "image/png"},
    {".jpg",  "image/jpeg"},
};

const char *mime_from_path(const char *path)
{
    /* nombre del archivo */
    const char *slash = strrchr(path, '/');
    const char *name = slash ? slash + 1 : path;

    const char *dot = strrchr(name, '.');
    if (dot == NULL)
        return "application/octet-stream";

    for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
        if (strcasecmp(dot, table[i].ext) == 0)
            return table[i].type;
    }

    return "application/octet-stream";
}
