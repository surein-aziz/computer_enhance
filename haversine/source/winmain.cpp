#include <stdio.h>
#include <time.h>

#include "windows.h"
#include "helpful.h"

// Single unit compilation
#include "reference_haversine.cpp"

struct Bytes {
    u8* buffer = 0; // malloced buffer
    s32 size = 0;
};

struct HaversineData {
    f64* x0 = 0;
    f64* y0 = 0;
    f64* x1 = 0;
    f64* y1 = 0;
    u64 count = 0;
};

struct HaversineResult {
    f64* results = 0;
    f64 average = 0.0;
    u64 count = 0;
};

void write_entire_file(Bytes bytes, const char* file_path)
{
    FILE* file = fopen(file_path, "wb");
    Assert(file);
    u64 written = fwrite(bytes.buffer, 1, bytes.size, file);
    Assert(written == bytes.size);
    fclose(file);
}

Bytes read_entire_file(const char* file_path)
{
    Bytes bytes;
    
    FILE* file = fopen(file_path, "rb");
    fseek(file, 0, SEEK_END);
    
    bytes.size = ftell(file);
    bytes.buffer = (u8*)malloc(bytes.size);
    
    fseek(file, 0, SEEK_SET);
    fread(bytes.buffer, 1, bytes.size, file);
    fclose(file);
    
    return bytes;
}

HaversineData parse_haversine_json(Bytes json) {
    //TODO(surein): implement
    return {};
}

HaversineResult calculate_haversine(HaversineData data) {
    HaversineResult result;
    result.results = (f64*)malloc(data.count*sizeof(f64));
    result.count = data.count;

    // Use an estimate of the earth's radius for the radius, arbitrarily.
    f64 radius = 6372.8;
    f64 average = 0.0;
    for (u64 i = 0; i < result.count; ++i) {
        result.results[i] = ReferenceHaversine(data.x0[i], data.y0[i], data.x1[i], data.y1[i], radius);
        average += result.results[i];
    }
    average = average / result.count;

    return result;
}

void compare_results(HaversineResult result, HaversineResult reference_result) {
    // For now, just compare number processed and resultant average
    if (result.count == reference_result.count) {
        printf("Processed %d calculations, as expected.\n", (s32)result.count);
    } else {
        printf("Processed %d calculations.\nExpected to process %d calculations.\nERROR!\n", (s32)result.count, (s32)reference_result.count);
    }

    if (result.average == reference_result.average) {
        printf("Result was %.17g, as expected.\n", result.average);
    } else {
        printf("Result was %.17g.\nExpected result was %.17g.\nERROR!\n", result.average, reference_result.average);
    }
}

s32 APIENTRY WinMain(HINSTANCE instance,
                     HINSTANCE prev_instance,
                     LPTSTR cmd_line,
                     int show)
{
    // Parse json
    Bytes json = read_entire_file("../data/data_10000000.json");
    HaversineData data = parse_haversine_json(json);

    // Calculate haversines
    HaversineResult result = calculate_haversine(data);

    // Parse reference results
    Bytes binary = read_entire_file("../data/result_10000000.b");
    u64 num = binary.size/(sizeof(f64))-1; // Binary contains each individual result and then the averaged result at the end.
    f64* f64_buffer = (f64*)binary.buffer;
    HaversineResult reference_result = { f64_buffer, f64_buffer[num], num };

    // Compare calculated results with reference results and output
    compare_results(result, reference_result);

    return 0;
}