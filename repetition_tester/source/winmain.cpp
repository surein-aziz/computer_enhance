#include <stdio.h>
#include <fcntl.h>
#include <io.h>

#include "windows.h"
#include "../../common/helpful.h"

// Single compilation unit
#include "../../common/platform_metrics.cpp"

struct Bytes {
    u8* buffer = 0; // malloced buffer
    s32 size = 0;
};

static const f64 wait_ms = 10000;

static u64 cpu_freq = 0;
static u64 target_byte_count = 0;

static s64 test_elapsed = 0;
static s64 begin_end_offset = 0;
static u64 byte_count = 0;
static u64 elapsed_since_min = 0;

static u64 total_tests = 0;
static u64 total_elapsed = 0;
static u64 max_elapsed = 0;
static u64 min_elapsed = (u64)-1;

void init(const char* label, u64 target)
{
    printf("--- %s ---\n", label);
    target_byte_count = target;
    if (!cpu_freq) cpu_freq = guess_cpu_freq(100);

    test_elapsed = 0;
    begin_end_offset = 0;
    byte_count = 0;
    elapsed_since_min = 0;

    total_tests = 0;
    total_elapsed = 0;
    max_elapsed = 0;
    min_elapsed = (u64)-1;
}

static void print_time(const char* label, f64 elapsed, u64 count)
{
    Assert(cpu_freq > 0);

    f64 seconds = elapsed/(f64)cpu_freq;
    printf("%s: %.0f", label, elapsed);
    printf(" (%fms)", 1000.0*seconds);

    f64 gigabyte = (1024.0 * 1024.0 * 1024.0);
    f64 bandwidth = count / (gigabyte*seconds);
    printf(" %fgb/s", bandwidth);
    printf("\n");
}

bool testing()
{
    Assert(byte_count == target_byte_count);
    Assert(begin_end_offset == 0);

    total_tests++;
    total_elapsed += test_elapsed;
    elapsed_since_min += test_elapsed;

    if ((u64)test_elapsed > max_elapsed) {
        max_elapsed = test_elapsed;
    }
    if ((u64)test_elapsed < min_elapsed) {
        min_elapsed = test_elapsed;
        elapsed_since_min = 0;
    }

    if (1000.0*(elapsed_since_min / (f64)cpu_freq) > wait_ms) {
        printf("\r");
        print_time("Min", (f64)min_elapsed, target_byte_count);
        print_time("Max", (f64)max_elapsed, target_byte_count);
        print_time("Avg", total_elapsed / (f64)total_tests, target_byte_count);
        printf("\n");
        return false;
    } else {
        printf("\r");
        print_time("Min", min_elapsed, target_byte_count);
    }
    return true;
}

void begin()
{
    test_elapsed -= read_cpu_timer();
    begin_end_offset++;
}

void end()
{
    test_elapsed += read_cpu_timer();
    begin_end_offset--;
}

void count(u64 bytes)
{
    byte_count += bytes;
}

void test_fread(const char* file_name, Bytes bytes)
{
    init("fread", bytes.size);
    while (testing()) {
        FILE* file = fopen(file_name, "rb");
        Assert(file);
        
        begin();
        fread(bytes.buffer, 1, bytes.size, file);
        end();
        
        count(bytes.size);
        fclose(file);
    }
}

void test_read(const char* file_name, Bytes bytes)
{
    init("_read", bytes.size);
    while (testing()) {
        s32 file = _open(file_name, _O_BINARY|_O_RDONLY);
        Assert(file == -1);

        u8* out = bytes.buffer;
        u64 remaining = bytes.size;
        while (remaining > 0) {
            u32 read_size = INT_MAX;
            if ((u64)read_size > remaining) read_size = (u32)remaining;

            begin();
            int res = _read(file, out, read_size);
            end();
            Assert(res == (s32)read_size);
            count(read_size);
            
            remaining -= read_size;
            out += read_size;
        }
        
        _close(file);
    }
}

void test_read_file(const char* file_name, Bytes bytes)
{
    init("ReadFile", bytes.size);
    while (testing()) {
        HANDLE file = CreateFileA(file_name, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        Assert(file != INVALID_HANDLE_VALUE);

        u8* out = bytes.buffer;
        u64 remaining = bytes.size;
        while (remaining > 0) {
            u32 read_size = (u32)-1;
            if ((u64)read_size > remaining) read_size = (u32)remaining;

            DWORD bytes_read = 0;
            begin();
            BOOL res = ReadFile(file, out, read_size, &bytes_read, 0);
            end();
            Assert(res && (bytes_read == read_size));
            count(read_size);
            
            remaining -= read_size;
            out += read_size;
        }
        
        CloseHandle(file);
    }
}

s32 main(int arg_count, char** args)
{
    const char* file_name = "../data/data_10000000.json";
    Bytes bytes;
    FILE* file = fopen(file_name, "rb");
    fseek(file, 0, SEEK_END);
    bytes.size = ftell(file);
    bytes.buffer = (u8*)malloc(bytes.size);
    fclose(file);

    test_fread(file_name, bytes);
    test_read(file_name, bytes);
    test_read_file(file_name, bytes);
    return 0;
}