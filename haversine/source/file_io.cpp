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

// To be used on the file reading thread.
unsigned long thread_file_read(void* bytes_chunks_param)
{
    BytesChunks* chunks = (BytesChunks*)bytes_chunks_param;

    // Initialisation

    FILE* file = fopen(chunks->file_path, "rb");
    fseek(file, 0, SEEK_END);
    chunks->file_size = ftell(file);
    u64 file_cursor = 0;
    Assert(chunks->file_size > 2*chunks->chunk_size); // Maximum chunk size half the size of the file.

    b32 next0 = TRUE;
    b32 read_complete = FALSE;
    u8* buffer = 0;
    volatile b32* complete_marker = 0;
    while (!read_complete) {
        // Until we have read all chunks of file, we wait for the next buffer to become available and perform the read when it is.
        if (next0) {
            while (!chunks->buffer0_complete) Sleep(10);
            buffer = chunks->buffer0;
            complete_marker = &chunks->buffer0_complete;
        } else {
            while (!chunks->buffer1_complete) Sleep(10);
            buffer = chunks->buffer0;
            complete_marker = &chunks->buffer0_complete;
        }
        next0 = !next0;

        {
            TIME_BANDWIDTH("fread", chunks->chunk_size);
            Assert(complete_marker && *complete_marker && buffer);
            
            fseek(file, file_cursor, SEEK_SET);
            u64 read_size = chunks->chunk_size;
            if (file_cursor + read_size > chunks->file_size) { 
                read_size = chunks->file_size - file_cursor;
                // Set buffer to zero for final read.
                memset(buffer, 0, chunks->chunk_size + chunks->extra_size);
            }
            fread(buffer + chunks->extra_size, 1, read_size, file);
            MemoryBarrier();
            *complete_marker = FALSE;
            MemoryBarrier();
            if (file_cursor + read_size > chunks->file_size) chunks->no_more_chunks = TRUE;
            MemoryBarrier();
            file_cursor += read_size;
        }
    }
    fclose(file);
    return 0;
}

// Chunk size is the size of the chunks to read the file in.
// Extra size is the size of the extra buffer space in each buffer.
// File reads will fill the latter part of the buffer, after the extra spaxe.
// This space can be used when needing to read stuff that straddles chunks.
FileReadData begin_file_read_chunks(const char* file_path, u64 chunk_size_bytes, u64 extra_size_bytes)
{
    Assert(chunk_size_bytes > 1024); // Minimum size 1kb

    BytesChunks* chunks = (BytesChunks*)malloc(sizeof(BytesChunks));
    chunks->file_path = file_path;
    chunks->chunk_size = chunk_size_bytes;
    chunks->extra_size = extra_size_bytes;
    chunks->buffer0 = (u8*)malloc(chunks->chunk_size + chunks->extra_size);
    chunks->buffer1 = (u8*)malloc(chunks->chunk_size + chunks->extra_size);
    chunks->buffer0_complete = TRUE;
    chunks->buffer1_complete = TRUE;

    FileReadData data = {};
    data.buffer0 = chunks->buffer0;
    data.buffer1 = chunks->buffer1;
    data.buffer0_complete = &chunks->buffer0_complete;
    data.buffer1_complete = &chunks->buffer1_complete;
    data.no_more_chunks = &chunks->no_more_chunks;
    data.file_size = &chunks->file_size;
    data.chunk_size = chunk_size_bytes;
    data.extra_size = extra_size_bytes;
    CreateThread(0, 0, thread_file_read, (void*)&chunks, 0, &data.thread_id);
    return data;
}