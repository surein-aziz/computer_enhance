#include <intrin.h>

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
