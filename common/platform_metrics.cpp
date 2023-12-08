#include <intrin.h>
#include <psapi.h>

#pragma comment (lib, "bcrypt.lib")

struct Bytes {
    u8* buffer = 0; // malloced buffer
    s32 size = 0;
};

struct WindowsMetrics
{
	bool initialized = false;
	HANDLE handle = 0;
};
static WindowsMetrics global_metrics;

static void initialize_metrics()
{
	Assert(!global_metrics.initialized);
	global_metrics.initialized = true;
	global_metrics.handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId());
}

static u64 read_os_page_fault_count()
{
	Assert(global_metrics.initialized);
    PROCESS_MEMORY_COUNTERS_EX memory_counters = {};
    memory_counters.cb = sizeof(memory_counters);
    GetProcessMemoryInfo(global_metrics.handle, (PROCESS_MEMORY_COUNTERS*)&memory_counters, sizeof(memory_counters));
    return memory_counters.PageFaultCount;
}

static u64 get_os_timer_freq()
{
	LARGE_INTEGER Freq;
	QueryPerformanceFrequency(&Freq);
	return Freq.QuadPart;
}

static u64 read_os_timer()
{
	LARGE_INTEGER Value;
	QueryPerformanceCounter(&Value);
	return Value.QuadPart;
}

/* NOTE(casey): This does not need to be "inline", it could just be "static"
   because compilers will inline it anyway. But compilers will warn about 
   static functions that aren't used. So "inline" is just the simplest way 
   to tell them to stop complaining about that. */
inline u64 read_cpu_timer()
{
	// NOTE(casey): If you were on ARM, you would need to replace __rdtsc
	// with one of their performance counter read instructions, depending
	// on which ones are available on your platform.
	
	return __rdtsc();
}

// Input milliseconds to wait to generate the estimate. Longer waits will be *slightly* more accurate. 100 is fine.
inline u64 guess_cpu_freq(u64 milliseconds)
{
	Assert(milliseconds > 0);
	u64 os_freq = get_os_timer_freq();
	u64 cpu_start = read_cpu_timer();
	u64 os_start = read_os_timer();
	u64 os_end = 0;
	u64 os_elapsed = 0;
	u64 os_wait_time = os_freq * milliseconds / 1000;
	while(os_elapsed < os_wait_time)
	{
		os_end = read_os_timer();
		os_elapsed = os_end - os_start;
	}
	
	u64 cpu_end = read_cpu_timer();
	u64 cpu_elapsed = cpu_end - cpu_start;
	Assert(os_elapsed > 0);
	return os_freq * cpu_elapsed / os_elapsed;
}

static u64 get_max_os_random_count()
{
    return 0xffffffff;
}

static b32 read_os_random_bytes(u64 count, void* dest)
{
    b32 res = false;
    if(count < get_max_os_random_count())
    {
        res = (BCryptGenRandom(0, (BYTE *)dest, (u32)count, BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0);
    }
    
    return res;
}

inline void fill_with_random_bytes(Bytes bytes)
{
    u64 max_rand_count = get_max_os_random_count();
    u64 at_offset = 0;
    while(at_offset < bytes.size)
    {
        u64 read_count = bytes.size - at_offset;
        if(read_count > max_rand_count)
        {
            read_count = max_rand_count;
        }
        
        read_os_random_bytes(read_count, bytes.buffer + at_offset);
        at_offset += read_count;
    }
}
