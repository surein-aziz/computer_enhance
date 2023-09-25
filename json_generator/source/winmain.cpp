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

enum class GenType {
    UNIFORM,
    CLUSTER
};

void write_entire_file(Bytes bytes, const char* file_path)
{
    FILE* file = fopen(file_path, "wb");
    Assert(file);
    u64 written = fwrite(bytes.buffer, 1, bytes.size, file);
    Assert(written == bytes.size);
    fclose(file);
}

r64 rand_x() {
    static const r64 x_mult = 180.0/RAND_MAX;
    return (rand()*x_mult)-90.0;
}

r64 rand_y() {
    static const r64 y_mult = 360.0/RAND_MAX;
    return (rand()*y_mult)-180.0;
}

r64 rand_x_cluster(f64 cluster) {
    static const r64 x_mult = 18.0/RAND_MAX;
    return cluster + (rand()*x_mult)-9.0;
}

r64 rand_y_cluster(f64 cluster) {
    static const r64 y_mult = 36.0/RAND_MAX;
    return cluster+(rand()*y_mult)-18.0;
}


r64 generate(u64 num_pairs, u32 seed, const char* data_output, const char* binary_output, GenType type) {
    Bytes binary;
    binary.size = (num_pairs+1)*sizeof(f64);
    binary.buffer = (u8*)malloc(binary.size);
    f64* results = (f64*)binary.buffer;

    FILE* data_file = fopen(data_output, "wb");
    fprintf(data_file, "{\"pairs\":[\n");
    srand(seed);
    f64 sum = 0.0;
    r64 radius = 6372.8; // Arbitrary. Use an estimate of the earth's radius.
    f64 x0_cluster = 0.0;
    f64 y0_cluster = 0.0;
    f64 x1_cluster = 0.0;
    f64 y1_cluster = 0.0;
    for (u64 i = 0; i < num_pairs; ++i) {
        r64 x0, y0, x1, y1;
        if (type == GenType::CLUSTER) {
            if ((i % (num_pairs/5)) == 0) {
                x0_cluster = rand_x()*0.9;
                y0_cluster = rand_y()*0.9;
                x1_cluster = rand_x()*0.9;
                y1_cluster = rand_y()*0.9;
            }
            x0 = rand_x_cluster(x0_cluster);
            y0 = rand_y_cluster(y0_cluster);
            x1 = rand_x_cluster(x1_cluster);
            y1 = rand_y_cluster(y1_cluster);
        } else {
            Assert(type == GenType::UNIFORM);
            x0 = rand_x();
            y0 = rand_y();
            x1 = rand_x();
            y1 = rand_y();
        }

        results[i] = ReferenceHaversine(x0, y0, x1, y1, radius);
        sum += results[i];
        fprintf(data_file, "{\"x0\":%.17g, \"y0\":%.17g, \"x1\":%.17g, \"y1\":%.17g}", x0, y0, x1, y1);
        if (i+1 < num_pairs) fprintf(data_file, ",\n");
    }
    fprintf(data_file, "]}");
    fclose(data_file);
    
    f64 avg = sum / num_pairs;
    results[num_pairs] = avg; // Save the average at the end.
    write_entire_file(binary, binary_output);
    free(binary.buffer);

    return avg;
}

s32 APIENTRY WinMain(HINSTANCE instance,
                     HINSTANCE prev_instance,
                     LPTSTR cmd_line,
                     int show)
{
    u64 num_pairs = 10000;
    u32 seed = time(0);
    GenType type = GenType::CLUSTER;
    f64 avg = generate(num_pairs, seed, "../output/data.json", "../output/result.b", type);
    if (type == GenType::UNIFORM) {
        printf("Output %d uniformly random pairs. The seed was %d. Average result: %.17g\n", (s32)num_pairs, (s32)seed, avg);
    } else if (type == GenType::CLUSTER) {
        printf("Output %d random pairs in clusters. The seed was %d. Average result: %.17g\n", (s32)num_pairs, (s32)seed, avg);
    } else {
        Assert(!"Unknown type.");
    }
    return 0;
}