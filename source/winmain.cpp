#include <stdio.h>

#include "windows.h"
#include "helpful.h"
#include "data.h"

// Single unit compilation
#include "decode.cpp"

//TODO(surein): clean up string handling ensuring no memory leaks
//TODO(surein): complete disassembler sufficiently to complete listing 42

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

// Get instruction line string given substrings. Returns malloced ptr.
char* instruction_line(const char* instruction, const char* operand0, const char* operand1) {
    const char* space_str = " ";
    const char* mid_str = ", ";
    const char* end_str = "\n";
    char* instruction_str = (char*)malloc(sizeof(char)*(strlen(instruction)+strlen(space_str)+strlen(mid_str)+strlen(end_str)+strlen(operand0)+strlen(operand1)+1));
    strcpy(instruction_str, instruction);
    strcat(instruction_str, space_str);
    strcat(instruction_str, operand0);
    strcat(instruction_str, mid_str);
    strcat(instruction_str, operand1);
    strcat(instruction_str, end_str);
    return instruction_str;
}

// Get instruction line string given substrings for a conditional jump. Returns malloced ptr.
char* instruction_line_jump(const char* instruction, s8 offset, s32 label) {
    char offset_str[20];
    sprintf(offset_str, "%d", offset);
    char label_str[100];
    strcpy(label_str, "label_");
    sprintf(label_str + strlen(label_str), "%d", label);
    const char* space_str = " ";
    const char* mid_str = " ; ";
    const char* end_str = "\n";
    char* instruction_str = (char*)malloc(sizeof(char)*(strlen(instruction)+strlen(space_str)+strlen(end_str)+strlen(mid_str) + strlen(label_str) + strlen(offset_str)+1));
    strcpy(instruction_str, instruction);
    strcat(instruction_str, space_str);
    strcat(instruction_str, label_str);
    strcat(instruction_str, mid_str); 
    strcat(instruction_str, offset_str);
    strcat(instruction_str, end_str);
    return instruction_str;
}

const char* instruction_str(InstrType type) {
    switch (type) {
        case InstrType::MOV: return "mov";
        case InstrType::ADD: return "add";
        case InstrType::SUB: return "sub";
        case InstrType::CMP: return "cmp";
        case InstrType::JE: return "je";
        case InstrType::JL: return "jl";
        case InstrType::JLE: return "jle";
        case InstrType::JB: return "jb";
        case InstrType::JBE: return "jbe";
        case InstrType::JP: return "jp";
        case InstrType::JO: return "jo";
        case InstrType::JS: return "js";
        case InstrType::JNE: return "jne";
        case InstrType::JNL: return "jnl";
        case InstrType::JG: return "jg";
        case InstrType::JNB: return "jnb";
        case InstrType::JA: return "ja";
        case InstrType::JNP: return "jnp";
        case InstrType::JNO: return "jno";
        case InstrType::JNS: return "jns";
        case InstrType::LOOP: return "loop";
        case InstrType::LOOPZ: return "loopz";
        case InstrType::LOOPNZ: return "loopnz";
        case InstrType::JCXZ: return "jcxz";
        case InstrType::NONE:
        case InstrType::COUNT:
            break;
    }
    Assert(!"Invalid instruction.");
    return "";
}

const char* reg_str(Register type) {
    switch (type) {
        case Register::AX: return "AX";
        case Register::AL: return "AL";
        case Register::AH: return "AH";
        case Register::BX: return "BX";
        case Register::BL: return "BL";
        case Register::BH: return "BH";
        case Register::CX: return "CX";
        case Register::CL: return "CL";
        case Register::CH: return "CH";
        case Register::DX: return "DX";
        case Register::DL: return "DL";
        case Register::DH: return "DH";
        case Register::SP: return "SP";
        case Register::BP: return "BP";
        case Register::SI: return "SI";
        case Register::DI: return "DI";
        case Register::NONE:
        case Register::COUNT:
            break;
    }
    Assert(!"Invalid register.");
    return "";
}

const char* memory_address_str(MemoryAddress address) {
    switch (address) {
        case MemoryAddress::BX_SI: return "BX + SI";
        case MemoryAddress::BX_DI: return "BX + DI";
        case MemoryAddress::BP_SI: return "BP + SI";
        case MemoryAddress::BP_DI: return "BP + DI";
        case MemoryAddress::SI: return "SI";
        case MemoryAddress::DI: return "DI";
        case MemoryAddress::BP: return "BP";
        case MemoryAddress::BX: return "BX";
        case MemoryAddress::DIRECT:
        case MemoryAddress::NONE:
        case MemoryAddress::COUNT:
            break;
    }
    Assert(!"Invalid memory address.");
    return "";
}

// Returns malloced string for operand
char* operand_str(Operand operand) {
    switch (operand.type) {
        case OperandType::REGISTER:
            {
                const char* reg_string = reg_str(operand.reg);
                char* reg_string_malloc = (char*)malloc((strlen(reg_string)+1)*sizeof(char));
                strcpy(reg_string_malloc, reg_string);
                return reg_string_malloc;
            }
        case OperandType::MEMORY:
            {
                char* mem_str = (char*)malloc(100*sizeof(char));
                strcpy(mem_str, "[");
                if (operand.address == MemoryAddress::DIRECT) {
                    char* cur = mem_str + strlen(mem_str);
                    sprintf(cur, "%d", operand.displacement);
                } else {
                    const char* memory_address_string = memory_address_str(operand.address);
                    strcat(mem_str, memory_address_string);
                    char* cur = mem_str + strlen(mem_str);
                    if (operand.displacement > 0) {
                        sprintf(cur, " + %d", operand.displacement);
                    } else if (operand.displacement < 0) {
                        sprintf(cur, " - %d", -operand.displacement);
                    }
                }
                strcat(mem_str, "]");
                return mem_str;
            }
        case OperandType::IMMEDIATE:
            {
                char* immediate_str = (char*)malloc(100*sizeof(char));
                if (operand.byte) {
                    sprintf(immediate_str, "byte %d", operand.immediate);
                } else {
                    sprintf(immediate_str, "word %d", operand.immediate);
                }
                return immediate_str;
            }
        case OperandType::JUMP_OFFSET:
            // Note: jump offset operand cannot be used with a second operand. We expect caller to handle outputting this. Assert for now. We could improve this later.
        case OperandType::COUNT:
        case OperandType::NONE:
            break;
    }
    Assert(!"Invalid operand type");
    return 0;
}

Bytes write_instruction_line(Bytes bytes, Instruction instruction) {
    char* str = 0;
    switch (instruction.type) {
        case InstrType::MOV:
        case InstrType::ADD:
        case InstrType::SUB:
        case InstrType::CMP: 
            {
                char* dest_operand = operand_str(instruction.operands[0]);
                char* source_operand = operand_str(instruction.operands[1]);
                str = instruction_line(instruction_str(instruction.type), dest_operand, source_operand);
                free(dest_operand);
                free(source_operand);
                break;
            }
        case InstrType::JE:
        case InstrType::JL:
        case InstrType::JLE:
        case InstrType::JB:
        case InstrType::JBE:
        case InstrType::JP:
        case InstrType::JO:
        case InstrType::JS:
        case InstrType::JNE:
        case InstrType::JNL:
        case InstrType::JG:
        case InstrType::JNB:
        case InstrType::JA:
        case InstrType::JNP:
        case InstrType::JNO:
        case InstrType::JNS:
        case InstrType::LOOP:
        case InstrType::LOOPZ:
        case InstrType::LOOPNZ:
        case InstrType::JCXZ: 
            {
                Assert(instruction.operands[0].type == OperandType::JUMP_OFFSET);
                str = instruction_line_jump(instruction_str(instruction.type), instruction.operands[0].offset, instruction.operands[0].label);
                break;
            }
        case InstrType::NONE:
        case InstrType::COUNT:
            Assert(!"Invalid instruction.");
            break;
    }

    if (!str) {
        Assert(!"Failed to generate instruction line string.");
        return bytes;
    }

    Bytes out = append_chars(bytes, str);
    free(str);
    return out;
}

// disassemble ASM, optionally execute it, and output
void process(Bytes asm_file, const char* output_path, bool exec)
{
    Bytes output;
    s32 current = 0;

    // Save number of bytes output for each instruction
    s32* instruction_bytes = (s32*)malloc(asm_file.size*sizeof(s32));

    // Save index of each label found.
    s32* label_indices = (s32*)malloc(MAX_LABELS*sizeof(s32));
    s32 label_count = 0;

    Instruction* instructions = (Instruction*)malloc(MAX_INSTRUCTIONS*sizeof(Instruction));
    int instruction_count = 0;
    
    while (current < asm_file.size) {
        s32 prev = current;

        instructions[instruction_count] = decode_instruction(asm_file, &current, label_indices, &label_count);
        Assert(instruction_count < MAX_INSTRUCTIONS);
        if (current == prev) {
            Assert(!"Decode failed.");
            break;
        }
        instruction_bytes[instruction_count] = current-prev;
        instruction_count++;
    }

    // Output to bytes ready to be output to file.
    const char* preamble = "bits 16\n";
    output = append_chars(output, preamble);

    s32 labels_inserted = 0; // All labels should be inserted. Debugging check.
    s32 total_bytes = 0; // Keep track number of input bytes corresponding to instructions output so far.
    s32 next_label_index = get_next_label_index(total_bytes, label_count, label_indices);
    for (int i = 0; i < instruction_count; ++i) {
        output = write_instruction_line(output, instructions[i]);

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

    free(instructions);
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