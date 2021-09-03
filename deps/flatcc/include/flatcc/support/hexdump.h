#ifndef HEXDUMP_H
#define HEXDUMP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

/* Based on: http://stackoverflow.com/a/7776146 */
static void hexdump(const char *desc, const void *addr, size_t len, FILE *fp) {
    unsigned int i;
    unsigned char buf[17];
    const unsigned char *pc = (const unsigned char*)addr;

    /* Output description if given. */
    if (desc != NULL) fprintf(fp, "%s:\n", desc);

    for (i = 0; i < (unsigned int)len; i++) {

        if ((i % 16) == 0) {
            if (i != 0) fprintf(fp, "  |%s|\n", buf);
            fprintf(fp, "%08x ", i);
        } else if ((i % 8) == 0) {
            fprintf(fp, " ");
        }
        fprintf(fp, " %02x", pc[i]);
        if ((pc[i] < 0x20) || (pc[i] > 0x7e)) {
            buf[i % 16] = '.';
        } else {
            buf[i % 16] = pc[i];
        }
        buf[(i % 16) + 1] = '\0';
    }
    if (i % 16 <= 8 && i % 16 != 0) fprintf(fp, " ");
    while ((i % 16) != 0) {
        fprintf(fp, "   ");
        i++;
    }
    fprintf(fp, "  |%s|\n", buf);
}

#ifdef __cplusplus
}
#endif

#endif /* HEXDUMP_H */
