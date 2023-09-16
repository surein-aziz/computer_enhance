#include <stdio.h>

#include "windows.h"
#include "helpful.h"
#include "data.h"

// Single unit compilation
#include "disassemble.cpp"

//TODO(surein): clean up string handling ensuring no memory leaks
//TODO(surein): complete disassembler sufficiently to complete listing 42

#define MAX_LABELS 1000
#define MAX_LINES 10000

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

Bytes append_chars(Bytes bytes, const char* chars)
{
    u8* old_buffer = bytes.buffer;
    s32 old_size = bytes.size;
    s32 length = (s32)strlen(chars);
    bytes.size = bytes.size + length;
    bytes.buffer = (u8*)malloc(bytes.size);
    if (old_buffer) {
        memcpy(bytes.buffer, old_buffer, old_size);
        free(old_buffer);
    }
    memcpy(bytes.buffer+old_size, (u8*)chars, length);

    return bytes;
}

char* get_label_line(s32 label_num) {
    char* label_str = (char*)malloc(100*sizeof(char*));
    sprintf(label_str, "label_%d:\n", label_num);
    return label_str;
}

s32 get_next_label_index(s32 total_bytes, s32 label_count, s32* label_indices) {
    int next = -1;
    int min_dist = INT_MAX;
    for (int i = 0; i < label_count; ++i) {
        if (label_indices[i] > total_bytes && (label_indices[i]-total_bytes) < min_dist) {
            next = i;
            min_dist = label_indices[i]-total_bytes;
        }
    }
    return next;
}

// disassemble ASM, optionally execute it, and output
void process(Bytes asm_file, const char* output_path, bool exec)
{
    Bytes output;
    s32 current = 0;

    // Save number of bytes output for each instruction
    s32* instruction_bytes = (s32*)malloc(asm_file.size*sizeof(s32));
    s32 instruction_count = 0;

    // Save index of each label found.
    s32* label_indices = (s32*)malloc(MAX_LABELS*sizeof(s32));
    s32 label_count = 0;

    char** instruction_lines = (char**)malloc(MAX_LINES*sizeof(char*));
    int line_count = 0;
    
    while (current < asm_file.size) {
        s32 prev = current;

        instruction_lines[line_count++] = decode_instruction(asm_file, &current, label_indices, &label_count);
        Assert(line_count < MAX_LINES);
        if (current == prev) {
            Assert(!"Decode failed.");
            break;
        }
        instruction_bytes[instruction_count++] = current-prev;
    }

    // Output to bytes ready to be output to file.
    const char* preamble = "bits 16\n";
    output = append_chars(output, preamble);

    s32 labels_inserted = 0; // All labels should be inserted. Debugging check.
    s32 total_bytes = 0; // Keep track number of input bytes corresponding to instructions output so far.
    s32 next_label_index = get_next_label_index(total_bytes, label_count, label_indices);
    for (int i = 0; i < instruction_count; ++i) {
        output = append_chars(output, instruction_lines[i]);
        free(instruction_lines[i]);

        total_bytes += instruction_bytes[i];
        // Insert label if required
        if (next_label_index >= 0 && total_bytes == label_indices[next_label_index]) {
            char* label_line = get_label_line(next_label_index+1);
            output = append_chars(output, label_line);
            free(label_line);
            labels_inserted++;
            next_label_index = get_next_label_index(total_bytes, label_count, label_indices);
        }
    }
    Assert(labels_inserted == label_count);


    free(instruction_lines);
    free(instruction_bytes);
    free(label_indices);

    write_entire_file(output, output_path);
}

s32 APIENTRY WinMain(HINSTANCE instance,
                     HINSTANCE prev_instance,
                     LPTSTR cmd_line,
                     int show)
{
    Bytes bytes = read_entire_file("../data/listing_0043_immediate_movs");
    process(bytes, "../output/listing_0043_immediate_movs.asm", true);
    free(bytes.buffer);
    bytes = {};

    bytes = read_entire_file("../data/listing_0044_register_movs");
    process(bytes, "../output/listing_0044_register_movs.asm", true);
    free(bytes.buffer);
    bytes = {};

    bytes = read_entire_file("../data/listing_0045_challenge_register_movs");
    process(bytes, "../output/listing_0045_challenge_register_movs.asm", true);
    free(bytes.buffer);
    bytes = {};

    return 0;
}