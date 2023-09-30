// Should call time_program_start() at start of program and time_program_end_and_print() at end. Then use TIME_FUNCTION / TimeScope in between as required.

#define TIME_FUNCTION TimeScope time_scope(__FUNCTION__, __COUNTER__+1)

struct TimeInfo;

static u64 program_start = 0;
static arr<TimeInfo> time_infos;
static u64 active_index = 0;

struct TimeInfo {
	const char* name = 0;
	u64 count = 0;
	u64 child_time = 0;
	u64 cpu_time = 0;
};

struct TimeScope {
	TimeScope(const char* a_name, s32 counter) {
		Assert(program_start > 0);
		if (time_infos.size <= counter) {
			time_infos.resize(time_infos.size + 100);
		}
		index = counter;
		name = a_name;
		if (active_index) {
			parent_index = active_index;
		}
		active_index = index;
		start = read_cpu_timer();
	}

	~TimeScope() {
		Assert(program_start > 0);
		u64 end = read_cpu_timer();
		time_infos[index].name = name;
		time_infos[index].count++;
		time_infos[index].cpu_time += end - start;
		if (parent_index) {
			time_infos[parent_index].child_time += end - start;
		}
		active_index = parent_index;
	}

	u64 index = 0;
	u64 parent_index = 0;
	const char* name = 0;
	u64 start = 0;
};

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

	for (u64 i = 1; i < time_infos.size; ++i) {
		if (!time_infos[i].name) continue;
		f64 pct = (time_infos[i].cpu_time / (f64)program_time)*100.0;
		u64 exclusive_time = time_infos[i].cpu_time - time_infos[i].child_time;
		f64 exclusive_pct = (exclusive_time / (f64)program_time)*100.0;
		printf("%s[%llu] %llu (%.4g%%), exclusive %llu (%.4g%%)\n", time_infos[i].name, time_infos[i].count, time_infos[i].cpu_time, pct, exclusive_time, exclusive_pct);
	}
}