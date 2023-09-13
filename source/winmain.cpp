#include "windows.h"
#include "helpful.h"

#include <stdio.h>

//TODO(surein): clean up string handling ensuring no memory leaks

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

void decode_memory(u8 rm, char* str, s16 disp) {
    if (rm == 0b00000000) {
        strcpy(str, "[BX + SI");
    } else if (rm == 0b00000001) {
        strcpy(str, "[BX + DI");
    } else if (rm == 0b00000010) {
        strcpy(str, "[BP + SI");
    } else if (rm == 0b00000011) {
        strcpy(str, "[BP + DI");
    } else if (rm == 0b00000100) {
        strcpy(str, "[SI");
    } else if (rm == 0b00000101) {
        strcpy(str, "[DI");
    } else if (rm == 0b00000110) {
        strcpy(str, "[BP");
    } else if (rm == 0b00000111) {
        strcpy(str, "[BX");
    } else {
        Assert(!"Invalid rm code");
        return;
    }

    if (disp > 0) {
        char* cur = str + strlen(str);
        sprintf(cur, " + %d", disp);
    } else if (disp < 0) {
        s32 big_disp = disp;
        char* cur = str + strlen(str);
        sprintf(cur, " - %d", -big_disp);
    }

    strcat(str, "]");
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
        u8 byte1 = asm_file.buffer[(*current)++];
        bool d = !!(byte1 & 0b00000010);
        bool w = !!(byte1 & 0b00000001);

        u8 byte2 = asm_file.buffer[(*current)++];
        u8 mod = byte2 & 0b11000000;
        u8 reg = byte2 & 0b00111000;
        u8 rm = byte2 & 0b00000111;

        u8 register_bits = reg >> 3;
        const char* dest_str = 0;
        const char* source_str = 0;

        if (mod == 0b11000000) {
            // Register mode (no displacement)

            if (d) {
                dest_str = decode_register(register_bits, w);
                source_str = decode_register(rm, w);
            } else {
                source_str = decode_register(register_bits, w);
                dest_str = decode_register(rm, w);
            }

        } else if (mod == 0b00000000) {
            // Memory mode (no displacement except for direct address)

            char memory_str[100]; // 100 should be large enough
            if (rm == 0b00000110) {
                // Use direct address. Read 2 more bytes for this.
                u8 low = asm_file.buffer[(*current)++];
                u8 high = asm_file.buffer[(*current)++];
                u16 address = low + (high << 8);
                strcpy(memory_str, "[");
                char* num_start = memory_str + strlen(memory_str);
                sprintf(num_start, "%d", address);
                strcat(memory_str, "]");
            } else {
                decode_memory(rm, memory_str, 0);
            }

            if (d) {
                dest_str = decode_register(register_bits, w);
                source_str = memory_str;
            } else {
                source_str = decode_register(register_bits, w);
                dest_str = memory_str;
            }

        } else if (mod == 0b01000000) {
            // Memory mode with 8-bit displacement

            char memory_str[100]; // 100 should be large enough
            u8 low = asm_file.buffer[(*current)++];
            decode_memory(rm, memory_str, (s8)low);
            if (d) {
                dest_str = decode_register(register_bits, w);
                source_str = memory_str;
            } else {
                source_str = decode_register(register_bits, w);
                dest_str = memory_str;
            }

        } else {
            // Memory mode with 16-bit displacement
            Assert(mod == 0b10000000);

            char memory_str[100]; // 100 should be large enough
            u8 low = asm_file.buffer[(*current)++];
            u8 high = asm_file.buffer[(*current)++];
            u16 disp = low | (high << 8);
            decode_memory(rm, memory_str, (s16)disp);
            if (d) {
                dest_str = decode_register(register_bits, w);
                source_str = memory_str;
            } else {
                source_str = decode_register(register_bits, w);
                dest_str = memory_str;
            }

        }

        // Construct disassembly string
        const char* mov_str = "mov ";
        const char* mid_str = ", ";
        const char* end_str = "\n";
        char* instruction_str = (char*)malloc(sizeof(char)*(strlen(mov_str)+strlen(mid_str)+strlen(end_str)+strlen(dest_str)+strlen(source_str)+1));
        strcpy(instruction_str, mov_str);
        strcat(instruction_str, dest_str);
        strcat(instruction_str, mid_str);
        strcat(instruction_str, source_str);
        strcat(instruction_str, end_str);
        return instruction_str;

    } else if ((asm_file.buffer[*current] & 0b11110000) == 0b10110000) {
        // Immediate to register

        // Read first byte
        u8 byte1 = asm_file.buffer[(*current)++];
        bool w = !!(byte1 & 0b00001000);
        u8 reg = byte1 & 0b00000111;

        const char* dest_str = decode_register(reg, w);
        char source_str[100];

        // Read first data byte
        u16 data = asm_file.buffer[(*current)++];
        if (w) {
            // There's a second data byte
            data |= asm_file.buffer[(*current)++] << 8;
        }
        sprintf(source_str, "%d", data);

        // Construct disassembly string
        const char* mov_str = "mov ";
        const char* mid_str = ", ";
        const char* end_str = "\n";
        char* instruction_str = (char*)malloc(sizeof(char)*(strlen(mov_str)+strlen(mid_str)+strlen(end_str)+strlen(dest_str)+strlen(source_str)+1));
        strcpy(instruction_str, mov_str);
        strcat(instruction_str, dest_str);
        strcat(instruction_str, mid_str);
        strcat(instruction_str, source_str);
        strcat(instruction_str, end_str);
        return instruction_str;
    } else if ((asm_file.buffer[*current] & 0b11111100) == 0b10100000) {
        // Accumulator to memory / memory to accumulator

        // Read first byte
        u8 byte1 = asm_file.buffer[(*current)++];
        bool d = !!(byte1 & 0b00000010);
        bool w = !!(byte1 & 0b00000001);

        u8 addr_lo = asm_file.buffer[(*current)++];
        u8 addr_hi = asm_file.buffer[(*current)++];
        u16 addr = addr_lo | (addr_hi << 8);

        const char* reg_str = w ? "AX" : "AL";

        char memory_str[100];
        strcpy(memory_str, "[");
        char* num_start = memory_str + strlen(memory_str);
        sprintf(num_start, "%d", addr);
        strcat(memory_str, "]");

        const char* dest_str = 0;
        const char* source_str = 0;
        if (d) {
            dest_str = memory_str;
            source_str = reg_str;
        } else {
            dest_str = reg_str;
            source_str = memory_str;
        }

        // Construct disassembly string
        const char* mov_str = "mov ";
        const char* mid_str = ", ";
        const char* end_str = "\n";
        char* instruction_str = (char*)malloc(sizeof(char)*(strlen(mov_str)+strlen(mid_str)+strlen(end_str)+strlen(dest_str)+strlen(source_str)+1));
        strcpy(instruction_str, mov_str);
        strcat(instruction_str, dest_str);
        strcat(instruction_str, mid_str);
        strcat(instruction_str, source_str);
        strcat(instruction_str, end_str);
        return instruction_str;
    } else if ((asm_file.buffer[*current] & 0b11111110) == 0b11000110) {
        // Immediate to register / memory

        // Read first byte
        u8 byte1 = asm_file.buffer[(*current)++];
        bool w = !!(byte1 & 0b00000001);

        u8 byte2 = asm_file.buffer[(*current)++];
        u8 mod = byte2 & 0b11000000;
        u8 rm = byte2 & 0b00000111;

        const char* dest_str = 0;

        if (mod == 0b11000000) {
            // Register mode (no displacement)

            dest_str = decode_register(rm, w);

        } else if (mod == 0b00000000) {
            // Memory mode (no displacement except for direct address)

            char memory_str[100]; // 100 should be large enough
            if (rm == 0b00000110) {
                // Use direct address. Read 2 more bytes for this.
                u8 low = asm_file.buffer[(*current)++];
                u8 high = asm_file.buffer[(*current)++];
                u16 address = low + (high << 8);
                strcpy(memory_str, "[");
                char* num_start = memory_str + strlen(memory_str);
                sprintf(num_start, "%d", address);
                strcat(memory_str, "]");
            } else {
                decode_memory(rm, memory_str, 0);
            }
            dest_str = memory_str;

        } else if (mod == 0b01000000) {
            // Memory mode with 8-bit displacement

            char memory_str[100]; // 100 should be large enough
            u8 low = asm_file.buffer[(*current)++];
            decode_memory(rm, memory_str, (s8)low);
            dest_str = memory_str;

        } else {
            // Memory mode with 16-bit displacement
            Assert(mod == 0b10000000);

            char memory_str[100]; // 100 should be large enough
            u8 low = asm_file.buffer[(*current)++];
            u8 high = asm_file.buffer[(*current)++];
            u16 disp = low | (high << 8);
            decode_memory(rm, memory_str, (s16)disp);
            dest_str = memory_str;
        }

        // Get more bytes for the immediate
        char source_str[100];

        // Read first data byte
        u16 data = asm_file.buffer[(*current)++];
        if (w) {
            // There's a second data byte
            data |= asm_file.buffer[(*current)++] << 8;
            sprintf(source_str, "word %d", data);
        } else {
            sprintf(source_str, "byte %d", data);
        }


        // Construct disassembly string
        const char* mov_str = "mov ";
        const char* mid_str = ", ";
        const char* end_str = "\n";
        char* instruction_str = (char*)malloc(sizeof(char)*(strlen(mov_str)+strlen(mid_str)+strlen(end_str)+strlen(dest_str)+strlen(source_str)+1));
        strcpy(instruction_str, mov_str);
        strcat(instruction_str, dest_str);
        strcat(instruction_str, mid_str);
        strcat(instruction_str, source_str);
        strcat(instruction_str, end_str);
        return instruction_str;
    }

    // Not supported yet
    Assert(FALSE);
    return "";
}

// disassemble ASM and output
void disassemble(Bytes asm_file, const char* output_path)
{
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
    Bytes bytes37 = read_entire_file("../data/listing_0039_more_movs");
    disassemble(bytes37, "../output/listing_0039_out.asm");

    Bytes bytes38 = read_entire_file("../data/listing_0040_challenge_movs");
    disassemble(bytes38, "../output/listing_0040_out.asm");

    return 0;
}