
struct BytesChunks {
    uintptr file = 0;
    u64 file_size = 0;
    u64 file_cursor = 0;
    u64 chunk_size = 0;
    u64 extra_size = 0;

    u8* buffer0 = 0;
    u8* buffer1 = 0;
    b32 buffer0_complete = 0;
    b32 buffer1_complete = 0;
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