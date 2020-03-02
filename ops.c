#include "ops.h"

const char *opNames[256] = {
    "NOP", "LD BC,d16", "LD (BC),A", "INC BC", "INC B", "DEC B", "LD B,d8", "RLCA",
    "LD (a16),SP", "ADD HL,BC", "LD A,(BC)", "DEC BC", "INC C", "DEC C", "LD C,d8", "RRCA",
    "STOP", "LD DE,d16", "LD (DE),A", "INC DE", "INC D", "DEC D", "LD D,d8", "RLA",
    "JR r8", "ADD HL,DE", "LD A,(DE)", "DEC DE", "INC E", "DEC E", "LD E,d8", "RRA",
    "JR NZ,r8", "LD HL,d16", "LDI (HL),A", "INC HL", "INC H", "DEC H", "LD H,d8", "DAA",
    "JR Z,r8", "ADD HL,HL", "LDI A,(HL)", "DEC HL", "INC L", "DEC L", "LD L,d8", "CPL",
    "JR NC,r8", "LD SP,d16", "LDD (HL),A", "INC SP", "INC (HL)", "DEC (HL)", "LD (HL),d8", "SCF",
    "JR C,r8", "ADD HL,SP", "LDD A,(HL)", "DEC SP", "INC A", "DEC A", "LD A,d8", "CCF"
    "LD B,B", "LD B,C", "LD B,D", "LD B,E", "LD B,H", "LD B,L", "LD B,(HL)", "LD B,A",
    "LD C,B", "LD C,C", "LD C,D", "LD C,E", "LD C,H", "LD C,L", "LD C,(HL)", "LD C,A",
    "LD D,B", "LD D,C", "LD D,D", "LD D,E", "LD D,H", "LD D,L", "LD D,(HL)", "LD D,A",
    "LD E,B", "LD E,C", "LD E,D", "LD E,E", "LD E,H", "LD E,L", "LD E,(HL)", "LD E,A",
    "LD H,B", "LD H,C", "LD H,D", "LD H,E", "LD H,H", "LD H,L", "LD H,(HL)", "LD H,A",
    "LD L,B", "LD L,C", "LD L,D", "LD L,E", "LD L,H", "LD L,L", "LD L,(HL)", "LD L,A",
    "LD (HL),B", "LD (HL),C", "LD (HL),D", "LD (HL),E", "LD (HL),H", "LD (HL),L", "HALT", "LD (HL),A",
    "LD A,B", "LD A,C", "LD A,D", "LD A,E", "LD A,H", "LD A,L", "LD A,(HL)", "LD A,A",
    "ADD A,B", "ADD A,C", "ADD A,D", "ADD A,E", "ADD A,H", "ADD A,L", "ADD A,(HL)", "ADD A,A",
    "ADC A,B", "ADC A,C", "ADC A,D", "ADC A,E", "ADC A,H", "ADC A,L", "ADC A,(HL)", "ADC A,A",
    "SUB B", "SUB C", "SUB D", "SUB E", "SUB H", "SUB L", "SUB (HL)", "SUB A",
    "SBC A,B", "SBC A,C", "SBC A,D", "SBC A,E", "SBC A,H", "SBC A,L", "SBC A,(HL)", "SBC A,A",
    "AND B", "AND C", "AND D", "AND E", "AND H", "AND L", "AND (HL)", "AND A",
    "XOR B", "XOR C", "XOR D", "XOR E", "XOR H", "XOR L", "XOR (HL)", "XOR A",
    "OR B", "OR C", "OR D", "OR E", "OR H", "OR L", "OR (HL)", "OR A",
    "CP B", "CP C", "CP D", "CP E", "CP H", "CP L", "CP (HL)", "CP A",
    "RET NZ", "POP BC", "JP NZ,a16", "JP a16", "CALL NZ,a16", "PUSH BC", "ADD A,d8", "RST 00H",
    "RET Z", "RET", "JP Z,a16", "PREFIX CB", "CALL Z,a16", "CALL a16", "ADC A,d8", "RST 08H",
    "RET NC", "POP DE", "JP NC,a16", "INVALID", "CALL NC,a16", "PUSH DE", "SUB d8", "RST 10H",
    "RET C", "RETI", "JP C,a16", "INVALID", "CALL C,a16", "INVALID", "SBC A,d8", "RST 18H",
    "LD ($FF00+a8),A", "POP HL", "LD ($FF00+C),A", "INVALID", "INVALID", "PUSH HL", "AND,d8", "RST 20H",
    "ADD SP,r8", "JP (HL)", "LD (a16),A", "INVALID", "INVALID", "INVALID", "XOR d8", "RST 28H",
    "LD A,($FF00+a8)", "POP AF", "LD A,($FF00+C)", "DI", "INVALID", "PUSH AF", "OR d8", "RST 30H",
    "LD HL,SP+r8", "LD SP,HL", "LD A,(a16)", "EI", "INVALID", "INVALID", "CP d8", "RST 38H"
};

const char *cbOpNames[256] = {
    "RLC B", "RLC C", "RLC D", "RLC E", "RLC H", "RLC L", "RLC (HL)", "RLC A",
    "RRC B", "RRC C", "RRC D", "RRC E", "RRC H", "RRC L", "RRC (HL)", "RRC A",
    "RL B", "RL C", "RL D", "RL E", "RL H", "RL L", "RL (HL)", "RL A",
    "RR B", "RR C", "RR D", "RR E", "RR H", "RR L", "RR (HL)", "RR A",
    "SLA B", "SLA C", "SLA D", "SLA E", "SLA H", "SLA L", "SLA (HL)", "SLA A",
    "SRA B", "SRA C", "SRA D", "SRA E", "SRA H", "SRA L", "SRA (HL)", "SRA A",
    "SWAP B", "SWAP C", "SWAP D", "SWAP E", "SWAP H", "SWAP L", "SWAP (HL)", "SWAP A",
    "SRL B", "SRL C", "SRL D", "SRL E", "SRL H", "SRL L", "SRL (HL)", "SRL A",
    "BIT0 B", "BIT0 C", "BIT0 D", "BIT0 E", "BIT0 H", "BIT0 L", "BIT0 (HL)", "BIT0 A",
    "BIT1 B", "BIT1 C", "BIT1 D", "BIT1 E", "BIT1 H", "BIT1 L", "BIT1 (HL)", "BIT1 A",
    "BIT2 B", "BIT2 C", "BIT2 D", "BIT2 E", "BIT2 H", "BIT2 L", "BIT2 (HL)", "BIT2 A",
    "BIT3 B", "BIT3 C", "BIT3 D", "BIT3 E", "BIT3 H", "BIT3 L", "BIT3 (HL)", "BIT3 A",
    "BIT4 B", "BIT4 C", "BIT4 D", "BIT4 E", "BIT4 H", "BIT4 L", "BIT4 (HL)", "BIT4 A",
    "BIT5 B", "BIT5 C", "BIT5 D", "BIT5 E", "BIT5 H", "BIT5 L", "BIT5 (HL)", "BIT5 A",
    "BIT6 B", "BIT6 C", "BIT6 D", "BIT6 E", "BIT6 H", "BIT6 L", "BIT6 (HL)", "BIT6 A",
    "BIT7 B", "BIT7 C", "BIT7 D", "BIT7 E", "BIT7 H", "BIT7 L", "BIT7 (HL)", "BIT7 A",
    "RES0 B", "RES0 C", "RES0 D", "RES0 E", "RES0 H", "RES0 L", "RES0 (HL)", "RES0 A",
    "RES1 B", "RES1 C", "RES1 D", "RES1 E", "RES1 H", "RES1 L", "RES1 (HL)", "RES1 A",
    "RES2 B", "RES2 C", "RES2 D", "RES2 E", "RES2 H", "RES2 L", "RES2 (HL)", "RES2 A",
    "RES3 B", "RES3 C", "RES3 D", "RES3 E", "RES3 H", "RES3 L", "RES3 (HL)", "RES3 A",
    "RES4 B", "RES4 C", "RES4 D", "RES4 E", "RES4 H", "RES4 L", "RES4 (HL)", "RES4 A",
    "RES5 B", "RES5 C", "RES5 D", "RES5 E", "RES5 H", "RES5 L", "RES5 (HL)", "RES5 A",
    "RES6 B", "RES6 C", "RES6 D", "RES6 E", "RES6 H", "RES6 L", "RES6 (HL)", "RES6 A",
    "RES7 B", "RES7 C", "RES7 D", "RES7 E", "RES7 H", "RES7 L", "RES7 (HL)", "RES7 A",
    "SET0 B", "SET0 C", "SET0 D", "SET0 E", "SET0 H", "SET0 L", "SET0 (HL)", "SET0 A",
    "SET1 B", "SET1 C", "SET1 D", "SET1 E", "SET1 H", "SET1 L", "SET1 (HL)", "SET1 A",
    "SET2 B", "SET2 C", "SET2 D", "SET2 E", "SET2 H", "SET2 L", "SET2 (HL)", "SET2 A",
    "SET3 B", "SET3 C", "SET3 D", "SET3 E", "SET3 H", "SET3 L", "SET3 (HL)", "SET3 A",
    "SET4 B", "SET4 C", "SET4 D", "SET4 E", "SET4 H", "SET4 L", "SET4 (HL)", "SET4 A",
    "SET5 B", "SET5 C", "SET5 D", "SET5 E", "SET5 H", "SET5 L", "SET5 (HL)", "SET5 A",
    "SET6 B", "SET6 C", "SET6 D", "SET6 E", "SET6 H", "SET6 L", "SET6 (HL)", "SET6 A",
    "SET7 B", "SET7 C", "SET7 D", "SET7 E", "SET7 H", "SET7 L", "SET7 (HL)", "SET7 A",
};

const char *opName(uint8_t op)
{
    return opNames[op];
};

const char *cbOpName(uint8_t op)
{
    return cbOpNames[op];
};
