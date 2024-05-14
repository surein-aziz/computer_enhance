#include <stdio.h>
#include <fcntl.h>
#include <io.h>

#include "windows.h"
#include "../../common/helpful.h"

// Single compilation unit
#include "../../common/platform_metrics.cpp"

extern "C" void MOVAllBytesASM(u64 count, u8* data);
extern "C" void NOPAllBytesASM(u64 count);
extern "C" void CMPAllBytesASM(u64 count);
extern "C" void DECAllBytesASM(u64 count);
#pragma comment (lib, "listing_0132_nop_loop")

extern "C" void NOP3x1AllBytes(u64 count);
extern "C" void NOP1x3AllBytes(u64 count);
extern "C" void NOP1x9AllBytes(u64 count);
#pragma comment (lib, "listing_0134_multinop_loops")

extern "C" void ConditionalNOP(u64 count, u8* data);
#pragma comment (lib, "listing_0136_conditional_nop_loops")

extern "C" void Read_x1(u64 count, u8* data);
extern "C" void Read_x2(u64 count, u8* data);
extern "C" void Read_x3(u64 count, u8* data);
extern "C" void Read_x4(u64 count, u8* data);
#pragma comment (lib, "listing_0144_read_unroll")

extern "C" void Write_x1(u64 count, u8* data);
extern "C" void Write_x2(u64 count, u8* data);
extern "C" void Write_x3(u64 count, u8* data);
extern "C" void Write_x4(u64 count, u8* data);
#pragma comment (lib, "write_ports")

extern "C" void Read_4x2(u64 count, u8* data);
extern "C" void Read_4x3(u64 count, u8* data);
extern "C" void Read_4x4(u64 count, u8* data);
extern "C" void Read_8x2(u64 count, u8* data);
extern "C" void Read_8x3(u64 count, u8* data);
extern "C" void Read_8x4(u64 count, u8* data);
extern "C" void Read_16x2(u64 count, u8* data);
extern "C" void Read_16x3(u64 count, u8* data);
extern "C" void Read_16x4(u64 count, u8* data);
extern "C" void Read_32x2(u64 count, u8* data);
extern "C" void Read_32x3(u64 count, u8* data);
extern "C" void Read_32x4(u64 count, u8* data);
#pragma comment (lib, "listing_0150_read_widths")

extern "C" void Read_1024mb(u8* data);
extern "C" void Read_256mb(u8* data);
extern "C" void Read_64mb(u8* data);
extern "C" void Read_16mb(u8* data);
extern "C" void Read_4mb(u8* data);
extern "C" void Read_1024kb(u8* data);
extern "C" void Read_256kb(u8* data);
extern "C" void Read_64kb(u8* data);
extern "C" void Read_16kb(u8* data);
extern "C" void Read_4kb(u8* data);
extern "C" void Read_1kb(u8* data);
#pragma comment (lib, "cache_test")

extern "C" void Read_Granular(u64 kbs, u8* data, u64 reps);
#pragma comment (lib, "granular_cache_test")

extern "C" void Read_Fixed(u64 fixed_count, u8* data, u64 reads);
#pragma comment (lib, "fixed_test")

extern "C" void Write_Temporal(u8* read_data, u8* write_data);
extern "C" void Write_Non_Temporal(u8* read_data, u8* write_data);
#pragma comment (lib, "write_temporality_test")

extern "C" void Random_Math(u8* random_data, u8* math_data);
extern "C" void Random_Math_Prefetch(u8* random_data, u8* math_data);
#pragma comment (lib, "prefetch_math_test")

static const f64 wait_ms = 5000;

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

static void test_write_bytes(const char* label, Bytes preallocated_bytes, bool use_preallocated)
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

static void test_MOVAllBytes(const char* label, Bytes preallocated_bytes, bool use_preallocated)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;
        if (!use_preallocated) {
            buffer = (u8*)malloc(preallocated_bytes.size);
        }
        u8* out = buffer;

        begin();
        MOVAllBytesASM(preallocated_bytes.size, out);
        end();
        
        count(preallocated_bytes.size);

        if (!use_preallocated) {
            free(buffer);
        }
    } while (testing());
}

static void test_Read_x1(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;

        begin();
        Read_x1(preallocated_bytes.size, buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_Read_x2(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;

        begin();
        Read_x2(preallocated_bytes.size, buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_Read_x3(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;

        begin();
        Read_x3(preallocated_bytes.size, buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}


static void test_Read_x4(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;

        begin();
        Read_x4(preallocated_bytes.size, buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_Read_4x2(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;

        begin();
        Read_4x2(preallocated_bytes.size, buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_Read_4x3(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;

        begin();
        Read_4x3(preallocated_bytes.size, buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_Read_4x4(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;

        begin();
        Read_4x4(preallocated_bytes.size, buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_Read_8x2(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;

        begin();
        Read_8x2(preallocated_bytes.size, buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_Read_8x3(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;

        begin();
        Read_8x3(preallocated_bytes.size, buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_Read_8x4(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;

        begin();
        Read_8x4(preallocated_bytes.size, buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_Read_16x2(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;

        begin();
        Read_16x2(preallocated_bytes.size, buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_Read_16x3(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;

        begin();
        Read_16x3(preallocated_bytes.size, buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_Read_16x4(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;

        begin();
        Read_16x4(preallocated_bytes.size, buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_Read_32x2(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;

        begin();
        Read_32x2(preallocated_bytes.size, buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_Read_32x3(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;

        begin();
        Read_32x3(preallocated_bytes.size, buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_Read_32x4(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;

        begin();
        Read_32x4(preallocated_bytes.size, buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_Write_x1(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;

        begin();
        Write_x1(preallocated_bytes.size, buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_Write_x2(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;

        begin();
        Write_x2(preallocated_bytes.size, buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_Write_x3(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;

        begin();
        Write_x3(preallocated_bytes.size, buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}


static void test_Write_x4(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;

        begin();
        Write_x4(preallocated_bytes.size, buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_Read_Granular_Offset(const char* label, Bytes preallocated_bytes_aligned, u64 bytes, u64 offset)
{
    u64 reps = 0x40000000 / bytes;
    u64 total = bytes * reps;
    u8* buffer = preallocated_bytes_aligned.buffer;
    Assert(preallocated_bytes_aligned.size > 0x3FFFFFFF);
    DecomposedVirtualAddress dva = decompose_pointer_4k(buffer);
    Assert((dva.offset % 64) == 0);
    init(label, total);
    do {
        begin();
        Read_Granular(bytes, buffer + offset, reps);
        end();
        count(total);
    } while (testing());
}

static void test_Write_Temporal(const char* label, Bytes bytes_read, Bytes bytes_write)
{
    Assert(bytes_read.size > 0x3FFF); // Read bytes at least 16kb
    Assert(bytes_write.size > 0x3FFFFFFF); // Write bytes at least 1gb

    u8* read_buffer = bytes_read.buffer;
    DecomposedVirtualAddress read_dva = decompose_pointer_4k(read_buffer);
    Assert((read_dva.offset % 64) == 0);
    u8* write_buffer = bytes_write.buffer;
    DecomposedVirtualAddress write_dva = decompose_pointer_4k(write_buffer);
    Assert((write_dva.offset % 64) == 0);

    u64 total = 1 << 28;
    init(label, total);
    do {
        begin();
        Write_Temporal(read_buffer, write_buffer);
        end();
        
        count(total);
    } while (testing());
}

static void test_Write_Non_Temporal(const char* label, Bytes bytes_read, Bytes bytes_write)
{
    Assert(bytes_read.size > 0x3FFF); // Read bytes at least 16kb
    Assert(bytes_write.size > 0x3FFFFFFF); // Write bytes at least 1gb

    u8* read_buffer = bytes_read.buffer;
    DecomposedVirtualAddress read_dva = decompose_pointer_4k(read_buffer);
    Assert((read_dva.offset % 64) == 0);
    u8* write_buffer = bytes_write.buffer;
    DecomposedVirtualAddress write_dva = decompose_pointer_4k(write_buffer);
    Assert((write_dva.offset % 64) == 0);

    u64 total = 1 << 28;
    init(label, total);
    do {
        begin();
        Write_Non_Temporal(read_buffer, write_buffer);
        end();
        
        count(total);
    } while (testing());
}

static void test_Random_Math(const char* label, Bytes bytes_random, Bytes bytes_buffer)
{
    Assert(bytes_read.size >= 0x10000); // Read bytes at least 64kb
    Assert(bytes_write.size >= 0x40000000); // Write bytes at least 1gb

    u8* random_buffer = bytes_random.buffer;
    DecomposedVirtualAddress random_dva = decompose_pointer_4k(random_buffer);
    Assert((random_dva.offset % 64) == 0);
    u8* math_buffer = bytes_buffer.buffer;
    DecomposedVirtualAddress math_dva = decompose_pointer_4k(math_buffer);
    Assert((math_dva.offset % 64) == 0);

    u64 total = 8*1024*64;
    init(label, total);
    do {
        begin();
        Random_Math(random_buffer, math_buffer);
        end();
        
        count(total);
    } while (testing());
}

static void test_Random_Math_Prefetch(const char* label, Bytes bytes_random, Bytes bytes_buffer)
{
    Assert(bytes_read.size >= 0x10000); // Read bytes at least 64kb
    Assert(bytes_write.size >= 0x40000000); // Write bytes at least 1gb

    u8* random_buffer = bytes_random.buffer;
    DecomposedVirtualAddress random_dva = decompose_pointer_4k(random_buffer);
    Assert((random_dva.offset % 64) == 0);
    u8* math_buffer = bytes_buffer.buffer;
    DecomposedVirtualAddress math_dva = decompose_pointer_4k(math_buffer);
    Assert((math_dva.offset % 64) == 0);

    u64 total = 8*1024*64;
    init(label, total);
    do {
        begin();
        Random_Math_Prefetch(random_buffer, math_buffer);
        end();
        
        count(total);
    } while (testing());
}

static void test_Read_Fixed(const char* label_in, Bytes bytes, u64 fixed_count, u64 read_count)
{
    // "index bits" = bits we are fixing, "tag bits" = higher bits, "n-way set associative" = uses mini caches each storing n cache lines
    // Lowest 6 bits are in the same cache line.
    Assert(6 + read_count + fixed_count <= 29);
    Assert(bytes.size > 0x3FFFFFFF); // Buffer must be at least 1gb, 2^30

    u8* buffer = bytes.buffer;
    DecomposedVirtualAddress dva = decompose_pointer_4k(buffer);
    Assert((dva.offset % 64) == 0);

    u64 read_size = (u64)1 << read_count;
    f64 kbs = read_size / (1024.0);
    char label[200];
    sprintf(label, "%s, read size %0.4f kbs", label_in, kbs);

    u64 total = ((u64)1 << (read_count + 12))*64;
    init(label, total);
    do {
        begin();
        Read_Fixed(fixed_count, buffer, read_size);
        end();
        
        count(total);
    } while (testing());
}

static void test_Read_1024mb(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;
        Assert(preallocated_bytes.size > 0x3FFFFFFF);

        begin();
        Read_1024mb(buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}


static void test_Read_256mb(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;
        Assert(preallocated_bytes.size > 0x3FFFFFFF);

        begin();
        Read_256mb(buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_Read_64mb(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;
        Assert(preallocated_bytes.size > 0x3FFFFFFF);

        begin();
        Read_64mb(buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_Read_16mb(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;
        Assert(preallocated_bytes.size > 0x3FFFFFFF);

        begin();
        Read_16mb(buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_Read_4mb(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;
        Assert(preallocated_bytes.size > 0x3FFFFFFF);

        begin();
        Read_4mb(buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_Read_1024kb(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;
        Assert(preallocated_bytes.size > 0x3FFFFFFF);

        begin();
        Read_1024kb(buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_Read_256kb(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;
        Assert(preallocated_bytes.size > 0x3FFFFFFF);

        begin();
        Read_256kb(buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_Read_64kb(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;
        Assert(preallocated_bytes.size > 0x3FFFFFFF);

        begin();
        Read_64kb(buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_Read_16kb(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;
        Assert(preallocated_bytes.size > 0x3FFFFFFF);

        begin();
        Read_16kb(buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_Read_4kb(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;
        Assert(preallocated_bytes.size > 0x3FFFFFFF);

        begin();
        Read_4kb(buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_Read_1kb(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;
        Assert(preallocated_bytes.size > 0x3FFFFFFF);

        begin();
        Read_1kb(buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_ConditionalNOP(const char* label, Bytes preallocated_bytes)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;

        begin();
        ConditionalNOP(preallocated_bytes.size, buffer);
        end();
        
        count(preallocated_bytes.size);
    } while (testing());
}

static void test_NOPAllBytes(const char* label, Bytes preallocated_bytes, bool use_preallocated)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;
        if (!use_preallocated) {
            buffer = (u8*)malloc(preallocated_bytes.size);
        }

        begin();
        NOPAllBytesASM(preallocated_bytes.size);
        end();
        
        count(preallocated_bytes.size);

        if (!use_preallocated) {
            free(buffer);
        }
    } while (testing());
}

static void test_CMPAllBytes(const char* label, Bytes preallocated_bytes, bool use_preallocated)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;
        if (!use_preallocated) {
            buffer = (u8*)malloc(preallocated_bytes.size);
        }

        begin();
        CMPAllBytesASM(preallocated_bytes.size);
        end();
        
        count(preallocated_bytes.size);

        if (!use_preallocated) {
            free(buffer);
        }
    } while (testing());
}

static void test_DECAllBytes(const char* label, Bytes preallocated_bytes, bool use_preallocated)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;
        if (!use_preallocated) {
            buffer = (u8*)malloc(preallocated_bytes.size);
        }

        begin();
        DECAllBytesASM(preallocated_bytes.size);
        end();
        
        count(preallocated_bytes.size);

        if (!use_preallocated) {
            free(buffer);
        }
    } while (testing());
}

static void test_NOP3x1AllBytes(const char* label, Bytes preallocated_bytes, bool use_preallocated)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;
        if (!use_preallocated) {
            buffer = (u8*)malloc(preallocated_bytes.size);
        }

        begin();
        NOP3x1AllBytes(preallocated_bytes.size);
        end();
        
        count(preallocated_bytes.size);

        if (!use_preallocated) {
            free(buffer);
        }
    } while (testing());
}

static void test_NOP1x3AllBytes(const char* label, Bytes preallocated_bytes, bool use_preallocated)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;
        if (!use_preallocated) {
            buffer = (u8*)malloc(preallocated_bytes.size);
        }

        begin();
        NOP1x3AllBytes(preallocated_bytes.size);
        end();
        
        count(preallocated_bytes.size);

        if (!use_preallocated) {
            free(buffer);
        }
    } while (testing());
}

static void test_NOP1x9AllBytes(const char* label, Bytes preallocated_bytes, bool use_preallocated)
{
    init(label, preallocated_bytes.size);
    do {
        u8* buffer = preallocated_bytes.buffer;
        if (!use_preallocated) {
            buffer = (u8*)malloc(preallocated_bytes.size);
        }

        begin();
        NOP1x9AllBytes(preallocated_bytes.size);
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

    // 64kb
    Bytes bytes_random;
    bytes_random.size = 0x10000;
    bytes_random.buffer = (u8*)_mm_malloc(bytes_random.size, 64);

    // 1gb
    Bytes bytes_buffer;
    bytes_buffer.size = 0x40000000;
    bytes_buffer.buffer = (u8*)_mm_malloc(bytes_buffer.size, 64);

    // Fill bytes_random with random offsets into the 1gb buffer.
    for (u64 i = 0; i < 8*1024; ++i) {
        u64 offset = rand() % bytes_buffer.size;
        memcpy(bytes_random.buffer + 8*i, &offset, 8);
    }

    // Fill bytes_buffer with ascending count.
    for (u64 i = 0; i < bytes_buffer.size/8; ++i) {
        memcpy(bytes_buffer.buffer + 8*i, &i, 8);
    }

    // Test prefetching.
    // Do maths on 64 bytes from a random point in a 1gb buffer, 8*1024 times.
    test_Random_Math("Math at random place in buffer without prefetch.", bytes_random, bytes_buffer);
    test_Random_Math_Prefetch("Math at random place in buffer with prefetch.", bytes_random, bytes_buffer);

    /*
    // Test non-temporal stores.
    // Copy 16kb of memory 16384 times into 256mb.
    test_Write_Temporal("16384 copies of 16kb, temporal stores", bytes_read, bytes_write);
    test_Write_Non_Temporal("16384 copies of 16kb, non temporal stores", bytes_read, bytes_write);

    // Test performance reading cache lines at positions chosen to hit a particular performance bottleneck.
    // Keep the bottom 6 bits the same -- all of these are in the same 64-byte cache line.
    // Test read performance keeping a differing number of the next lowest bits of their address fixed -- 0 through 10
    test_Read_Fixed("No bits fixed", bytes, 0, 13);
    test_Read_Fixed("1 bit fixed", bytes, 1, 13);
    test_Read_Fixed("2 bits fixed", bytes, 2, 13);
    test_Read_Fixed("3 bits fixed", bytes, 3, 13);
    test_Read_Fixed("4 bits fixed", bytes, 4, 13);
    test_Read_Fixed("5 bits fixed", bytes, 5, 13);
    test_Read_Fixed("6 bits fixed", bytes, 6, 13);
    test_Read_Fixed("7 bits fixed", bytes, 7, 13);
    test_Read_Fixed("8 bits fixed", bytes, 8, 13);
    test_Read_Fixed("9 bits fixed", bytes, 9, 13);
    test_Read_Fixed("10 bits fixed", bytes, 10, 13);

    test_Read_Fixed("No bits fixed", bytes, 0, 8);
    test_Read_Fixed("1 bit fixed", bytes, 1, 8);
    test_Read_Fixed("2 bits fixed", bytes, 2, 8);
    test_Read_Fixed("3 bits fixed", bytes, 3, 8);
    test_Read_Fixed("4 bits fixed", bytes, 4, 8);
    test_Read_Fixed("5 bits fixed", bytes, 5, 8);
    test_Read_Fixed("6 bits fixed", bytes, 6, 8);
    test_Read_Fixed("7 bits fixed", bytes, 7, 8);
    test_Read_Fixed("8 bits fixed", bytes, 8, 8);
    test_Read_Fixed("9 bits fixed", bytes, 9, 8);
    test_Read_Fixed("10 bits fixed", bytes, 10, 8);
    test_Read_Fixed("11 bits fixed", bytes, 11, 8);
    test_Read_Fixed("12 bits fixed", bytes, 12, 8);
    test_Read_Fixed("13 bits fixed", bytes, 13, 8);
    test_Read_Fixed("14 bits fixed", bytes, 14, 8);
    test_Read_Fixed("15 bits fixed", bytes, 15, 8);

    // Goes to main memory on my machine
    test_Read_Granular_Offset("1024mb read aligned", bytes, 1024*1024*1024, 0);
    test_Read_Granular_Offset("1024mb read off-by-1", bytes, 1024*1024*1024, 1);
    test_Read_Granular_Offset("1024mb read off-by-15", bytes, 1024*1024*1024, 15);
    test_Read_Granular_Offset("1024mb read off-by-32", bytes, 1024*1024*1024, 32);

    // Fits in L3 on my machine I think, though things are fuzzier here
    test_Read_Granular_Offset("4mb read aligned", bytes, 4*1024*1024, 0);
    test_Read_Granular_Offset("4mb read off-by-1", bytes, 4*1024*1024, 1);
    test_Read_Granular_Offset("4mb read off-by-15", bytes, 4*1024*1024, 15);
    test_Read_Granular_Offset("4mb read off-by-32", bytes, 4*1024*1024, 32);

    // Fits in L2 on my machine I think, though things are fuzzier here
    test_Read_Granular_Offset("256kb read aligned", bytes, 256*1024, 0);
    test_Read_Granular_Offset("256kb read off-by-1", bytes, 256*1024, 1);
    test_Read_Granular_Offset("256kb read off-by-15", bytes, 256*1024, 15);
    test_Read_Granular_Offset("256kb read off-by-32", bytes, 256*1024, 32);

    // Fits in L1 on my machine
    test_Read_Granular_Offset("1kb read aligned", bytes, 1*1024, 0);
    test_Read_Granular_Offset("1kb read off-by-1", bytes, 1*1024, 1);
    test_Read_Granular_Offset("1kb read off-by-15", bytes, 1*1024, 15);
    test_Read_Granular_Offset("1kb read off-by-32", bytes, 1*1024, 32);

    /*
    test_Read_Granular("1024mb read", bytes, 1024*1024*1024);
    test_Read_Granular("256mb read", bytes, 256*1024*1024);
    test_Read_Granular("128mb read", bytes, 128*1024*1024);
    test_Read_Granular("64mb read", bytes, 64*1024*1024);
    test_Read_Granular("48mb read", bytes, 48*1024*1024);
    test_Read_Granular("32mb read", bytes, 32*1024*1024);
    test_Read_Granular("24mb read", bytes, 24*1024*1024);
    test_Read_Granular("16mb read", bytes, 16*1024*1024);
    test_Read_Granular("4mb read", bytes, 4*1024*1024);
    test_Read_Granular("3mb read", bytes, 3*1024*1024);
    test_Read_Granular("2mb read", bytes, 2*1024*1024);
    test_Read_Granular("1536kb read", bytes, 1536*1024);
    test_Read_Granular("1024kb read", bytes, 1024*1024);
    test_Read_Granular("256kb read", bytes, 256*1024);
    test_Read_Granular("64kb read", bytes, 64*1024);
    test_Read_Granular("48kb read", bytes, 48*1024);
    test_Read_Granular("32kb read", bytes, 32*1024);
    test_Read_Granular("24kb read", bytes, 24*1024);
    test_Read_Granular("16kb read", bytes, 16*1024);
    test_Read_Granular("4kb read", bytes, 4*1024);
    test_Read_Granular("1kb read", bytes, 1*1024);

    test_Read_1024mb("1024mb read", bytes);
    test_Read_256mb("256mb read", bytes);
    test_Read_64mb("64mb read", bytes);
    test_Read_16mb("16mb read", bytes);
    test_Read_4mb("4mb read", bytes);
    test_Read_1024kb("1024kb read", bytes);
    test_Read_256kb("256kb read", bytes);
    test_Read_64kb("64kb read", bytes);
    test_Read_16kb("16kb read", bytes);
    test_Read_4kb("4kb read", bytes);
    test_Read_1kb("1kb read", bytes);

    const char* file_name = "../data/data_10000000.json";
    Bytes bytes;
    FILE* file = fopen(file_name, "rb");
    fseek(file, 0, SEEK_END);
    bytes.size = ftell(file);
    bytes.buffer = (u8*)malloc(bytes.size);
    fclose(file);

    test_Read_4x2("4-byte read 2 times per loop", bytes);
    test_Read_4x3("4-byte read 3 times per loop", bytes);
    test_Read_4x4("4-byte read 4 times per loop", bytes);
    test_Read_8x2("8-byte read 2 times per loop", bytes);
    test_Read_8x3("8-byte read 3 times per loop", bytes);
    test_Read_8x4("8-byte read 4 times per loop", bytes);
    test_Read_16x2("16-byte read 2 times per loop", bytes);
    test_Read_16x3("16-byte read 3 times per loop", bytes);
    test_Read_16x4("16-byte read 4 times per loop", bytes);
    test_Read_32x2("32-byte read 2 times per loop", bytes);
    test_Read_32x3("32-byte read 3 times per loop", bytes);
    test_Read_32x4("32-byte read 4 times per loop", bytes);

    test_Write_x1("Write once per loop", bytes);
    test_Write_x2("Write twice per loop", bytes);
    test_Write_x3("Write three times per loop", bytes);
    test_Write_x4("Write four times per loop", bytes);

    test_Read_x1("Read once per loop", bytes);
    test_Read_x2("Read twice per loop", bytes);
    test_Read_x3("Read three times per loop", bytes);
    test_Read_x4("Read four times per loop", bytes);


    test_write_bytes("write bytes (preallocated)", bytes, true);
    test_MOVAllBytes("write bytes via ASM (preallocated)", bytes, true);
    for (u64 i = 0; i < bytes.size; ++i) {
        bytes.buffer[i] = 0;
    }
    test_ConditionalNOP("write bytes via ASM, branch predict no jumps", bytes);
    for (u64 i = 0; i < bytes.size; ++i) {
        bytes.buffer[i] = 1;
    }
    test_ConditionalNOP("write bytes via ASM, branch predict all jumps", bytes);
    for (u64 i = 0; i < bytes.size/2; ++i) {
        bytes.buffer[i] = 0;
        bytes.buffer[i+1] = 1;
    }
    test_ConditionalNOP("write bytes via ASM, branch predict jump every other", bytes);
    for (u64 i = 0; i < bytes.size/3; ++i) {
        bytes.buffer[i] = 0;
        bytes.buffer[i+1] = 0;
        bytes.buffer[i+2] = 1;
    }
    test_ConditionalNOP("write bytes via ASM, branch predict jump every 3rd", bytes);
    for (u64 i = 0; i < bytes.size/4; ++i) {
        bytes.buffer[i] = 0;
        bytes.buffer[i+1] = 0;
        bytes.buffer[i+2] = 0;
        bytes.buffer[i+3] = 1;
    }
    test_ConditionalNOP("write bytes via ASM, branch predict jump every 4th", bytes);
    for (u64 i = 0; i < bytes.size; ++i) {
        bytes.buffer[i] = rand();
    }
    test_ConditionalNOP("write bytes via ASM, branch predict jump rand()", bytes);
    fill_with_random_bytes(bytes);
    test_ConditionalNOP("write bytes via ASM, branch predict jump bcrypt rand", bytes);

    test_NOP3x1AllBytes("empty loop of same size via ASM (mov -> 3 byte nop, memory not touched)", bytes, true);
    test_NOP1x3AllBytes("empty loop of same size via ASM (mov -> 3 1 byte nops, memory not touched)", bytes, true);
    test_NOP1x9AllBytes("empty loop of same size via ASM (mov -> 9 1 byte nops, memory not touched)", bytes, true);
    test_write_page_faults();
    test_backwards_write_page_faults();
    test_write_bytes("write bytes preallocated", bytes, true);
    test_write_bytes("write bytes with allocation", bytes, false);
    test_write_bytes_backward("write bytes backward preallocated", bytes, true);
    test_write_bytes_backward("write bytes backward with allocation", bytes, false);
    test_fread(file_name, "fread preallocated", bytes, true);
    test_fread(file_name, "fread with allocation", bytes, false);
    test_read(file_name, "_read preallocated", bytes, true);
    test_read(file_name, "_read with allocation", bytes, false);
    test_read_file(file_name, "ReadFile preallocated", bytes, true);
    test_read_file(file_name, "ReadFile with allocation", bytes, false);
    */
    return 0;
}