#include "windows.h"
#include "../../common/helpful.h"

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
    s32 read_size = bytes.size;
    bytes.buffer = (u8*)malloc(bytes.size);
    
    {
        TIME_BANDWIDTH("fread", read_size);

        fseek(file, 0, SEEK_SET);
        fread(bytes.buffer, 1, read_size, file);
        fclose(file);
    }

    return bytes;
}

u64 queue_chunk_read(uintptr file_ptr, u64 file_size, u64 start_cursor, u64 chunk_size, u64 extra_size, u8* buffer, b32* complete_marker)
{
    TIME_BANDWIDTH("fread", chunk_size);
    Assert(*complete_marker);
    
    FILE* file = (FILE*)file_ptr;
    
    fseek(file, start_cursor, SEEK_SET);
    u64 read_size = chunk_size;
    if (start_cursor + read_size > file_size) { 
        read_size = file_size - start_cursor;
        // Set buffer to zero for final read.
        memset(buffer, 0, chunk_size + extra_size);
    }
    fread(buffer + extra_size, 1, read_size, file);
    *complete_marker = FALSE;

    return start_cursor + read_size;
}

// Chunk size is the size of the chunks to read the file in.
// Extra size is the size of the extra buffer space in each buffer.
// File reads will fill the latter part of the buffer, after the extra spaxe.
// This space can be used when needing to read stuff that straddles chunks.
BytesChunks begin_file_read_chunks(const char* file_path, u64 chunk_size_bytes, u64 extra_size_bytes)
{
    Assert(chunk_size_bytes > 1024); // Minimum size 1kb

    BytesChunks chunks = {};
    chunks.chunk_size = chunk_size_bytes;
    chunks.extra_size = extra_size_bytes;

    FILE* file = fopen(file_path, "rb");
    fseek(file, 0, SEEK_END);
    chunks.file_size = ftell(file);
    Assert(chunks.file_size > 2*chunks.chunk_size); // Maximum chunk size half the size of the file.
    chunks.file = (uintptr)file;

    chunks.buffer0 = (u8*)malloc(chunks.chunk_size + chunks.extra_size);
    chunks.buffer1 = (u8*)malloc(chunks.chunk_size + chunks.extra_size);
    chunks.buffer0_complete = TRUE;
    chunks.buffer1_complete = TRUE;
    
    //TODO start_file_thread();
    chunks.file_cursor = queue_chunk_read(chunks.file, chunks.file_size, chunks.file_cursor, chunks.chunk_size, chunks.extra_size, chunks.buffer0, &chunks.buffer0_complete);
    chunks.file_cursor = queue_chunk_read(chunks.file, chunks.file_size, chunks.file_cursor, chunks.chunk_size, chunks.extra_size, chunks.buffer1, &chunks.buffer1_complete);
    return chunks;
}