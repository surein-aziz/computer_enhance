#define MAX_LABELS 1000
#define MAX_INSTRUCTIONS 10000

struct Bytes {
    u8* buffer = 0; // malloced buffer
    s32 size = 0;
};

enum class InstrType {
    NONE,

    MOV, //TODO: instruction_line("mov ", dest_str, source_str);
    ADD,//TODO: instruction_line("add ", dest_str, source_str);
    SUB,//TODO: instruction_line("sub ", dest_str, source_str);
    CMP,//TODO: instruction_line("cmp ", dest_str, source_str);
    JE,
    JL,
    JLE,
    JB,
    JBE,
    JP,
    JO,
    JS,
    JNE,
    JNL,
    JG,
    JNB,
    JA,
    JNP,
    JNO,
    JNS,
    LOOP,
    LOOPZ,
    LOOPNZ,
    JCXZ,
    
    COUNT
};

enum class Register {
    NONE,

    AX,
    AL,
    AH,
    BX,
    BL,
    BH,
    CX,
    CL,
    CH,
    DX,
    DL,
    DH,
    SP,
    BP,
    SI,
    DI,

    COUNT
};

enum class MemoryAddress {
    NONE,

    DIRECT,
    BX_SI,
    BX_DI,
    BP_SI,
    BP_DI,
    SI,
    DI,
    BP,
    BX,

    COUNT
};

enum class OperandType {
    NONE,

    REGISTER,
    MEMORY,
    IMMEDIATE,
    JUMP_OFFSET,

    COUNT
};

struct JumpOffset {
    s8 offset;
    s32 label;
};

struct Operand {
    OperandType type;
    union {
        // OperandType::REGISTER
        Register reg; // TODO: Just make e.g. a switch to get the string

        // OperandType::MEMORY
        MemoryAddress address;
        s32 displacement;
        /* TODO: get string as in eg

        strcpy(memory_str, "[");
        char* num_start = memory_str + strlen(memory_str);
        sprintf(num_start, "%d", address);
        strcat(memory_str, "]");

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
        */

        // OperandType::IMMEDIATE
        s32 immediate; // TODO: char source_str[100]; sprintf(source_str, "%d", data); sprintf(data_str, "word %d", data16); sprintf(data_str, "byte %d", data8);
        bool byte;

        // OperandType::JUMP_OFFSET
        JumpOffset jump;

        /* TODO print
        void get_label(char* label_str, s32 current, s8 offset, s32* label_indices, s32* label_count) {
    strcpy(label_str, "label_");
    s32 location = current + offset;
    for (s32 i = 0; i < *label_count; ++i) {
        if (label_indices[i] == location) {
            char num_str[20];
            sprintf(num_str, "%d", i+1);
            strcat(label_str, num_str);
            return;
        }
    }
    if (*label_count >= MAX_LABELS) {
        Assert(!"Too many labels.");
        return;
    }
    char num_str[20];
    sprintf(num_str, "%d", *label_count+1);
    strcat(label_str, num_str);
    label_indices[(*label_count)++] = location;
}
        */
    };
};

struct Instruction {
    InstrType type = InstrType::NONE;
    Operand operands[2] = {};
};