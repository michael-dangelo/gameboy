#include "cpu.h"

#include "memory.h"

#include <stdint.h>
#include <stdio.h>

static struct 
{
    uint8_t a, b, c, d, e, h, l, f;
    uint16_t pc, sp;
} r;

void dispatch(uint8_t op);

void Cpu_step()
{
    dispatch(Mem_rb(r.pc++));
}

// 8-bit loads
void LD_rr(uint8_t *dest, uint8_t *src) { *dest = *src; }
void LD_rn(uint8_t *dest);
void LD_rHLm(uint8_t op);
void LD_HLmr(uint8_t op);
void LD_HLmn();
void LD_Arrm(uint8_t op);
void LD_Annm();
void LD_rrmA(uint8_t op);
void LD_nnmA();
void LD_AIOn();
void LD_IOnA();
void LD_AIOC();
void LD_IOCA();
void LDI_HLmA();
void LDI_AHLm();
void LDD_HLmA();
void LDD_AHLm();

// 16-bit loads
void LD_rrnn(uint8_t op);
void LD_SPHL();
void PUSH(uint8_t op);
void POP(uint8_t op);

// 8-bit arithmetic/logical
void ADD_r(uint8_t op);
void ADD_n();
void ADD_HLm();
void ADC_r(uint8_t op);
void ADC_n();
void ADC_HLm();
void SUB_r(uint8_t op);
void SUB_n();
void SUB_HLm();
void SBC_r(uint8_t op);
void SBC_n();
void SBC_HLm();
void AND_r(uint8_t op);
void AND_n();
void AND_HLm();
void XOR_r(uint8_t op);
void XOR_n();
void XOR_HLm();
void OR_r();
void OR_n();
void OR_HLm();
void CP_r();
void CP_n();
void CP_HLm();
void INC_r(uint8_t op);
void INC_HLm();
void DEC_r(uint8_t op);
void DEC_HLm();
void DAA();
void CPL();

// 16-bit arithmetic/logical
void ADD_HLrr(uint8_t op);
void INC_rr(uint8_t op);
void DEC_rr(uint8_t op);
void ADD_SPdd();
void LD_HLSPdd();

// Rotate/shift
void RLCA();
void RLA();
void RRCA();
void RRA();
void RLC_r(uint8_t op);
void RLC_HL();
void RL_r(uint8_t op);
void RL_HL();
void RRC_r(uint8_t op);
void RRC_HL();
void RR_r(uint8_t op);
void RR_HL();
void SLA_r(uint8_t op);
void SLA_HL();
void SWAP_r(uint8_t op);
void SWAP_HL();
void SRA_r(uint8_t op);
void SRA_HL();
void SRL_r(uint8_t op);
void SRL_HL();

// Single-bit
void BIT_nr(uint8_t op);
void BIT_nHL();
void SET_nr(uint8_t op);
void SET_N_HL(uint8_t op);
void RES_nr(uint8_t op);
void RES_nHL(uint8_t op);

// Control
void CCF();
void SCF();
void NOP() {}
void HALT();
void STOP();
void DI();
void EI();

// Jumps
void JP_nn();
void JP_HL();
void JP_fnn(uint8_t op);
void JP_PCdd();
void CALL_nn();
void CALL_fnn(uint8_t op);
void RET();
void RET_f(uint8_t op);
void RETI();
void RST_n();

void dispatch(uint8_t op)
{
    printf("op %02x", op);
    switch(op)
    {
        case 0x00: NOP(); break;
        case 0x40: LD_rr(&r.b, &r.b); break;
        case 0x41: LD_rr(&r.b, &r.c); break;
        case 0x42: LD_rr(&r.b, &r.d); break;
        case 0x43: LD_rr(&r.b, &r.e); break;
        case 0x44: LD_rr(&r.b, &r.h); break;
        case 0x45: LD_rr(&r.b, &r.l); break;
        case 0x47: LD_rr(&r.b, &r.a); break;
        case 0x48: LD_rr(&r.c, &r.b); break;
        case 0x49: LD_rr(&r.c, &r.c); break;
        case 0x4A: LD_rr(&r.c, &r.d); break;
        case 0x4B: LD_rr(&r.c, &r.e); break;
        case 0x4C: LD_rr(&r.c, &r.h); break;
        case 0x4D: LD_rr(&r.c, &r.l); break;
        case 0x4F: LD_rr(&r.c, &r.a); break;
        case 0x50: LD_rr(&r.d, &r.b); break;
        case 0x51: LD_rr(&r.d, &r.c); break;
        case 0x52: LD_rr(&r.d, &r.d); break;
        case 0x53: LD_rr(&r.d, &r.e); break;
        case 0x54: LD_rr(&r.d, &r.h); break;
        case 0x55: LD_rr(&r.d, &r.l); break;
        case 0x57: LD_rr(&r.d, &r.a); break;
        case 0x58: LD_rr(&r.e, &r.b); break;
        case 0x59: LD_rr(&r.e, &r.c); break;
        case 0x5A: LD_rr(&r.e, &r.d); break;
        case 0x5B: LD_rr(&r.e, &r.e); break;
        case 0x5C: LD_rr(&r.e, &r.h); break;
        case 0x5D: LD_rr(&r.e, &r.l); break;
        case 0x5F: LD_rr(&r.e, &r.a); break;
        case 0x60: LD_rr(&r.h, &r.b); break;
        case 0x61: LD_rr(&r.h, &r.c); break;
        case 0x62: LD_rr(&r.h, &r.d); break;
        case 0x63: LD_rr(&r.h, &r.e); break;
        case 0x64: LD_rr(&r.h, &r.h); break;
        case 0x65: LD_rr(&r.h, &r.l); break;
        case 0x67: LD_rr(&r.h, &r.a); break;
        case 0x68: LD_rr(&r.l, &r.b); break;
        case 0x69: LD_rr(&r.l, &r.c); break;
        case 0x6A: LD_rr(&r.l, &r.d); break;
        case 0x6B: LD_rr(&r.l, &r.e); break;
        case 0x6C: LD_rr(&r.l, &r.h); break;
        case 0x6D: LD_rr(&r.l, &r.l); break;
        case 0x6F: LD_rr(&r.l, &r.a); break;
        case 0x78: LD_rr(&r.a, &r.b); break;
        case 0x79: LD_rr(&r.a, &r.c); break;
        case 0x7A: LD_rr(&r.a, &r.d); break;
        case 0x7B: LD_rr(&r.a, &r.e); break;
        case 0x7C: LD_rr(&r.a, &r.h); break;
        case 0x7D: LD_rr(&r.a, &r.l); break;
        case 0x7F: LD_rr(&r.a, &r.a); break;
        default: printf(" - unknown"); break;
    }
    printf("\n");
}

