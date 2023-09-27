#include <stdio.h>
#include <time.h>

#include "windows.h"
#include "helpful.h"
#include "data.h"

// Single unit compilation
#include "reference_haversine.cpp"
#include "reference_parser.cpp"
#include "platform_metrics.cpp"

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

Bytes read_entire_file_null_term(const char* file_path)
{
    Bytes bytes;
    
    FILE* file = fopen(file_path, "rb");
    fseek(file, 0, SEEK_END);
    
    bytes.size = ftell(file)+1;
    bytes.buffer = (u8*)malloc(bytes.size);
    
    fseek(file, 0, SEEK_SET);
    fread(bytes.buffer, 1, bytes.size-1, file);
    fclose(file);
    bytes.buffer[bytes.size-1] = 0;

    return bytes;
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

s32 main(int arg_count, char** args)
{
    u64 start_time = read_cpu_timer();

    // Parse json
    Bytes json = read_entire_file_null_term("../data/data_10000000.json");

    u64 read_time = read_cpu_timer();

    HaversineData data = parse_haversine_json(json);

    u64 parse_time = read_cpu_timer();

    // Calculate haversines
    HaversineResult result = calculate_haversine(data);

    u64 haversine_time = read_cpu_timer();

    // Parse reference results
    Bytes binary = read_entire_file("../data/result_10000000.b");
    u64 num = binary.size/(sizeof(f64))-1; // Binary contains each individual result and then the averaged result at the end.
    f64* f64_buffer = (f64*)binary.buffer;
    HaversineResult reference_result = { f64_buffer, f64_buffer[num], num };

    u64 end_time = read_cpu_timer();

    // Compare calculated results with reference results and output
    compare_results(result, reference_result);

    // Output timing information
    u64 cpu_freq = guess_cpu_freq(100);
    u64 total_time = end_time - start_time;
    f64 total_ms = (total_time/(f64)cpu_freq)*1000.0;
    printf("\nTotal time: %.8gms (CPU freq %llu)\n", total_ms, cpu_freq);
    u64 read_time_inc = read_time - start_time;
    f64 read_pct = (read_time_inc/(f64)total_time)*100.0;
    printf("Read JSON: %llu (%.4g%%)\n", read_time_inc, read_pct);
    u64 parse_time_inc = parse_time - read_time;
    f64 parse_pct = (parse_time_inc/(f64)total_time)*100.0;
    printf("Parse JSON: %llu (%.4g%%)\n", parse_time_inc, parse_pct);
    u64 haversine_time_inc = haversine_time - parse_time;
    f64 haversine_pct = (haversine_time_inc/(f64)total_time)*100.0;
    printf("Calculate Haversines: %llu (%.4g%%)\n", haversine_time_inc, haversine_pct);
    u64 reference_time_inc = end_time - haversine_time;
    f64 reference_pct = (reference_time_inc/(f64)total_time)*100.0;
    printf("Read Reference Data: %llu (%.4g%%)\n", reference_time_inc, reference_pct);

    return 0;
}