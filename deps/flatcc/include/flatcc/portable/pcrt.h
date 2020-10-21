#ifndef PCRT_H
#define PCRT_H

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Assertions and pointer violations in debug mode may trigger a dialog
 * on Windows. When running headless this is not helpful, but
 * unfortunately it cannot be disabled with a compiler option so code
 * must be injected into the runtime early in the main function.
 * A call to the provided `init_headless_crt()` macro does this in
 * a portable manner.
 *
 * See also:
 * https://stackoverflow.com/questions/13943665/how-can-i-disable-the-debug-assertion-dialog-on-windows
 */

#if defined(_WIN32)

#include <crtdbg.h>
#include <stdio.h>
#include <stdlib.h>

static int _portable_msvc_headless_report_hook(int reportType, char *message, int *returnValue)
{
    fprintf(stderr, "CRT[%d]: %s\n", reportType, message);
    *returnValue = 1;
    exit(1);
    return 1;
}

#define init_headless_crt() _CrtSetReportHook(_portable_msvc_headless_report_hook)

#else

#define init_headless_crt() ((void)0)

#endif


#ifdef __cplusplus
}
#endif

#endif /* PCRT_H */
