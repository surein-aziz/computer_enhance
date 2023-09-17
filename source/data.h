#define MAX_LABELS 1000
#define MAX_INSTRUCTIONS 10000

struct Bytes {
    u8* buffer = 0; // malloced buffer
    s32 size = 0;
};

enum class InstrType {
    NONE,

    MOV,
    ADD,
    SUB,
    CMP,
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

struct Operand {
    OperandType type;

    // Note: this could be a union with some fiddling.
    
    // OperandType::REGISTER
    Register reg;

    // OperandType::MEMORY
    MemoryAddress address;
    s32 displacement;

    // OperandType::IMMEDIATE
    s32 immediate;
    bool byte;

    // OperandType::JUMP_OFFSET
    s8 offset;
    s32 label;
};

struct Instruction {
    InstrType type = InstrType::NONE;
    Operand operands[2] = {};
};