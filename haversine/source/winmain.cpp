#include <stdio.h>
#include <time.h>

#include "windows.h"
#include "helpful.h"

// Single unit compilation
#include "reference_haversine.cpp"

struct Bytes {
    u8* buffer = 0; // malloced buffer
    s32 size = 0;
};

void write_entire_file(Bytes bytes, const char* file_path)
{
    FILE* file = fopen(file_path, "wb");
    Assert(file);
    u64 written = fwrite(bytes.buffer, 1, bytes.size, file);
    Assert(written == bytes.size);
    fclose(file);
}

Bytes read_entire_file(const char* file_path)
{
    Bytes bytes;
    
    FILE* file = fopen(file_path, "rb");
    fseek(file, 0, SEEK_END);
    
    bytes.size = ftell(file);
    bytes.buffer = (u8*)malloc(bytes.size);
    
    fseek(file, 0, SEEK_SET);
    fread(bytes.buffer, 1, bytes.size, file);
    fclose(file);
    
    return bytes;
}

s32 APIENTRY WinMain(HINSTANCE instance,
                     HINSTANCE prev_instance,
                     LPTSTR cmd_line,
                     int show)
{
    Bytes json = read_entire_file("../data/data_10000000.json");

    Bytes binary = read_entire_file("../data/result_10000000.b");
    


    return 0;
}