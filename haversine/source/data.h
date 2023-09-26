struct Bytes {
    u8* buffer = 0; // malloced buffer
    s32 size = 0;
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