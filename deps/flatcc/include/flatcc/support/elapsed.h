#ifndef ELAPSED_H
#define ELAPSED_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

/* Based on http://stackoverflow.com/a/8583395 */
#if !defined(_WIN32)
#include <sys/time.h>
static double elapsed_realtime(void) { // returns 0 seconds first time called
    static struct timeval t0;
    struct timeval tv;
    gettimeofday(&tv, 0);
    if (!t0.tv_sec)
        t0 = tv;
    return (double)(tv.tv_sec - t0.tv_sec) + (double)(tv.tv_usec - t0.tv_usec) / 1e6;
}
#else
#include <windows.h>
#ifndef FatalError
#define FatalError(s) do { perror(s); exit(-1); } while(0)
#endif
static double elapsed_realtime(void) { // granularity about 50 microsecs on my machine
	static LARGE_INTEGER freq, start;
    LARGE_INTEGER count;
    if (!QueryPerformanceCounter(&count))
        FatalError("QueryPerformanceCounter");
    if (!freq.QuadPart) { // one time initialization
        if (!QueryPerformanceFrequency(&freq))
            FatalError("QueryPerformanceFrequency");
        start = count;
    }
    return (double)(count.QuadPart - start.QuadPart) / freq.QuadPart;
}
#endif

/* end Based on stackoverflow */

static int show_benchmark(const char *descr, double t1, double t2, size_t size, int rep, const char *reptext)
{
    double tdiff = t2 - t1;
    double nstime;

    printf("operation: %s\n", descr);
    printf("elapsed time: %.3f (s)\n", tdiff);
    printf("iterations: %d\n", rep);
    printf("size: %lu (bytes)\n", (unsigned long)size);
    printf("bandwidth: %.3f (MB/s)\n", (double)rep * (double)size / 1e6 / tdiff);
    printf("throughput in ops per sec: %.3f\n", rep / tdiff);
    if (reptext && rep != 1) {
        printf("throughput in %s ops per sec: %.3f\n", reptext, 1 / tdiff);
    }
    nstime = tdiff * 1e9 / rep;
    if (nstime < 1000) {
        printf("time per op: %.3f (ns)\n", nstime);
    } else if (nstime < 1e6) {
        printf("time per op: %.3f (us)\n", nstime / 1000);
    } else if (nstime < 1e9) {
        printf("time per op: %.3f (ms)\n", nstime / 1e6);
    } else {
        printf("time per op: %.3f (s)\n", nstime / 1e9);
    }
    return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* ELAPSED_H */
