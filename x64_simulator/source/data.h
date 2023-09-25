#define MAX_LABELS 1000
#define MAX_INSTRUCTIONS 10000

struct Bytes {
    u8* buffer = 0; // malloced buffer
    s32 size = 0;
};

// Processor type to simulate for clock count
enum class ClockType {
    COUNT_NONE,
    COUNT_8086,
    COUNT_8088
};

enum Flags {
    CARRY_FLAG = 1 << 0,
    PARITY_FLAG = 1 << 2,
    AUX_CARRY_FLAG = 1 << 4,
    ZERO_FLAG = 1 << 6,
    SIGN_FLAG = 1 << 7,
    TRAP_FLAG = 1 << 8,
    INTERRUPT_FLAG = 1 << 9,
    DIRECTION_FLAG = 1 << 10,
    OVERFLOW_FLAG = 1 << 11
};

// Store simulated processor context
struct Context {
    // Registers
    u16 ax = 0;
    u16 bx = 0;
    u16 cx = 0;
    u16 dx = 0;
    u16 sp = 0;
    u16 bp = 0;
    u16 si = 0;
    u16 di = 0;

    // Segment registers
    u16 es = 0;
    u16 cs = 0;
    u16 ss = 0;
    u16 ds = 0;

    // 0b 0 0 0 0 OF DF IF TF SF ZF 0 AF 0 PF 0 CF
    // OF Overflow Flag
    // DF Direction Flag
    // IF Interrupt Flag
    // TF Trap Flag
    // SF Sign Flag
    // ZF Zero Flag
    // AF Auxiliary Carry Flag
    // PF Parity Flag
    // CF Carry Flag
    u16 flags = 0;

    // Instruction pointer
    u16 ip = 0;

    // Running count of clocks
    s32 clocks = 0;

    // Memory
    u8* memory = 0;
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

    // Segment registers
    ES,
    CS,
    SS,
    DS,

    FLAGS,
    IP,

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
    bool byte; // If just one byte, will be in lower byte of immediate. Otherwise will be lower two bytes.

    // OperandType::JUMP_OFFSET
    s8 offset;
    s32 label;
};

struct Instruction {
    InstrType type = InstrType::NONE;
    Operand operands[2] = {};
};