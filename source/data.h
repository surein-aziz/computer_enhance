#define MAX_LABELS 1000
#define MAX_LINES 10000

struct Bytes {
    u8* buffer = 0; // malloced buffer
    s32 size = 0;
};