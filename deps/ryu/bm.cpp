#include "ryu.h"
#include <benchmark/benchmark.h>

// Register the function as a benchmark
double testdoubles[] = {
//     10,
    0.0000000298023223876953125,
    -57.4025789315173,
    1234567,
    1234567.8
};

static void BM_print(benchmark::State& state) {
    double d = testdoubles[state.range(0)];
    uint32_t precision = state.range(1);
    char buf[50] = {0};
    while (state.KeepRunning()) {
        d2sfixed_buffered_n(d, precision, buf);
    }
}


static void CustomArguments(benchmark::internal::Benchmark* b) {
    for (uint32_t j = 0; j < sizeof(testdoubles) / sizeof(double); j++)
        for (uint32_t i = 0; i <= 4; i++)
            b->Args({j, i});
}
BENCHMARK(BM_print)->Apply(CustomArguments);

// Run the benchmark
BENCHMARK_MAIN();
