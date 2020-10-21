#ifndef READFILE_H
#define READFILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>

static char *readfile(const char *filename, size_t max_size, size_t *size_out)
{
    FILE *fp;
    long k;
    size_t size, pos, n, _out;
    char *buf;

    size_out = size_out ? size_out : &_out;

    fp = fopen(filename, "rb");
    size = 0;
    buf = 0;

    if (!fp) {
        goto fail;
    }
    fseek(fp, 0L, SEEK_END);
    k = ftell(fp);
    if (k < 0) goto fail;
    size = (size_t)k;
    *size_out = size;
    if (max_size > 0 && size > max_size) {
        goto fail;
    }
    rewind(fp);
    buf = (char *)malloc(size ? size : 1);
    if (!buf) {
        goto fail;
    }
    pos = 0;
    while ((n = fread(buf + pos, 1, size - pos, fp))) {
        pos += n;
    }
    if (pos != size) {
        goto fail;
    }
    fclose(fp);
    *size_out = size;
    return buf;

fail:
    if (fp) {
        fclose(fp);
    }
    if (buf) {
        free(buf);
    }
    *size_out = size;
    return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* READFILE_H */
