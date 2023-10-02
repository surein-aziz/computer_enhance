// Should call time_program_start() at start of program and time_program_end_and_print() at end. Then use TIME_FUNCTION / TimeScope in between as required.
static u64 program_start = 0;

#ifndef PROFILER

#define TIME_BANDWIDTH
#define TIME_SCOPE
#define TIME_FUNCTION
#define ASSERT_ENOUGH_TIMEINFOS
#define TOTAL_TIMEINFOS

#else

#define TIME_BANDWIDTH(Name, ByteCount) TimeScope time_scope(Name, __COUNTER__+1, ByteCount)
#define TIME_FUNCTION TIME_BANDWIDTH(__FUNCTION__, 0)
#define ASSERT_ENOUGH_TIMEINFOS static_assert(__COUNTER__ < TOTAL_TIMEINFOS)
#define TOTAL_TIMEINFOS 4096

struct TimeInfo {
	const char* name = 0;
	u64 count = 0;
	u64 inclusive_time = 0;
	u64 exclusive_time = 0;
	u64 byte_count = 0;
};
static u64 active_index = 0;

static TimeInfo time_infos[TOTAL_TIMEINFOS];

struct TimeScope {
	TimeScope(const char* name_, s32 counter, u64 byte_count) {
		Assert(program_start > 0);
		index = counter;
		name = name_;
		if (active_index) {
			parent_index = active_index;
		}
		inclusive_start = time_infos[index].inclusive_time;
		time_infos[index].byte_count += byte_count;
		active_index = index;
		start = read_cpu_timer();
	}

	~TimeScope() {
		Assert(program_start > 0);
		u64 elapsed = read_cpu_timer() - start;
		time_infos[index].name = name;
		time_infos[index].count++;
		// Add elapsed to inclusive time when starting this block, overwriting any recursive time.
		time_infos[index].inclusive_time = inclusive_start + elapsed;
		// Add elapsed to exclusive time -- this needs to include recursive time and subtract child time.
		time_infos[index].exclusive_time += elapsed;
		if (parent_index) {
			time_infos[parent_index].exclusive_time -= elapsed;
		}
		active_index = parent_index;
	}

	u64 index = 0;
	u64 parent_index = 0;
	const char* name = 0;
	u64 start = 0;
	u64 inclusive_start = 0;
};

#endif

void time_program_start()
{
	program_start = read_cpu_timer();
}

void time_program_end_and_print()
{
	u64 program_time = read_cpu_timer() - program_start;
	program_start = 0;

    u64 cpu_freq = guess_cpu_freq(100);
    f64 total_ms = (program_time/(f64)cpu_freq)*1000.0;
    printf("\nTotal time: %.8gms (CPU freq %llu)\n", total_ms, cpu_freq);

#ifdef PROFILER
	for (u64 i = 1; i < TOTAL_TIMEINFOS; ++i) {
		if (!time_infos[i].name) continue;
		f64 pct = (time_infos[i].inclusive_time / (f64)program_time)*100.0;
		f64 exclusive_pct = (time_infos[i].exclusive_time / (f64)program_time)*100.0;
		printf("%s[%llu] %llu (%.2f%%), exclusive %llu (%.2f%%)", time_infos[i].name, time_infos[i].count, time_infos[i].inclusive_time, pct, time_infos[i].exclusive_time, exclusive_pct);
		if (time_infos[i].byte_count > 0) {
			f64 megabyte = 1024.0*1024.0;
			f64 gigabyte = megabyte*1024.0;

			f64 seconds = time_infos[i].inclusive_time / (f64)cpu_freq;
			f64 gigabytes_per_second = time_infos[i].byte_count / (gigabyte*seconds);
			f64 megabytes = time_infos[i].byte_count / megabyte;
			printf(" %.2fmb at %.2fgb/s", megabytes, gigabytes_per_second);
		}
		printf("\n");
	}
#endif
}