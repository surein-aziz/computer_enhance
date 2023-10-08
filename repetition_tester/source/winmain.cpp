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
static s64 test_page_faults = 0;
static s64 begin_end_offset = 0;
static u64 byte_count = 0;
static u64 elapsed_since_min = 0;

static u64 total_tests = 0;
static u64 total_elapsed = 0;
static u64 total_page_faults = 0;

// Note that we track min and max elapsed time to determine things like test runtime.
// We also record other metrics for the min and max elapsed runtime.
static u64 max_elapsed = 0;
static u64 min_elapsed = (u64)-1;
static u64 max_page_faults = 0;
static u64 min_page_faults = 0;

struct DecomposedVirtualAddress
{
    u16 pml4_index = 0;
    u16 directory_ptr_index = 0;
    u16 directory_index = 0;
    u16 table_index = 0;
    u32 offset = 0;
};

static void print_dva(DecomposedVirtualAddress address)
{
    printf("|%3u|%3u|%3u|%3u|%10u|", address.pml4_index, address.directory_ptr_index, address.directory_index, address.table_index, address.offset);
}

static void print_dva_as_line(char const* label, DecomposedVirtualAddress address)
{
    printf("%s", label);
    print_dva(address);
    printf("\n");
}

static DecomposedVirtualAddress decompose_pointer_4k(void* ptr)
{
    DecomposedVirtualAddress result = {};
    
    u64 address = (u64)ptr;
    result.pml4_index = ((address >> 39) & 0x1ff);
    result.directory_ptr_index = ((address >> 30) & 0x1ff);
    result.directory_index = ((address >> 21) & 0x1ff);
    result.table_index = ((address >> 12) & 0x1ff);
    result.offset = ((address >> 0) & 0xfff);
    
    return result;
}

void init(const char* label, u64 target)
{
    printf("--- %s ---\n", label);
    target_byte_count = target;
    if (!cpu_freq) cpu_freq = guess_cpu_freq(100);

    test_elapsed = 0;
    test_page_faults = 0;
    begin_end_offset = 0;
    byte_count = 0;
    elapsed_since_min = 0;

    total_tests = 0;
    total_elapsed = 0;
    total_page_faults = 0;
    max_elapsed = 0;
    min_elapsed = (u64)-1;
    max_page_faults = 0;
    min_page_faults = 0;
}

static void print_time(const char* label, f64 elapsed, f64 page_faults, u64 count)
{
    Assert(cpu_freq > 0);

    f64 seconds = elapsed/(f64)cpu_freq;
    printf("%s: %.0f", label, elapsed);
    printf(" (%fms)", 1000.0*seconds);
    
    f64 gigabyte = (1024.0 * 1024.0 * 1024.0);
    f64 bandwidth = count / (gigabyte*seconds);
    printf(" %fgb/s", bandwidth);

    if (page_faults > 0.0) {
        f64 kilobyte = 1024.0;
        printf(" PF: %0.4f (%0.4fk/fault)", page_faults, count / (kilobyte*page_faults));
    }
}

bool testing()
{
    Assert(byte_count == target_byte_count);
    Assert(begin_end_offset == 0);

    total_tests++;
    total_elapsed += test_elapsed;
    elapsed_since_min += test_elapsed;

    total_page_faults += test_page_faults;

    if ((u64)test_elapsed > max_elapsed) {
        max_elapsed = test_elapsed;
        max_page_faults = test_page_faults;
    }
    if ((u64)test_elapsed < min_elapsed) {
        min_elapsed = test_elapsed;
        min_page_faults = test_page_faults;
        elapsed_since_min = 0;
    }
    test_elapsed = 0;
    test_page_faults = 0;
    begin_end_offset = 0;
    byte_count = 0;

    if (1000.0*(elapsed_since_min / (f64)cpu_freq) > wait_ms) {
        printf("\r                                                                                  \r");
        print_time("Min", (f64)min_elapsed, (f64)min_page_faults, target_byte_count);
        printf("\n");
        print_time("Max", (f64)max_elapsed, (f64)max_page_faults, target_byte_count);
        printf("\n");
        print_time("Avg", total_elapsed / (f64)total_tests, total_page_faults / (f64)total_tests, target_byte_count);
        printf("\n\n");
        return false;
    } else {
        printf("\r                                                                                  \r");
        print_time("Min", min_elapsed, (f64)min_page_faults, target_byte_count);
    }
    return true;
}

void begin()
{
    test_elapsed -= read_cpu_timer();
    test_page_faults -= read_os_page_fault_count();
    begin_end_offset++;
}

void end()
{
    test_elapsed += read_cpu_timer();
    test_page_faults += read_os_page_fault_count();
    begin_end_offset--;
}

void count(u64 bytes)
{
    byte_count += bytes;
}

void test_write_page_faults()
{
    u64 page_size = 4096;
    u64 page_count = 4096;
    u64 total_size = page_size*page_count;

    for (u64 i = 1; i <= page_count; ++i) {
        u64 touch_size = page_size*i;
        u8* data = (u8*)VirtualAlloc(0, total_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        u64 faults_start = read_os_page_fault_count();
        for (u64 j = 0; j < touch_size; ++j) {
            data[j] = (u8)j;
        }
        u64 faults = read_os_page_fault_count() - faults_start;

        printf("%3llu, %3llu, %3llu, %3lld,", page_count, i, faults, faults - i);
        void* end_ptr = data+touch_size-1;
        print_dva(decompose_pointer_4k(end_ptr));
        printf("\n");
        VirtualFree(data, 0, MEM_RELEASE);
    }
}

void test_backwards_write_page_faults()
{
    u64 page_size = 4096;
    u64 page_count = 4096;
    u64 total_size = page_size*page_count;

    for (u64 i = 1; i <= page_count; ++i) {
        u64 touch_size = page_size*i;
        u8* data = (u8*)VirtualAlloc(0, total_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        u64 faults_start = read_os_page_fault_count();
        for (s64 j = total_size-1; j >= (s64)(total_size - touch_size); --j) {
            data[j] = (u8)j;
        }
        u64 faults = read_os_page_fault_count() - faults_start;

        printf("%llu, %llu, %llu, %lld\n", page_count, i, faults, faults - i);
        VirtualFree(data, 0, MEM_RELEASE);
    }
}

void test_write_bytes(const char* label, Bytes preallocated_bytes, bool use_preallocated)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;
        if (!use_preallocated) {
            buffer = (u8*)malloc(preallocated_bytes.size);
        }
        u8* out = buffer;

        begin();
        for (u64 i = 0; i < preallocated_bytes.size; ++i) {
            out[i] = (u8)i;
        }
        end();
        
        count(preallocated_bytes.size);

        if (!use_preallocated) {
            free(buffer);
        }
    } while (testing());
}

void test_write_bytes_backward(const char* label, Bytes preallocated_bytes, bool use_preallocated)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;
        if (!use_preallocated) {
            buffer = (u8*)malloc(preallocated_bytes.size);
        }
        u8* out = buffer;

        begin();
        for (s64 i = preallocated_bytes.size-1; i >= 0; --i) {
            out[i] = (u8)i;
        }
        end();
        
        count(preallocated_bytes.size);

        if (!use_preallocated) {
            free(buffer);
        }
    } while (testing());
}

void test_fread(const char* file_name, const char* label, Bytes preallocated_bytes, bool use_preallocated)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;
        if (!use_preallocated) {
            buffer = (u8*)malloc(preallocated_bytes.size);
        }
        u8* out = buffer;

        FILE* file = fopen(file_name, "rb");
        Assert(file);
        
        begin();
        fread(out, 1, preallocated_bytes.size, file);
        end();
        
        count(preallocated_bytes.size);
        fclose(file);

        if (!use_preallocated) {
            free(buffer);
        }
    } while (testing());
}

void test_read(const char* file_name, const char* label, Bytes preallocated_bytes, bool use_preallocated)
{
    init(label, preallocated_bytes.size);
    do {
        s32 file = _open(file_name, _O_BINARY|_O_RDONLY);
        Assert(file != -1);

        u8* buffer = preallocated_bytes.buffer;
        if (!use_preallocated) {
            buffer = (u8*)malloc(preallocated_bytes.size);
        }
        u8* out = buffer;
        u64 remaining = preallocated_bytes.size;
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

        if (!use_preallocated) {
            free(buffer);
        }    
    } while (testing());
}

void test_read_file(const char* file_name, const char* label, Bytes preallocated_bytes, bool use_preallocated)
{
    init(label, preallocated_bytes.size);
    do {
        HANDLE file = CreateFileA(file_name, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        Assert(file != INVALID_HANDLE_VALUE);

        u8* buffer = preallocated_bytes.buffer;
        if (!use_preallocated) {
            buffer = (u8*)malloc(preallocated_bytes.size);
        }
        u8* out = buffer;
        u64 remaining = preallocated_bytes.size;
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

        if (!use_preallocated) {
            free(buffer);
        }
    } while (testing());
}

s32 main(int arg_count, char** args)
{
    initialize_metrics();

    const char* file_name = "../data/data_10000000.json";
    Bytes bytes;
    FILE* file = fopen(file_name, "rb");
    fseek(file, 0, SEEK_END);
    bytes.size = ftell(file);
    bytes.buffer = (u8*)malloc(bytes.size);
    fclose(file);

    test_write_page_faults();
    //test_backwards_write_page_faults();
    //test_write_bytes("write bytes preallocated", bytes, true);
    //test_write_bytes("write bytes with allocation", bytes, false);
    //test_write_bytes_backward("write bytes backward preallocated", bytes, true);
    //test_write_bytes_backward("write bytes backward with allocation", bytes, false);
    //test_fread(file_name, "fread preallocated", bytes, true);
    //test_fread(file_name, "fread with allocation", bytes, false);
    //test_read(file_name, "_read preallocated", bytes, true);
    //test_read(file_name, "_read with allocation", bytes, false);
    //test_read_file(file_name, "ReadFile preallocated", bytes, true);
    //test_read_file(file_name, "ReadFile with allocation", bytes, false);
    return 0;
}