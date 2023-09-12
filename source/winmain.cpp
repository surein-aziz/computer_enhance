#include "windows.h"
#include "helpful.h"

#include <stdio.h>

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

const char* decode_register(u8 reg, bool w) 
{
    if (reg == 0b00000000) {
        return w ? "AX" : "AL";
    } else if (reg == 0b00000001) {
        return w ? "CX" : "CL";
    } else if (reg == 0b00000010) {
        return w ? "DX" : "DL";
    } else if (reg == 0b00000011) {
        return w ? "BX" : "BL";
    } else if (reg == 0b00000100) {
        return w ? "SP" : "AH";
    } else if (reg == 0b00000101) {
        return w ? "BP" : "CH";
    } else if (reg == 0b00000110) {
        return w ? "SI" : "DH";
    } else if (reg == 0b00000111) {
        return w ? "DI" : "BH";
    }
    Assert(!"Invalid reg code");
    return "";
}

// Returns malloced char
char* decode_instruction(Bytes asm_file, int* current) {

    if ((asm_file.buffer[*current] & 0b11111100) == 0b10001000) {
        // Register/memory to/from register

        // We initially read from two bytes for this instruction
        u8 byte1 = asm_file.buffer[*current];
        bool d = !!(byte1 & 0b00000010);
        bool w = !!(byte1 & 0b00000001);

        u8 byte2 = asm_file.buffer[*current+1];
        u8 mod = byte2 & 0b11000000;
        if (mod != 0b11000000) {
            // This instruction is not register-to-register. Unsupported for now.
            Assert(FALSE);
            return "";
        }
        u8 reg = byte2 & 0b00111000;
        u8 rm = byte2 & 0b00000111;

        u8 dest = reg >> 3;
        u8 source = rm;
        if (!d) {
            u8 flip = dest;
            dest = source;
            source = flip;
        }


        const char* mov_str = "mov ";
        const char* dest_str = decode_register(dest, w);
        const char* mid_str = ", ";
        const char* source_str = decode_register(source, w);
        const char* end_str = "\n";
        char* instruction_str = (char*)malloc(sizeof(char)*(strlen(mov_str)+strlen(mid_str)+strlen(end_str)+strlen(dest_str)+strlen(source_str)+1));
        char* inst_str_cur = instruction_str;
        memcpy(inst_str_cur, mov_str, sizeof(char)*strlen(mov_str));
        inst_str_cur += strlen(mov_str);
        memcpy(inst_str_cur, dest_str, sizeof(char)*strlen(dest_str));
        inst_str_cur += strlen(dest_str);
        memcpy(inst_str_cur, mid_str, sizeof(char)*strlen(mid_str));
        inst_str_cur += strlen(mid_str);
        memcpy(inst_str_cur, source_str, sizeof(char)*strlen(source_str));
        inst_str_cur += strlen(source_str);
        memcpy(inst_str_cur, end_str, sizeof(char)*strlen(end_str));
        inst_str_cur += strlen(end_str);
        inst_str_cur[0] = 0;

        (*current) += 2;

        return instruction_str;
    } 

    // Not supported yet
    Assert(FALSE);
    return "";
}

// disassemble ASM and output
void disassemble(Bytes asm_file, const char* output_path)
{
    //TODO: complete outputting disassembly
    Bytes output;
    s32 current = 0;

    const char* preamble = "bits 16\n";
    output = append_chars(output, preamble);
    
    while (current < asm_file.size) {
        s32 check_progress = current;
        char* instruction_str = decode_instruction(asm_file, &current);
        if (current == check_progress) {
            Assert(!"Decode failed.");
            break;
        }

        output = append_chars(output, instruction_str);
        free(instruction_str);
    }

    
    write_entire_file(output, output_path);
}

s32 APIENTRY WinMain(HINSTANCE instance,
                     HINSTANCE prev_instance,
                     LPTSTR cmd_line,
                     int show)
{
    Bytes bytes37 = read_entire_file("../data/listing_0037_single_register_mov");
    disassemble(bytes37, "../output/listing_0037_out.asm");

    Bytes bytes38 = read_entire_file("../data/listing_0038_many_register_mov");
    disassemble(bytes38, "../output/listing_0038_out.asm");

    return 0;
}