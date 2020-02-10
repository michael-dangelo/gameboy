#include "cpu.h"

#include "memory.h"

#include <stdint.h>
#include <stdio.h>

void dispatch(uint8_t op);

static struct 
{
    uint8_t a, b, c, d, e, h, l, f;
    uint16_t pc, sp;
} r;

void cpuStep()
{
    uint8_t op = getByte(r.pc++);
    //printf("%x ", op);
    dispatch(op);
}

void z(uint8_t v)
{
    if (v)
        r.f &= ~128;
    else
        r.f |= 128;
}

void LD_BC_D16() { r.b = getByte(r.pc++); r.c = getByte(r.pc++); }
void LD_B_D8() { r.b = getByte(r.pc++); }
void LD_IOBC_A() { writeByte(((uint16_t)r.b << 8) + r.c, r.a); }
void INC_BC() { if (!++r.c) ++r.b; }
void INC_B() { ++r.b; z(r.b); }
void DEC_B() { --r.b; z(r.b); }
void RLCA() {}
void NOP() {}

void dispatch(uint8_t op)
{
    printf("Dispatching op %x\n", op);
    switch(op)
    {
        case 0x00: NOP(); return;
        case 0x01: LD_BC_D16(); return;
        case 0x02: LD_IOBC_A(); return;
        case 0x03: INC_BC(); return;
        case 0x04: INC_B(); return;
        case 0x05: DEC_B(); return;
        case 0x06: LD_B_D8(); return;
        default: printf("Unknown instruction %x\n", op);
    }
    /*
        case 0x07: RLCA(); return;
        case 0x08: LD_A16_SP(); return;
        case 0x09: ADD_HL_BC(); return;
        case 0x0A: LD_A_BC(); return;
        case 0x0B: DEC_BC(); return;
        case 0x0C: INC_C(); return
        case 0x0D: DEC_C(); return;
        case 0x0E: LD_C_D8(); return;
        case 0x0F: RRCA(); return;
        case 0x10: STOP(); return;
        case 0x11: LD_DE_D16(); return;
        case 0x12: LD_DE_A(); return;
        case 0x13: INC_DE(); return;
        case 0x14: INC_D(); return;
        case 0x15: DEC_D(); return;
        case 0x16: LD_D_D8(); return;
        case 0x17: RLA(); return;
        case 0x18: JR_R8(); return;
        case 0x19: ADD_HL_DE(); return;
        case 0x1A: LD_A_DE(); return;
        case 0x1B: DEC_DE(); return;
        case 0x1C: INC_E(); return;
        case 0x1D: DEC_E(); return;
        case 0x1E: LD_E_D8(); return;
        case 0x1F: RRA(); return;
        case 0x20: JR_NZ_R8(); return;
        case 0x21: LD_HL_D16(); return;
        case 0x22: LD_HLPOS_A(); return;
        case 0x23: INC_HL(); return;
        case 0x24: INC_H(); return;
        case 0x25: DEC_H(); return;
        case 0x26: LD_H_D8(); return;
        case 0x27: DAA(); return;
        case 0x28: JR_Z_R8(); return;
        case 0x29: ADD_HL_HL(); return;
        case 0x2A: LD_A_HLPOS(); return;
        case 0x2B: DEC_HL(); return;
        case 0x2C: INC_L(); return;
        case 0x2D: DEC_L(); return;
        case 0x2E: LD_L_D8(); return;
        case 0x2F: CPL(); return;
        case 0x30: JR_NC_R8(); return;
        case 0x31: LD_SP_D16(); return;
        case 0x32: LD_HLNEG_A(); return;
        case 0x33: INC_SP(); return;
        case 0x34: INC_HL(); return;
        case 0x35: DEC_HL(); return;
        case 0x36: LD_HL_D8(); return;
        case 0x37: SCF(); return;
        case 0x38: JR_C_R8(); return;
        case 0x39: ADD_HL_SP(); return;
        case 0x3A: LD_A_HLNEG(); return;
        case 0x3B: DEC_SP(); return;
        case 0x3C: INC_A(); return;
        case 0x3D: DEC_A(); return;
        case 0x3E: LD_A_D8(); return;
        case 0x3F: CCF(); return;
        case 0x76: HALT(); return;
        case 0x40:
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x45:
        case 0x46:
        case 0x47:
        case 0x48:
        case 0x49:
        case 0x4A:
        case 0x4B:
        case 0x4C:
        case 0x4D:
        case 0x4E:
        case 0x4F:
        case 0x50:
        case 0x51:
        case 0x52:
        case 0x53:
        case 0x54:
        case 0x55:
        case 0x56:
        case 0x57:
        case 0x58:
        case 0x59:
        case 0x5A:
        case 0x5B:
        case 0x5C:
        case 0x5D:
        case 0x5E:
        case 0x5F:
        case 0x60:
        case 0x61:
        case 0x62:
        case 0x63:
        case 0x64:
        case 0x65:
        case 0x66:
        case 0x67:
        case 0x68:
        case 0x69:
        case 0x6A:
        case 0x6B:
        case 0x6C:
        case 0x6D:
        case 0x6E:
        case 0x6F:
        case 0x70:
        case 0x71:
        case 0x72:
        case 0x73:
        case 0x74:
        case 0x75:
        case 0x77:
        case 0x78:
        case 0x79:
        case 0x7A:
        case 0x7B:
        case 0x7C:
        case 0x7D:
        case 0x7E:
        case 0x7F: LD(op); return;
        case 0x80: 
        case 0x81:
        case 0x82:
        case 0x83:
        case 0x84:
        case 0x85:
        case 0x86:
        case 0x87: ADD(op); return;
        case 0x88:
        case 0x89:
        case 0x8A:
        case 0x8B:
        case 0x8C:
        case 0x8D:
        case 0x8E:
        case 0x8F: ADC(op); return;
        case 0x90: 
        case 0x91:
        case 0x92:
        case 0x93:
        case 0x94:
        case 0x95:
        case 0x96:
        case 0x97: SUB(op); return;
        case 0x98:
        case 0x99:
        case 0x9A:
        case 0x9B:
        case 0x9C:
        case 0x9D:
        case 0x9E:
        case 0x9F: SBC(op); return;
        case 0xA0: 
        case 0xA1:
        case 0xA2:
        case 0xA3:
        case 0xA4:
        case 0xA5:
        case 0xA6:
        case 0xA7: AND(op); return;
        case 0xA8:
        case 0xA9:
        case 0xAA:
        case 0xAB:
        case 0xAC:
        case 0xAD:
        case 0xAE:
        case 0xAF: XOR(op); return;
        case 0xB0: 
        case 0xB1:
        case 0xB2:
        case 0xB3:
        case 0xB4:
        case 0xB5:
        case 0xB6:
        case 0xB7: OR(op); return;
        case 0xB8:
        case 0xB9:
        case 0xBA:
        case 0xBB:
        case 0xBC:
        case 0xBD:
        case 0xBE:
        case 0xBF: CP(op); return;
        case 0xC0: RET_NZ(); return;
        case 0xC1: POP_BC(); return;
        case 0xC2: JP_NZ_A16(); return;
        case 0xC3: JP_A16(); return;
        case 0xC4: CALL_NZ_A16(); return;
        case 0xC5: PUSH_BC(); return;
        case 0xC6: ADD_A_D8(); return;
        case 0xC7: RST_00H(); return;
        case 0xC8: RET_Z(); return;
        case 0xC9: RET(); return;
        case 0xCA: JP_Z_A16(); return;
        case 0xCB: PREFIX_CB(); return;
        case 0xCC: CALL_Z_A16(); return;
        case 0xCD: CALL_A16(); return;
        case 0xCE: ADC_A_D8(); return;
        case 0xCF: RST_08H(); return;
        case 0xD0: RET_NC(); return;
        case 0xD1: POP_DE(); return;
        case 0xD2: JP_NC_A16(); return;
        case 0xD3: INVALID(op); return;
        case 0xD4: CALL_NC_A16(); return;
        case 0xD5: PUSH_DE(); return;
        case 0xD6: SUB_D8(); return;
        case 0xD7: RST_10H(); return;
        case 0xD8: RET_C(); return;
        case 0xD9: RETI(); return;
        case 0xDA: JP_C_A16(); return;
        case 0xDB: INVALID(op); return;
        case 0xDC: CALL_C_A16(); return;
        case 0xDD: INVALID(op); return;
        case 0xDE: SBC_A_D8(); return;
        case 0xDF: RST_18H(); return;
        case 0xE0: LDH_IOA8_A(); return;
        case 0xE1: POP_HL(); return;
        case 0xE2: LD_IOC_A(); return;
        case 0xE3: INVALID(op); return;
        case 0xE4: INVALID(op); return;
        case 0xE5: PUSH_HL(); return;
        case 0xE6: AND_D8(); return;
        case 0xE7: RST_20H(); return;
        case 0xE8: ADD_SP_R8(); return;
        case 0xE9: JP_HL(); return;
        case 0xEA: LD_IOA16_A(); return;
        case 0xEB: INVALID(op); return;
        case 0xEC: INVALID(op); return;
        case 0xED: INVALID(op); return;
        case 0xEE: XOR_D8(); return;
        case 0xEF: RST_28H(); return;
        case 0xF0: LDH_A_IOA8(); return;
        case 0xF1: POP_AF(); return;
        case 0xF2: LD_A_IOC(); return;
        case 0xF3: DI(); return;
        case 0xF4: INVALID(op); return;
        case 0xF5: PUSH_AF(); return;
        case 0xF6: OR_D8(); return;
        case 0xF7: RST_30H(); return;
        case 0xF8: LD_HL_SP_PLUS_R8(); return;
        case 0xF9: LD_SP_HL(); return;
        case 0xFA: LD_A_IOA16(); return;
        case 0xFB: EI(); return;
        case 0xFC: INVALID(op); return;
        case 0xFD: INVALID(op); return;
        case 0xFE: CP_D8(); return;
        case 0xFF: RST_38H(); return;
    }
    */
}

