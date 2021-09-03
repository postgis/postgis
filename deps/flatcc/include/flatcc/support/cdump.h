#ifndef CDUMP_H
#define CDUMP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

/* Generates a constant a C byte array. */
static void cdump(const char *name, void *addr, size_t len, FILE *fp) {
    unsigned int i;
    unsigned char *pc = (unsigned char*)addr;

    // Output description if given.
    name = name ? name : "dump";
    fprintf(fp, "const unsigned char %s[] = {", name);

    // Process every byte in the data.
    for (i = 0; i < (unsigned int)len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            fprintf(fp, "\n   ");
        } else if ((i % 8) == 0) {
            fprintf(fp, "   ");
        }

        fprintf(fp, " 0x%02x,", pc[i]);
    }
    fprintf(fp, "\n};\n");
}

#ifdef __cplusplus
}
#endif

#endif /* CDUMP_H */
