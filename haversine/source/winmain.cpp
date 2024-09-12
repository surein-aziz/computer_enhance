#include <stdio.h>
#include <time.h>

#include "windows.h"
#include "../../common/helpful.h"
#include "data.h"

// Enable profiler
#define PROFILER

// Single unit compilation
#include "../../common/platform_metrics.cpp"
#include "instrumentation.cpp"
#include "file_io.cpp"
#include "reference_haversine.cpp"
#include "reference_parser.cpp"

void compare_results(HaversineResult result, HaversineResult reference_result) {
    // For now, just compare number processed and resultant average
    if (result.count == reference_result.count) {
        printf("Processed %d calculations, as expected.\n", (s32)result.count);
    } else {
        printf("Processed %d calculations.\nExpected to process %d calculations.\nERROR!\n", (s32)result.count, (s32)reference_result.count);
    }

    f64 delta = abs(result.average - reference_result.average);
    if (delta < 1e-6) {
        printf("Result was within tolerance of %.17g, as expected.\nDelta was %.17g.\n", reference_result.average, delta);
    } else {
        printf("Result was %.17g.\nExpected result was %.17g.\nERROR!\n", result.average, reference_result.average);
    }
}

HaversineResult get_reference_results(const char* file_name)
{
    Bytes binary = read_entire_file(file_name);
    u64 num = binary.size/(sizeof(f64))-1; // Binary contains each individual result and then the averaged result at the end.
    f64* f64_buffer = (f64*)binary.buffer;
    return { f64_buffer, f64_buffer[num], num };
}

int main()
{
    initialize_metrics();
    time_program_start();

    // Parse json
    BytesChunks json_chunks = begin_file_read_chunks("../data/data_10000000.json", 1024*1024, 1024);
    HaversineData data = parse_haversine_json(json_chunks);

    // Calculate haversines
    HaversineResult result = calculate_haversine(data);

    // Output timing information - ignore the extra work done to verify the results.
    time_program_end_and_print();
    printf("\n");

    // Parse reference results
    HaversineResult reference_result = get_reference_results("../data/result_10000000.b");

    // Compare calculated results with reference results and output
    compare_results(result, reference_result);

    return 0;
}

ASSERT_ENOUGH_TIMEINFOS;