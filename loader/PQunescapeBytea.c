/***************************************************************************
 * This has been copied from:
 *      PostgreSQL: pgsql/src/interfaces/libpq/fe-exec.c,v
 *	1.165 2004/10/21 19:28:36 tgl Exp 
 *
 * Portions Copyright (c) 1996-2004, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 **************************************************************************/

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include "compat.h"

#if USE_VERSION < 73

#define ISFIRSTOCTDIGIT(CH) ((CH) >= '0' && (CH) <= '3')
#define ISOCTDIGIT(CH) ((CH) >= '0' && (CH) <= '7')
#define OCTVAL(CH) ((CH) - '0')

/*
 *              PQunescapeBytea - converts the null terminated string representation
 *              of a bytea, strtext, into binary, filling a buffer. It returns a
 *              pointer to the buffer (or NULL on error), and the size of the
 *              buffer in retbuflen. The pointer may subsequently be used as an
 *              argument to the function free(3). It is the reverse of PQescapeBytea.
 *
 *              The following transformations are made:
 *              \\       == ASCII 92 == \
 *              \ooo == a byte whose value = ooo (ooo is an octal number)
 *              \x       == x (x is any character not matched by the above transformations)
 */
unsigned char *
PQunescapeBytea(const unsigned char *strtext, size_t *retbuflen)
{
        size_t          strtextlen,
                                buflen;
        unsigned char *buffer,
                           *tmpbuf;
        size_t          i,
                                j;

        if (strtext == NULL)
                return NULL;

        strtextlen = strlen(strtext);

        /*
         * Length of input is max length of output, but add one to avoid
         * unportable malloc(0) if input is zero-length.
         */
        buffer = (unsigned char *) malloc(strtextlen + 1);
        if (buffer == NULL)
                return NULL;

        for (i = j = 0; i < strtextlen;)
        {
                switch (strtext[i])
                {
                        case '\\':
                                i++;
                                if (strtext[i] == '\\')
                                        buffer[j++] = strtext[i++];
                                else
                                {
                                        if ((ISFIRSTOCTDIGIT(strtext[i])) &&
                                                (ISOCTDIGIT(strtext[i + 1])) &&
                                                (ISOCTDIGIT(strtext[i + 2])))
                                        {
                                                int                     byte;

                                                byte = OCTVAL(strtext[i++]);
                                                byte = (byte << 3) + OCTVAL(strtext[i++]);
                                                byte = (byte << 3) + OCTVAL(strtext[i++]);
                                                buffer[j++] = byte;
                                        }
                                }

                                /*
                                 * Note: if we see '\' followed by something that isn't a
                                 * recognized escape sequence, we loop around having done
                                 * nothing except advance i.  Therefore the something will
                                 * be emitted as ordinary data on the next cycle. Corner
                                 * case: '\' at end of string will just be discarded.
                                 */
                                break;

                        default:
                                buffer[j++] = strtext[i++];
                                break;
                }
        }
        buflen = j;                                     /* buflen is the length of the dequoted
                                                                 * data */

        /* Shrink the buffer to be no larger than necessary */
        /* +1 avoids unportable behavior when buflen==0 */
        tmpbuf = realloc(buffer, buflen + 1);

        /* It would only be a very brain-dead realloc that could fail, but... */
        if (!tmpbuf)
        {
                free(buffer);
                return NULL;
        }

        *retbuflen = buflen;
        return tmpbuf;
}
#endif /* USE_VERSION < 73 */
