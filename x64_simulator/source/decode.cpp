
s32 get_label(s32 current, s8 offset, s32* label_indices, s32* label_count) {
    s32 location = current + offset;
    for (s32 i = 0; i < *label_count; ++i) {
        if (label_indices[i] == location) {
            return i+1;
        }
    }
    if (*label_count >= MAX_LABELS) {
        Assert(!"Too many labels.");
        return -1;
    }
    label_indices[(*label_count)++] = location;
    return *label_count;
}

MemoryAddress decode_memory(u8 rm) {
    if (rm == 0b00000000) {
        return MemoryAddress::BX_SI;
    } else if (rm == 0b00000001) {
        return MemoryAddress::BX_DI;
    } else if (rm == 0b00000010) {
        return MemoryAddress::BP_SI;
    } else if (rm == 0b00000011) {
        return MemoryAddress::BP_DI;
    } else if (rm == 0b00000100) {
        return MemoryAddress::SI;
    } else if (rm == 0b00000101) {
        return MemoryAddress::DI;
    } else if (rm == 0b00000110) {
        return MemoryAddress::BP;
    } else if (rm == 0b00000111) {
        return MemoryAddress::BX;
    } else {
        Assert(!"Invalid");
        return MemoryAddress::NONE;
    }
}

Register decode_register(u8 reg, bool w) 
{
    if (reg == 0b00000000) {
        return w ? Register::AX : Register::AL;
    } else if (reg == 0b00000001) {
        return w ? Register::CX : Register::CL;
    } else if (reg == 0b00000010) {
        return w ? Register::DX : Register::DL;
    } else if (reg == 0b00000011) {
        return w ? Register::BX : Register::BL;
    } else if (reg == 0b00000100) {
        return w ? Register::SP : Register::AH;
    } else if (reg == 0b00000101) {
        return w ? Register::BP : Register::CH;
    } else if (reg == 0b00000110) {
        return w ? Register::SI : Register::DH;
    } else if (reg == 0b00000111) {
        return w ? Register::DI : Register::BH;
    }
    Assert(!"Invalid reg code");
    return Register::NONE;
}

Register decode_segment_register(u8 reg)
{
        if (reg == 0b0) {
        return Register::ES;
    } else if (reg == 0b1) {
        return Register::CS;
    } else if (reg == 0b10) {
        return Register::SS;
    } else if (reg == 0b11) {
        return Register::DS;
    }
    Assert(!"Invalid reg code");
    return Register::NONE;
}

InstrType get_subvariant(u8 code) {
    if (code == 0b00000000) {
        return InstrType::ADD;
    } else if (code == 0b00101000) {
        return InstrType::SUB;
    } else if (code == 0b00111000) {
        return InstrType::CMP;
    } else {
        // Not supported yet
        Assert(FALSE);
        return InstrType::NONE;
    }
}

void extract_dwmodregrm(Bytes asm_file, int* current, bool segment_register, Instruction* out) {
        // We initially read from two bytes for this instruction
        u8 byte1 = asm_file.buffer[(*current)++];
        bool d = !!(byte1 & 0b00000010);
        bool w = !!(byte1 & 0b00000001);
        if (segment_register) {
            w = true;
        }

        u8 byte2 = asm_file.buffer[(*current)++];
        u8 mod = byte2 & 0b11000000;
        u8 reg = byte2 & 0b00111000;
        u8 rm = byte2 & 0b00000111;

        u8 register_bits = reg >> 3;

        if (mod == 0b11000000) {
            // Register mode (no displacement)

            out->operands[0].type = OperandType::REGISTER;
            out->operands[1].type = OperandType::REGISTER;
            Register reg_enum = segment_register ? decode_segment_register(register_bits) : decode_register(register_bits, w);
            if (d) {
                out->operands[0].reg = reg_enum;
                out->operands[1].reg = decode_register(rm, w);
            } else {
                out->operands[0].reg = decode_register(rm, w);
                out->operands[1].reg = reg_enum;
            }

        } else if (mod == 0b00000000) {
            // Memory mode (no displacement except for direct address)

            MemoryAddress address = MemoryAddress::DIRECT;
            u16 displacement = 0;
            if (rm == 0b00000110) {
                // Use direct address. Read 2 more bytes for this.
                u8 low = asm_file.buffer[(*current)++];
                u8 high = asm_file.buffer[(*current)++];
                displacement = low + (high << 8);
            } else {
                address = decode_memory(rm);
            }

            Register reg_enum = segment_register ? decode_segment_register(register_bits) : decode_register(register_bits, w);
            if (d) {
                out->operands[0].type = OperandType::REGISTER;
                out->operands[0].reg = reg_enum;
                out->operands[1].type = OperandType::MEMORY;
                out->operands[1].address = address;                
                out->operands[1].displacement = displacement;
            } else {
                out->operands[0].type = OperandType::MEMORY;
                out->operands[0].address = address;                
                out->operands[0].displacement = displacement;
                out->operands[1].type = OperandType::REGISTER;
                out->operands[1].reg = reg_enum;
            }

        } else if (mod == 0b01000000) {
            // Memory mode with 8-bit displacement

            u8 low = asm_file.buffer[(*current)++];
            s32 displacement = (s8)low;
            MemoryAddress address = decode_memory(rm);
            Register reg_enum = segment_register ? decode_segment_register(register_bits) : decode_register(register_bits, w);
            if (d) {
                out->operands[0].type = OperandType::REGISTER;
                out->operands[0].reg = reg_enum;
                out->operands[1].type = OperandType::MEMORY;
                out->operands[1].address = address;                
                out->operands[1].displacement = displacement;
            } else {
                out->operands[0].type = OperandType::MEMORY;
                out->operands[0].address = address;                
                out->operands[0].displacement = displacement;
                out->operands[1].type = OperandType::REGISTER;
                out->operands[1].reg = reg_enum;
            }

        } else {
            // Memory mode with 16-bit displacement
            Assert(mod == 0b10000000);

            u8 low = asm_file.buffer[(*current)++];
            u8 high = asm_file.buffer[(*current)++];
            s16 displacement = (s16)(low | (high << 8));
            MemoryAddress address = decode_memory(rm);
            Register reg_enum = segment_register ? decode_segment_register(register_bits) : decode_register(register_bits, w);
            if (d) {
                out->operands[0].type = OperandType::REGISTER;
                out->operands[0].reg = reg_enum;
                out->operands[1].type = OperandType::MEMORY;
                out->operands[1].address = address;                
                out->operands[1].displacement = displacement;
            } else {
                out->operands[0].type = OperandType::MEMORY;
                out->operands[0].address = address;                
                out->operands[0].displacement = displacement;
                out->operands[1].type = OperandType::REGISTER;
                out->operands[1].reg = reg_enum;
            }
        }
}

// s is an optional signed bit. set to false if not relevant
void extract_wmodrm(Bytes asm_file, int* current, bool s, Instruction* out) {
    // Read first byte
    u8 byte1 = asm_file.buffer[(*current)++];
    bool w = !!(byte1 & 0b00000001);

    u8 byte2 = asm_file.buffer[(*current)++];
    u8 mod = byte2 & 0b11000000;
    u8 rm = byte2 & 0b00000111;

    if (mod == 0b11000000) {
        // Register mode (no displacement)
        out->operands[0].type = OperandType::REGISTER;
        out->operands[0].reg = decode_register(rm, w);

    } else if (mod == 0b00000000) {
        // Memory mode (no displacement except for direct address)

        MemoryAddress address = MemoryAddress::DIRECT;
        u16 displacement = 0;
        if (rm == 0b00000110) {
            // Use direct address. Read 2 more bytes for this.
            u8 low = asm_file.buffer[(*current)++];
            u8 high = asm_file.buffer[(*current)++];
            displacement = low + (high << 8);
        } else {
            address = decode_memory(rm);
        }
        out->operands[0].type = OperandType::MEMORY;
        out->operands[0].address = address;
        out->operands[0].displacement = displacement;

    } else if (mod == 0b01000000) {
        // Memory mode with 8-bit displacement

        u8 low = asm_file.buffer[(*current)++];
        out->operands[0].type = OperandType::MEMORY;
        out->operands[0].address = decode_memory(rm);
        out->operands[0].displacement = (s8)low;

    } else {
        // Memory mode with 16-bit displacement
        Assert(mod == 0b10000000);

        u8 low = asm_file.buffer[(*current)++];
        u8 high = asm_file.buffer[(*current)++];
        u16 disp = low | (high << 8);
        out->operands[0].type = OperandType::MEMORY;
        out->operands[0].address = decode_memory(rm);
        out->operands[0].displacement = (s16)disp;
    }

    out->operands[1].type = OperandType::IMMEDIATE;
    // Read first data byte
    u8 data8 = asm_file.buffer[(*current)++];
    if (w && !s) {
        // There's a second data byte
        u16 data16 = data8;
        data16 |= asm_file.buffer[(*current)++] << 8;
        out->operands[1].immediate = data16;
        out->operands[1].byte = false;
    } else if (w && s) {
        s16 data16 = (s8)data8;
        out->operands[1].immediate = data16;
        out->operands[1].byte = false;
    } else if (!w && !s) {
        out->operands[1].immediate = data8;
        out->operands[1].byte = true;
    } else if (!w && s) {
        out->operands[1].immediate = (s8)data8;
        out->operands[1].byte = true;
    }
}

// Returns malloced char* (null-terminated)
Instruction decode_instruction(Bytes asm_file, s32* current, s32* label_indices, s32* label_count) {

    if ((asm_file.buffer[*current] & 0b11111100) == 0b10001000) {
        // MOV register/memory to/from register

        Instruction decoded;
        decoded.type = InstrType::MOV;
        extract_dwmodregrm(asm_file, current, false, &decoded);
        return decoded;

    } else if ((asm_file.buffer[*current] & 0b11110000) == 0b10110000) {
        // MOV immediate to register

        // Read first byte
        u8 byte1 = asm_file.buffer[(*current)++];
        bool w = !!(byte1 & 0b00001000);
        u8 reg = byte1 & 0b00000111;

        // Read first data byte
        u16 data = asm_file.buffer[(*current)++];
        bool byte = true;
        if (w) {
            // There's a second data byte
            data |= asm_file.buffer[(*current)++] << 8;
            byte = false;
        }

        Instruction decoded;
        decoded.type = InstrType::MOV;
        decoded.operands[0].type = OperandType::REGISTER;
        decoded.operands[0].reg = decode_register(reg, w);
        decoded.operands[1].type = OperandType::IMMEDIATE;
        decoded.operands[1].immediate = data;
        decoded.operands[1].byte = byte;
        return decoded;
    } else if ((asm_file.buffer[*current] & 0b11111100) == 0b10100000) {
        // MOV accumulator to memory / memory to accumulator

        // Read first byte
        u8 byte1 = asm_file.buffer[(*current)++];
        bool d = !!(byte1 & 0b00000010);
        bool w = !!(byte1 & 0b00000001);

        u8 addr_lo = asm_file.buffer[(*current)++];
        u8 addr_hi = asm_file.buffer[(*current)++];
        u16 addr = addr_lo | (addr_hi << 8);

        Register reg = w ? Register::AX : Register::AL;

        Instruction decoded;
        decoded.type = InstrType::MOV;
        if (d) {
            decoded.operands[0].type = OperandType::MEMORY;
            decoded.operands[0].address = MemoryAddress::DIRECT; 
            decoded.operands[0].displacement = addr; 
            decoded.operands[1].type = OperandType::REGISTER;
            decoded.operands[1].reg = reg;
        } else {
            decoded.operands[0].type = OperandType::REGISTER;
            decoded.operands[0].reg = reg;
            decoded.operands[1].type = OperandType::MEMORY;
            decoded.operands[1].address = MemoryAddress::DIRECT; 
            decoded.operands[1].displacement = addr; 
        }

        return decoded;
    } else if ((asm_file.buffer[*current] & 0b11111110) == 0b11000110) {
        // MOV immediate to register / memory

        Instruction decoded;
        decoded.type = InstrType::MOV;
        extract_wmodrm(asm_file, current, false, &decoded);
        return decoded;
    } else if ((asm_file.buffer[*current] & 0b11000100) == 0b00000000) {
        // Some subvariant of reg/memory and register to either

        Instruction decoded;
        decoded.type = get_subvariant(asm_file.buffer[*current] & 0b00111000);
        extract_dwmodregrm(asm_file, current, false, &decoded);
        return decoded;
    } else if  ((asm_file.buffer[*current] & 0b11111100) == 0b10000000) {
        // Some subvariant of immediate to register / memory

        bool s = !!(asm_file.buffer[*current] & 0b00000010);

        Instruction decoded;
        decoded.type = get_subvariant(asm_file.buffer[*current+1] & 0b00111000);
        extract_wmodrm(asm_file, current, s, &decoded);
        return decoded;
    } else if ((asm_file.buffer[*current] & 0b11000110) == 0b00000100) {
        // Some subvariant of immediate to accumulator

        Instruction decoded;
        decoded.type = get_subvariant(asm_file.buffer[*current] & 0b00111000);
        bool w = !!(asm_file.buffer[*current] & 0b00000001);
        (*current)++;

        decoded.operands[0].type = OperandType::REGISTER;
        decoded.operands[0].reg = w ? Register::AX : Register::AL;
        decoded.operands[1].type = OperandType::IMMEDIATE;
        u16 data = asm_file.buffer[(*current)++];
        decoded.operands[1].byte = true;
        if (w) {
            // There's a second data byte
            data |= asm_file.buffer[(*current)++] << 8;
            decoded.operands[1].byte = false;
        }
        decoded.operands[1].immediate = data;
        return decoded;
    } else if ((asm_file.buffer[*current] & 0b11111100) == 0b10001100) {
        // MOV segment register to/from memory

        Instruction decoded;
        decoded.type = InstrType::MOV;
        extract_dwmodregrm(asm_file, current, true, &decoded);
        return decoded;
    } else if (asm_file.buffer[*current] == 0b01110100) {
        // JE
        (*current)++;
        s8 offset = (s8)asm_file.buffer[(*current)++];

        Instruction decoded;
        decoded.type = InstrType::JE;
        decoded.operands[0].type = OperandType::JUMP_OFFSET;
        decoded.operands[0].offset = offset;
        decoded.operands[0].label = get_label(*current, offset, label_indices, label_count);
        return decoded;
    } else if (asm_file.buffer[*current] == 0b01111100) {
        // JL
        (*current)++;
        s8 offset = (s8)asm_file.buffer[(*current)++];

        Instruction decoded;
        decoded.type = InstrType::JL;
        decoded.operands[0].type = OperandType::JUMP_OFFSET;
        decoded.operands[0].offset = offset;
        decoded.operands[0].label = get_label(*current, offset, label_indices, label_count);
        return decoded;
    } else if (asm_file.buffer[*current] == 0b01111110) {
        // JLE
        (*current)++;
        s8 offset = (s8)asm_file.buffer[(*current)++];

        Instruction decoded;
        decoded.type = InstrType::JLE;
        decoded.operands[0].type = OperandType::JUMP_OFFSET;
        decoded.operands[0].offset = offset;
        decoded.operands[0].label = get_label(*current, offset, label_indices, label_count);
        return decoded;
    } else if (asm_file.buffer[*current] == 0b01110010) {
        // JB
        (*current)++;
        s8 offset = (s8)asm_file.buffer[(*current)++];

        Instruction decoded;
        decoded.type = InstrType::JB;
        decoded.operands[0].type = OperandType::JUMP_OFFSET;
        decoded.operands[0].offset = offset;
        decoded.operands[0].label = get_label(*current, offset, label_indices, label_count);
        return decoded;
    } else if (asm_file.buffer[*current] == 0b01110110) {
        // JBE
        (*current)++;
        s8 offset = (s8)asm_file.buffer[(*current)++];

        Instruction decoded;
        decoded.type = InstrType::JBE;
        decoded.operands[0].type = OperandType::JUMP_OFFSET;
        decoded.operands[0].offset = offset;
        decoded.operands[0].label = get_label(*current, offset, label_indices, label_count);
        return decoded;
    } else if (asm_file.buffer[*current] == 0b01111010) {
        // JP
        (*current)++;
        s8 offset = (s8)asm_file.buffer[(*current)++];

        Instruction decoded;
        decoded.type = InstrType::JP;
        decoded.operands[0].type = OperandType::JUMP_OFFSET;
        decoded.operands[0].offset = offset;
        decoded.operands[0].label = get_label(*current, offset, label_indices, label_count);
        return decoded;
    } else if (asm_file.buffer[*current] == 0b01110000) {
        // JO
        (*current)++;
        s8 offset = (s8)asm_file.buffer[(*current)++];

        Instruction decoded;
        decoded.type = InstrType::JO;
        decoded.operands[0].type = OperandType::JUMP_OFFSET;
        decoded.operands[0].offset = offset;
        decoded.operands[0].label = get_label(*current, offset, label_indices, label_count);
        return decoded;
    } else if (asm_file.buffer[*current] == 0b01111000) {
        // JS
        (*current)++;
        s8 offset = (s8)asm_file.buffer[(*current)++];

        Instruction decoded;
        decoded.type = InstrType::JS;
        decoded.operands[0].type = OperandType::JUMP_OFFSET;
        decoded.operands[0].offset = offset;
        decoded.operands[0].label = get_label(*current, offset, label_indices, label_count);
        return decoded;
    } else if (asm_file.buffer[*current] == 0b01110101) {
        // JNE
        (*current)++;
        s8 offset = (s8)asm_file.buffer[(*current)++];

        Instruction decoded;
        decoded.type = InstrType::JNE;
        decoded.operands[0].type = OperandType::JUMP_OFFSET;
        decoded.operands[0].offset = offset;
        decoded.operands[0].label = get_label(*current, offset, label_indices, label_count);
        return decoded;
    } else if (asm_file.buffer[*current] == 0b01111101) {
        // JNL
        (*current)++;
        s8 offset = (s8)asm_file.buffer[(*current)++];

        Instruction decoded;
        decoded.type = InstrType::JNL;
        decoded.operands[0].type = OperandType::JUMP_OFFSET;
        decoded.operands[0].offset = offset;
        decoded.operands[0].label = get_label(*current, offset, label_indices, label_count);
        return decoded;
    } else if (asm_file.buffer[*current] == 0b01111111) {
        // JG
        (*current)++;
        s8 offset = (s8)asm_file.buffer[(*current)++];

        Instruction decoded;
        decoded.type = InstrType::JG;
        decoded.operands[0].type = OperandType::JUMP_OFFSET;
        decoded.operands[0].offset = offset;
        decoded.operands[0].label = get_label(*current, offset, label_indices, label_count);
        return decoded;
    } else if (asm_file.buffer[*current] == 0b01110011) {
        // JNB
        (*current)++;
        s8 offset = (s8)asm_file.buffer[(*current)++];

        Instruction decoded;
        decoded.type = InstrType::JNB;
        decoded.operands[0].type = OperandType::JUMP_OFFSET;
        decoded.operands[0].offset = offset;
        decoded.operands[0].label = get_label(*current, offset, label_indices, label_count);
        return decoded;
    } else if (asm_file.buffer[*current] == 0b01110111) {
        // JA
        (*current)++;
        s8 offset = (s8)asm_file.buffer[(*current)++];

        Instruction decoded;
        decoded.type = InstrType::JA;
        decoded.operands[0].type = OperandType::JUMP_OFFSET;
        decoded.operands[0].offset = offset;
        decoded.operands[0].label = get_label(*current, offset, label_indices, label_count);
        return decoded;
    } else if (asm_file.buffer[*current] == 0b01111011) {
        // JNP
        (*current)++;
        s8 offset = (s8)asm_file.buffer[(*current)++];

        Instruction decoded;
        decoded.type = InstrType::JNP;
        decoded.operands[0].type = OperandType::JUMP_OFFSET;
        decoded.operands[0].offset = offset;
        decoded.operands[0].label = get_label(*current, offset, label_indices, label_count);
        return decoded;
    } else if (asm_file.buffer[*current] == 0b01110001) {
        // JNO
        (*current)++;
        s8 offset = (s8)asm_file.buffer[(*current)++];

        Instruction decoded;
        decoded.type = InstrType::JNO;
        decoded.operands[0].type = OperandType::JUMP_OFFSET;
        decoded.operands[0].offset = offset;
        decoded.operands[0].label = get_label(*current, offset, label_indices, label_count);
        return decoded;
    } else if (asm_file.buffer[*current] == 0b01111001) {
        // JNS
        (*current)++;
        s8 offset = (s8)asm_file.buffer[(*current)++];

        Instruction decoded;
        decoded.type = InstrType::JNS;
        decoded.operands[0].type = OperandType::JUMP_OFFSET;
        decoded.operands[0].offset = offset;
        decoded.operands[0].label = get_label(*current, offset, label_indices, label_count);
        return decoded;
    } else if (asm_file.buffer[*current] == 0b11100010) {
        // LOOP
        (*current)++;
        s8 offset = (s8)asm_file.buffer[(*current)++];

        Instruction decoded;
        decoded.type = InstrType::LOOP;
        decoded.operands[0].type = OperandType::JUMP_OFFSET;
        decoded.operands[0].offset = offset;
        decoded.operands[0].label = get_label(*current, offset, label_indices, label_count);
        return decoded;
    } else if (asm_file.buffer[*current] == 0b11100001) {
        // LOOPZ
        (*current)++;
        s8 offset = (s8)asm_file.buffer[(*current)++];

        Instruction decoded;
        decoded.type = InstrType::LOOPZ;
        decoded.operands[0].type = OperandType::JUMP_OFFSET;
        decoded.operands[0].offset = offset;
        decoded.operands[0].label = get_label(*current, offset, label_indices, label_count);
        return decoded;
    } else if (asm_file.buffer[*current] == 0b11100000) {
        // LOOPNZ
        (*current)++;
        s8 offset = (s8)asm_file.buffer[(*current)++];

        Instruction decoded;
        decoded.type = InstrType::LOOPNZ;
        decoded.operands[0].type = OperandType::JUMP_OFFSET;
        decoded.operands[0].offset = offset;
        decoded.operands[0].label = get_label(*current, offset, label_indices, label_count);
        return decoded;
    } else if (asm_file.buffer[*current] == 0b11100011) {
        // JCXZ
        (*current)++;
        s8 offset = (s8)asm_file.buffer[(*current)++];

        Instruction decoded;
        decoded.type = InstrType::JCXZ;
        decoded.operands[0].type = OperandType::JUMP_OFFSET;
        decoded.operands[0].offset = offset;
        decoded.operands[0].label = get_label(*current, offset, label_indices, label_count);
        return decoded;
    }

    // Not supported yet
    Assert(FALSE);
    return Instruction();
}