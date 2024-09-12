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
#include "reference_haversine.cpp"
#include "reference_parser.cpp"

void write_entire_file(Bytes bytes, const char* file_path)
{
    FILE* file = fopen(file_path, "wb");
    Assert(file);
    u64 written = fwrite(bytes.buffer, 1, bytes.size, file);
    Assert(written == bytes.size);
    fclose(file);
}

Bytes read_entire_file(const char* file_path, bool add_null_term)
{
    Bytes bytes;
    
    FILE* file = fopen(file_path, "rb");
    fseek(file, 0, SEEK_END);
    
    bytes.size = ftell(file);
    s32 read_size = bytes.size;
    if (add_null_term) {
        bytes.size++;
    }
    bytes.buffer = (u8*)malloc(bytes.size);

    for (u64 i = 0; i < bytes.size; i += 4*1024) {
        bytes.buffer[i] = i % (1 << 8);
    }
    
    {
        TIME_BANDWIDTH("fread", read_size);

        fseek(file, 0, SEEK_SET);
        fread(bytes.buffer, 1, read_size, file);
        fclose(file);
        if (add_null_term) {
            bytes.buffer[bytes.size-1] = 0;
        }
    }

    return bytes;
}

BytesChunks stream_file_chunks(const char* file_path, bool add_null_term)
{
    //TODO
    return {};
}

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
    Bytes binary = read_entire_file(file_name, false);
    u64 num = binary.size/(sizeof(f64))-1; // Binary contains each individual result and then the averaged result at the end.
    f64* f64_buffer = (f64*)binary.buffer;
    return { f64_buffer, f64_buffer[num], num };
}

s32 main(int arg_count, char** args)
{
    initialize_metrics();
    time_program_start();

    // Parse json
    Bytes json = read_entire_file("../data/data_10000000.json", true);
    HaversineData data = parse_haversine_json(json);

    // Calculate haversines
    HaversineResult result = calculate_haversine(data);

    // Output timing information - ignore the extra work done to verify the results.
    time_program_end_and_print();

    // Parse reference results
    HaversineResult reference_result = get_reference_results("../data/result_10000000.b");

    // Compare calculated results with reference results and output
    compare_results(result, reference_result);

    return 0;
}

ASSERT_ENOUGH_TIMEINFOS;