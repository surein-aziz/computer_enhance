
struct BytesChunks {
    volatile b32 buffer0_complete[16] = {};
    volatile b32 buffer1_complete[16] = {};
    volatile b32 no_more_chunks[16] = {};
    volatile u64 file_size = 0;

    const char* file_path = 0;
    u64 chunk_size = 0;
    u64 extra_size = 0;

    u8* buffer0 = 0;
    u8* buffer1 = 0;
};

struct FileReadData {
    unsigned long thread_id = 0;
    u8* buffer0 = 0;
    u8* buffer1 = 0;
    volatile b32* buffer0_complete = 0;
    volatile b32* buffer1_complete = 0;
    volatile b32* no_more_chunks = 0;
    volatile u64* file_size = 0;

    u64 chunk_size = 0;
    u64 extra_size = 0;
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