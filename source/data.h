#define MAX_LABELS 1000
#define MAX_LINES 10000

struct Bytes {
    u8* buffer = 0; // malloced buffer
    s32 size = 0;
};

enum class InstrType {
    NONE,

    MOV,
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

enum class OperandType {
    NONE,

    REGISTER,
    //TODO(surein)

    COUNT
};

struct Operand {
    int test = 0; //TODO(surein)
};

struct Instruction {
    InstrType type = InstrType::NONE;
    Operand operands[2];
};