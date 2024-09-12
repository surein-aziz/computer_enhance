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

void read_chunk(BytesChunks* chunks, b32 read0)
{
    TIME_BANDWIDTH("fread", chunks->chunk_size);
    Assert(read0 ? chunks->buffer0_complete : chunks->buffer1_complete);
    
    FILE* file = (FILE*)chunks->file;
    u8** buffer_ptr = read0 ? &chunks->buffer0 : &chunks->buffer1;
    b32* complete_ptr = read0? &chunks->buffer0_complete : &chunks->buffer1_complete;
    
    fseek(file, chunks->file_cursor, SEEK_SET);
    u64 read_size = chunks->chunk_size;
    if (chunks->file_cursor + read_size > chunks->file_size) { 
        read_size = chunks->file_size - chunks->file_cursor;
        // Set buffer to zero for final read.
        memset(*buffer_ptr, 0, chunks->chunk_size+chunks->extra_size);
    }
    fread((*buffer_ptr)+chunks->extra_size, 1, read_size, file);
    chunks->file_cursor += read_size;
    *complete_ptr = FALSE;
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
    
    read_chunk(&chunks, TRUE);
    read_chunk(&chunks, FALSE);
    return chunks;
}