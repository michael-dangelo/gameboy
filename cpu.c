#include "cpu.h"

#include "memory.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static struct
{
    uint8_t a, b, c, d, e, h, l, f;
    uint16_t pc, sp;
} r;

// Helpers
uint16_t rr(uint8_t high, uint8_t low) { return ((uint16_t)high << 8) + low; }
uint16_t BC() { return rr(r.b, r.c); }
uint16_t DE() { return rr(r.d, r.e); }
uint16_t HL() { return rr(r.h, r.l); }
uint16_t AF() { return rr(r.a, r.f); }
uint8_t CY() { return (r.f >> 4) & 1; }
uint8_t ZF() { return (r.f >> 7) & 1; }
void setHL(uint16_t val) { r.h = val >> 8; r.l = val & 0xFF; }
void setFlag(uint8_t val, uint8_t pos) { if (val) r.f |= (1 << pos); else r.f &= ~(1 << pos); }
void setCY(uint8_t val) { setFlag(val, 4); }
void setZF(uint8_t val) { setFlag(val, 7); }

// Debugging
void printCpu()
{
    printf("a: %02x b: %02x c: %02x d: %02x e: %02x h: %02x l: %02x f: %02x pc: %04x sp: %04x zf: %x cy: %x\n",
        r.a, r.b, r.c, r.d, r.e, r.h, r.l, r.f, r.pc, r.sp, ZF(), CY());
}

uint16_t debugCmd()
{
    char debugCmdBuffer[10];
    fgets(debugCmdBuffer, 10, stdin);
    char *ptr;
    return strtoul(debugCmdBuffer, &ptr, 16);
}

void dispatch(uint8_t op);

static void step()
{
    printf("\n");
    uint8_t op = Mem_rb(r.pc++);
    printf("op %02x\n", op);
    dispatch(op);
    printCpu();
}

// Main cpu loop
void Cpu_step()
{
    uint16_t addr = debugCmd();
    if (addr > 0)
        while (r.pc != addr)
            step();
    else
        step();
}

// 8-bit loads
void LD_rr(uint8_t *dest, uint8_t src) { *dest = src; }
void LD_rn(uint8_t *dest) { *dest = Mem_rb(r.pc++); }
void LD_rHLm(uint8_t *dest) { *dest = Mem_rb(HL()); }
void LD_HLmr(uint8_t src) { Mem_wb(HL(), src); }
void LD_HLmn() { Mem_wb(HL(), Mem_rb(r.pc++)); }
void LD_Arrm(uint16_t addr) { r.a = Mem_rb(addr); }
void LD_Annm() { r.a = Mem_rb(Mem_rw(r.pc)); r.pc += 2; }
void LD_rrmA(uint16_t addr) { Mem_wb(addr, r.a); }
void LD_nnmA() { Mem_wb(Mem_rw(r.pc), r.a); r.pc += 2; }
void LD_AIOn() { r.a = Mem_rb(0xFF00 + Mem_rb(r.pc++)); }
void LD_IOnA() { Mem_wb(0xFF00 + Mem_rb(r.pc++), r.a); }
void LD_AIOC() { r.a = Mem_rb(0xFF00 + r.c); }
void LD_IOCA() { Mem_wb(0xFF00 + r.c, r.a); }
void LDI_HLmA() { Mem_wb(HL(), r.a); setHL(HL() + 1); }
void LDI_AHLm() { r.a = Mem_rb(HL()); setHL(HL() + 1); }
void LDD_HLmA() { Mem_wb(HL(), r.a); setHL(HL() - 1); }
void LDD_AHLm() { r.a = Mem_rb(HL()); setHL(HL() - 1); }

// 16-bit loads
void LD_rrnn(uint8_t *high, uint8_t *low) { *low = Mem_rb(r.pc++); *high = Mem_rb(r.pc++); }
void LD_SPnn() { r.sp = Mem_rw(r.pc); r.pc += 2; }
void LD_nnmSP() { Mem_ww(Mem_rw(r.pc), r.sp); r.pc += 2; }
void LD_SPHL() { r.sp = HL(); }
void LD_HLSPdd() { int8_t d = Mem_rb(r.pc++); setHL(r.sp + d); setZF(0); setCY(d > 0 ? r.sp - d > r.sp : r.sp + d < r.sp); }
void PUSH(uint16_t val) { r.sp -= 2; Mem_ww(r.sp, val); }
void POP(uint8_t *high, uint8_t *low) { uint16_t w = Mem_rw(r.sp); *low = w & 0xFF; *high = w >> 8; r.sp += 2; }

// 8-bit arithmetic/logical
void ADD_r(uint8_t src) { r.a += src; setZF(r.a == 0); setCY(r.a - src > r.a); }
void ADD_n() { uint8_t n = Mem_rb(r.pc++); r.a += n; setZF(r.a == 0); setCY(r.a - n > r.a); }
void ADD_HLm() { uint8_t n = Mem_rb(HL()); r.a += n; setZF(r.a == 0); setCY(r.a - n > r.a); }
void ADC_r(uint8_t src) { r.a += src + CY(); setZF(r.a == 0); setCY(r.a - src > r.a); }
void ADC_n() { uint8_t n = Mem_rb(r.pc++); r.a += n + CY(); setZF(r.a == 0); setCY(r.a - (n + CY()) > r.a); }
void ADC_HLm() { uint8_t n = Mem_rb(HL()); r.a += n + CY(); setZF(r.a == 0); setCY(r.a - (n + CY()) > r.a); }
void SUB_r(uint8_t src) { r.a -= src; setZF(r.a == 0); setCY(r.a + src < r.a); }
void SUB_n() { uint8_t n = Mem_rb(r.pc++); r.a -= n; setZF(r.a == 0); setCY(r.a + n < r.a); }
void SUB_HLm() { uint8_t n = Mem_rb(HL()); r.a -= n; setZF(r.a == 0); setCY(r.a + n < r.a); }
void SBC_r(uint8_t src) { r.a -= src + CY(); setZF(r.a == 0); setCY(r.a + src + CY() < r.a); }
void SBC_n() { uint8_t n = Mem_rb(r.pc++); r.a -= n + CY(); setZF(r.a == 0); setCY(r.a + n + CY() < r.a); }
void SBC_HLm() { uint8_t n = Mem_rb(HL()); r.a -= n + CY(); setZF(r.a == 0); setCY(r.a + n + CY() < r.a); }
void AND_r(uint8_t src) { r.a &= src; setZF(r.a == 0); setCY(0); }
void AND_n() { r.a &= Mem_rb(r.pc++); setZF(r.a == 0); setCY(0); }
void AND_HLm() { r.a &= Mem_rb(HL()); setZF(r.a == 0); setCY(0); }
void XOR_r(uint8_t src) { r.a ^= src; setZF(r.a == 0); setCY(0); }
void XOR_n() { r.a ^= Mem_rb(r.pc++); setZF(r.a == 0); setCY(0); }
void XOR_HLm() { r.a ^= Mem_rb(HL()); setZF(r.a == 0); setCY(0); }
void OR_r(uint8_t src) { r.a |= src; setZF(r.a == 0); setCY(0); }
void OR_n() { r.a |= Mem_rb(r.pc++); setZF(r.a == 0); setCY(0); }
void OR_HLm() { r.a |= Mem_rb(HL()); setZF(r.a == 0); setCY(0); }
void CP_r(uint8_t src) { uint8_t n = r.a - src; setZF(n == 0); setCY(r.a - src > r.a); }
void CP_n() { uint8_t n = Mem_rb(r.pc++); setZF(r.a - n == 0); setCY(r.a + n > r.a); }
void CP_HLm() { uint8_t n = Mem_rb(HL()); setZF(r.a - n == 0); setCY(r.a + n > r.a); }
void INC_r(uint8_t *src) { (*src)++; setZF(*src); }
void INC_HLm() { Mem_wb(HL(), Mem_rb(HL()) + 1); setZF(Mem_rb(HL()) == 0); }
void DEC_r(uint8_t *src) { (*src)++; setZF(*src); }
void DEC_HLm() { Mem_wb(HL(), Mem_rb(HL()) - 1); setZF(Mem_rb(HL()) == 0); }
void DAA() { assert(0); }
void CPL() { r.a ^= 0xFF; }
void SCF() { setCY(1); }
void CCF() { setCY(CY() ^ 1); }

// 16-bit arithmetic/logical
void ADD_HLrr(uint16_t src) { setHL(HL() + src); setCY(HL() - src > HL()); }
void INC_rr(uint8_t *high, uint8_t *low) { (*low)++; if (!*low) (*high)++; }
void INC_SP() { r.sp++; }
void DEC_rr(uint8_t *high, uint8_t *low) { (*high)--; if (*high == INT8_MAX) (*low)--; }
void DEC_SP() { r.sp--; }
void ADD_SPdd() { int8_t d = Mem_rb(r.pc++); r.sp += d; setZF(0); setCY(d > 0 ? r.sp - d > r.sp : r.sp + d < r.sp); }

// Rotate/shift
void RLCA() { uint8_t v = (r.a >> 7) & 1; r.a <<= 1; r.a |= v; setZF(0); setCY(v == 1); }
void RLC_r(uint8_t *reg) { uint8_t v = (*reg >> 7) & 1; *reg <<= 1; *reg |= v; setZF(*reg == 0); setCY(v == 1); }
void RLC_HLm() { uint8_t m = Mem_rb(HL()); uint8_t v = (m >> 7) & 1; m <<= 1; m |= v; setHL(m); setZF(Mem_rb(HL()) == 0); setCY(v == 1); }
void RLA() { uint8_t v = CY(); setCY((r.a >> 7) & 1); r.a <<= 1; r.a |= v; setZF(0); setCY(v == 1); }
void RL_r(uint8_t *reg) { uint8_t v = CY(); setCY((*reg >> 7) & 1); *reg <<= 1; *reg |= v; setZF(*reg == 0); setCY(v == 1); }
void RL_HLm() { uint8_t m = Mem_rb(HL()); uint8_t v = CY(); setCY((m >> 7) & 1); m <<= 1; m |= v; setHL(m); setZF(Mem_rb(HL()) == 0); setCY(v == 1); }
void RRCA() { uint8_t v = r.a & 1; r.a >>= 1; r.a |= v << 7; setZF(0); setCY(v == 1); }
void RRC_r(uint8_t *reg) { uint8_t v = *reg & 1; *reg >>= 1; *reg |= v << 7; setZF(*reg == 0); setCY(v == 1); }
void RRC_HLm() { uint8_t m = Mem_rb(HL()); uint8_t v = m & 1; m >>= 1; m |= v << 7; setHL(m); setZF(Mem_rb(HL()) == 0); setCY(v == 1); }
void RRA() { uint8_t v = CY(); setCY(r.a & 1); r.a >>= 1; r.a |= v << 7; setZF(0); setCY(v == 1); }
void RR_r(uint8_t *reg) { uint8_t v = CY(); setCY(*reg & 1); *reg >>= 1; *reg |= v << 7; setZF(*reg == 0); setCY(v == 1); }
void RR_HLm() { uint8_t m = Mem_rb(HL()); uint8_t v = CY(); setCY(m & 1); m >>= 1; m |= v << 7; setHL(m); setZF(Mem_rb(HL()) == 0); setCY(v == 1); }
void SLA_r(uint8_t *reg) { uint8_t v = *reg & (1 << 7); *reg <<= 1; setZF(*reg == 0); setCY(v == 1); }
void SLA_HLm() { uint8_t v = HL() & (1 << 7); setHL(HL() << 1); setZF(Mem_rb(HL()) == 0); setCY(v == 1); }
void SRA_r(uint8_t *reg) { uint8_t v = *reg & (1 << 7); *reg >>= 1; *reg |= v; setZF(*reg == 0); setCY(0);}
void SRA_HLm() { uint8_t r = Mem_rb(HL()); uint8_t v = r & (1 << 7); r >>= 1; r |= v; setHL(r); setZF(Mem_rb(HL()) == 0); setCY(0); }
void SWAP_r(uint8_t *reg) { uint8_t v = *reg; *reg >>= 4; *reg += v << 4; setZF(*reg == 0); setCY(0); }
void SWAP_HLm() { uint8_t r = Mem_rb(HL()); uint8_t v = r; r >>= 4; r += v << 4; setHL(r); setZF(Mem_rb(HL()) == 0); setCY(0); }
void SRL_r(uint8_t *reg) { uint8_t v = *reg & 1; *reg >>= 1; setZF(*reg == 0); setCY(v == 1); }
void SRL_HLm() { uint8_t v = Mem_rb(HL()) & 1; setHL(Mem_rb(HL()) >> 1); setZF(Mem_rb(HL()) == 0); setCY(v == 1); }

// Single-bit
void BIT_nr(uint8_t n, uint8_t *reg) { setZF(!((*reg >> n) & 1)); }
void BIT_nHLm(uint8_t n) { setZF(!((Mem_rb(HL()) >> n) & 1)); }
void SET_nr(uint8_t n, uint8_t *reg) { *reg |= 1 << n; }
void SET_nHLm(uint8_t n) { setHL(HL() | 1 << n); }
void RES_nr(uint8_t n, uint8_t *reg) { *reg &= ~(1 << n); }
void RES_nHLm(uint8_t n) { setHL(HL() & ~(1 << n)); }

// Control
void NOP() {}
void HALT() { assert(0); }
void STOP() { assert(0); }
void DI() { printf("DI instruction, not sure what do\n"); }
void EI() { printf("EI instruction, not sure what do\n"); }

// Jumps
void JP_nn() { r.pc = Mem_rw(r.pc); }
void JP_HL() { r.pc = HL(); }
void JPNZ_nn() { uint16_t n = Mem_rw(r.pc++); if (!ZF()) r.pc = n; }
void JPZ_nn() { uint16_t n = Mem_rw(r.pc++); if (ZF()) r.pc = n; }
void JPNC_nn() { uint16_t n = Mem_rw(r.pc++); if (!CY()) r.pc = n; }
void JPC_nn() { uint16_t n = Mem_rw(r.pc++); if (CY()) r.pc = n; }
void JR_dd() { int8_t n = Mem_rb(r.pc++); r.pc += n; }
void JRNZ_dd() { int8_t n = Mem_rb(r.pc++); if (!ZF()) r.pc += n; }
void JRZ_dd() { int8_t n = Mem_rb(r.pc++); if (ZF()) r.pc += n; }
void JRNC_dd() { int8_t n = Mem_rb(r.pc++); if (!CY()) r.pc += n; }
void JRC_dd() { int8_t n = Mem_rb(r.pc++); if (CY()) r.pc += n; }
void CALL(uint16_t addr) { r.sp -= 2; Mem_ww(r.sp, r.pc); r.pc = addr; }
void CALL_nn() { uint16_t n = Mem_rw(r.pc); r.pc += 2; CALL(n); }
void CALLNZ_nn() { if (!ZF()) CALL_nn(); else r.pc += 2; }
void CALLZ_nn() { if (ZF()) CALL_nn(); else r.pc += 2; }
void CALLNC_nn() { if (!CY()) CALL_nn(); else r.pc += 2; }
void CALLC_nn() { if (CY()) CALL_nn(); else r.pc += 2; }
void RET() { r.pc = Mem_rw(r.sp); r.sp -= 2; }
void RETNZ() { if (!ZF()) RET(); }
void RETZ() { if (ZF()) RET(); }
void RETNC() { if (!CY()) RET(); }
void RETC() { if (CY()) RET(); }
void RETI() { RET(); EI(); }
void RST_n(uint8_t n) { CALL(n); }

void CB_PREFIX()
{
    uint8_t op = Mem_rb(r.pc++);
    printf("cb op %02x\n" , op);
    switch(op)
    {
        case 0x00: RLC_r(&r.b); break;
        case 0x01: RLC_r(&r.c); break;
        case 0x02: RLC_r(&r.d); break;
        case 0x03: RLC_r(&r.e); break;
        case 0x04: RLC_r(&r.h); break;
        case 0x05: RLC_r(&r.l); break;
        case 0x06: RLC_HLm(); break;
        case 0x07: RLCA(); setZF(r.a == 0); break;
        case 0x08: RRC_r(&r.b); break;
        case 0x09: RRC_r(&r.c); break;
        case 0x0A: RRC_r(&r.d); break;
        case 0x0B: RRC_r(&r.e); break;
        case 0x0C: RRC_r(&r.h); break;
        case 0x0D: RRC_r(&r.l); break;
        case 0x0E: RRC_HLm(); break;
        case 0x0F: RRCA(); setZF(r.a == 0); break;
        case 0x10: RL_r(&r.b); break;
        case 0x11: RL_r(&r.c); break;
        case 0x12: RL_r(&r.d); break;
        case 0x13: RL_r(&r.e); break;
        case 0x14: RL_r(&r.h); break;
        case 0x15: RL_r(&r.l); break;
        case 0x16: RL_HLm(); break;
        case 0x17: RLA(); setZF(r.a == 0); break;
        case 0x18: RR_r(&r.b); break;
        case 0x19: RR_r(&r.c); break;
        case 0x1A: RR_r(&r.d); break;
        case 0x1B: RR_r(&r.e); break;
        case 0x1C: RR_r(&r.h); break;
        case 0x1D: RR_r(&r.l); break;
        case 0x1E: RR_HLm(); break;
        case 0x1F: RRA(); setZF(r.a == 0); break;
        case 0x20: SLA_r(&r.b); break;
        case 0x21: SLA_r(&r.c); break;
        case 0x22: SLA_r(&r.d); break;
        case 0x23: SLA_r(&r.e); break;
        case 0x24: SLA_r(&r.h); break;
        case 0x25: SLA_r(&r.l); break;
        case 0x26: SLA_HLm(); break;
        case 0x27: SRA_r(&r.a); break;
        case 0x28: SRA_r(&r.b); break;
        case 0x29: SRA_r(&r.c); break;
        case 0x2A: SRA_r(&r.d); break;
        case 0x2B: SRA_r(&r.e); break;
        case 0x2C: SRA_r(&r.h); break;
        case 0x2D: SRA_r(&r.l); break;
        case 0x2E: SRA_HLm(); break;
        case 0x2F: SRA_r(&r.a); break;
        case 0x30: SWAP_r(&r.b); break;
        case 0x31: SWAP_r(&r.c); break;
        case 0x32: SWAP_r(&r.d); break;
        case 0x33: SWAP_r(&r.e); break;
        case 0x34: SWAP_r(&r.h); break;
        case 0x35: SWAP_r(&r.l); break;
        case 0x36: SWAP_HLm(); break;
        case 0x37: SWAP_r(&r.a); break;
        case 0x38: SRL_r(&r.b); break;
        case 0x39: SRL_r(&r.c); break;
        case 0x3A: SRL_r(&r.d); break;
        case 0x3B: SRL_r(&r.e); break;
        case 0x3C: SRL_r(&r.h); break;
        case 0x3D: SRL_r(&r.l); break;
        case 0x3E: SRL_HLm(); break;
        case 0x3F: SRL_r(&r.a); break;
        case 0x40: BIT_nr(0, &r.b); break;
        case 0x41: BIT_nr(0, &r.c); break;
        case 0x42: BIT_nr(0, &r.d); break;
        case 0x43: BIT_nr(0, &r.e); break;
        case 0x44: BIT_nr(0, &r.h); break;
        case 0x45: BIT_nr(0, &r.l); break;
        case 0x46: BIT_nHLm(0); break;
        case 0x47: BIT_nr(0, &r.a); break;
        case 0x48: BIT_nr(1, &r.b); break;
        case 0x49: BIT_nr(1, &r.c); break;
        case 0x4A: BIT_nr(1, &r.d); break;
        case 0x4B: BIT_nr(1, &r.e); break;
        case 0x4C: BIT_nr(1, &r.h); break;
        case 0x4D: BIT_nr(1, &r.l); break;
        case 0x4E: BIT_nHLm(1); break;
        case 0x4F: BIT_nr(1, &r.a); break;
        case 0x50: BIT_nr(2, &r.b); break;
        case 0x51: BIT_nr(2, &r.c); break;
        case 0x52: BIT_nr(2, &r.d); break;
        case 0x53: BIT_nr(2, &r.e); break;
        case 0x54: BIT_nr(2, &r.h); break;
        case 0x55: BIT_nr(2, &r.l); break;
        case 0x56: BIT_nHLm(2); break;
        case 0x57: BIT_nr(2, &r.a); break;
        case 0x58: BIT_nr(3, &r.b); break;
        case 0x59: BIT_nr(3, &r.c); break;
        case 0x5A: BIT_nr(3, &r.d); break;
        case 0x5B: BIT_nr(3, &r.e); break;
        case 0x5C: BIT_nr(3, &r.h); break;
        case 0x5D: BIT_nr(3, &r.l); break;
        case 0x5E: BIT_nHLm(3); break;
        case 0x5F: BIT_nr(3, &r.a); break;
        case 0x60: BIT_nr(4, &r.b); break;
        case 0x61: BIT_nr(4, &r.c); break;
        case 0x62: BIT_nr(4, &r.d); break;
        case 0x63: BIT_nr(4, &r.e); break;
        case 0x64: BIT_nr(4, &r.h); break;
        case 0x65: BIT_nr(4, &r.l); break;
        case 0x66: BIT_nHLm(4); break;
        case 0x67: BIT_nr(4, &r.a); break;
        case 0x68: BIT_nr(5, &r.b); break;
        case 0x69: BIT_nr(5, &r.c); break;
        case 0x6A: BIT_nr(5, &r.d); break;
        case 0x6B: BIT_nr(5, &r.e); break;
        case 0x6C: BIT_nr(5, &r.h); break;
        case 0x6D: BIT_nr(5, &r.l); break;
        case 0x6E: BIT_nHLm(5); break;
        case 0x6F: BIT_nr(5, &r.a); break;
        case 0x70: BIT_nr(6, &r.b); break;
        case 0x71: BIT_nr(6, &r.c); break;
        case 0x72: BIT_nr(6, &r.d); break;
        case 0x73: BIT_nr(6, &r.e); break;
        case 0x74: BIT_nr(6, &r.h); break;
        case 0x75: BIT_nr(6, &r.l); break;
        case 0x76: BIT_nHLm(6); break;
        case 0x77: BIT_nr(6, &r.a); break;
        case 0x78: BIT_nr(7, &r.b); break;
        case 0x79: BIT_nr(7, &r.c); break;
        case 0x7A: BIT_nr(7, &r.d); break;
        case 0x7B: BIT_nr(7, &r.e); break;
        case 0x7C: BIT_nr(7, &r.h); break;
        case 0x7D: BIT_nr(7, &r.l); break;
        case 0x7E: BIT_nHLm(7); break;
        case 0x7F: BIT_nr(7, &r.a); break;
        case 0x80: RES_nr(0, &r.b); break;
        case 0x81: RES_nr(0, &r.c); break;
        case 0x82: RES_nr(0, &r.d); break;
        case 0x83: RES_nr(0, &r.e); break;
        case 0x84: RES_nr(0, &r.h); break;
        case 0x85: RES_nr(0, &r.l); break;
        case 0x86: RES_nHLm(0); break;
        case 0x87: RES_nr(0, &r.a); break;
        case 0x88: RES_nr(1, &r.b); break;
        case 0x89: RES_nr(1, &r.c); break;
        case 0x8A: RES_nr(1, &r.d); break;
        case 0x8B: RES_nr(1, &r.e); break;
        case 0x8C: RES_nr(1, &r.h); break;
        case 0x8D: RES_nr(1, &r.l); break;
        case 0x8E: RES_nHLm(1); break;
        case 0x8F: RES_nr(1, &r.a); break;
        case 0x90: RES_nr(2, &r.b); break;
        case 0x91: RES_nr(2, &r.c); break;
        case 0x92: RES_nr(2, &r.d); break;
        case 0x93: RES_nr(2, &r.e); break;
        case 0x94: RES_nr(2, &r.h); break;
        case 0x95: RES_nr(2, &r.l); break;
        case 0x96: RES_nHLm(2); break;
        case 0x97: RES_nr(2, &r.a); break;
        case 0x98: RES_nr(3, &r.b); break;
        case 0x99: RES_nr(3, &r.c); break;
        case 0x9A: RES_nr(3, &r.d); break;
        case 0x9B: RES_nr(3, &r.e); break;
        case 0x9C: RES_nr(3, &r.h); break;
        case 0x9D: RES_nr(3, &r.l); break;
        case 0x9E: RES_nHLm(3); break;
        case 0x9F: RES_nr(3, &r.a); break;
        case 0xA0: RES_nr(4, &r.b); break;
        case 0xA1: RES_nr(4, &r.c); break;
        case 0xA2: RES_nr(4, &r.d); break;
        case 0xA3: RES_nr(4, &r.e); break;
        case 0xA4: RES_nr(4, &r.h); break;
        case 0xA5: RES_nr(4, &r.l); break;
        case 0xA6: RES_nHLm(4); break;
        case 0xA7: RES_nr(4, &r.a); break;
        case 0xA8: RES_nr(5, &r.b); break;
        case 0xA9: RES_nr(5, &r.c); break;
        case 0xAA: RES_nr(5, &r.d); break;
        case 0xAB: RES_nr(5, &r.e); break;
        case 0xAC: RES_nr(5, &r.h); break;
        case 0xAD: RES_nr(5, &r.l); break;
        case 0xAE: RES_nHLm(5); break;
        case 0xAF: RES_nr(5, &r.a); break;
        case 0xB0: RES_nr(6, &r.b); break;
        case 0xB1: RES_nr(6, &r.c); break;
        case 0xB2: RES_nr(6, &r.d); break;
        case 0xB3: RES_nr(6, &r.e); break;
        case 0xB4: RES_nr(6, &r.h); break;
        case 0xB5: RES_nr(6, &r.l); break;
        case 0xB6: RES_nHLm(6); break;
        case 0xB7: RES_nr(6, &r.a); break;
        case 0xB8: RES_nr(7, &r.b); break;
        case 0xB9: RES_nr(7, &r.c); break;
        case 0xBA: RES_nr(7, &r.d); break;
        case 0xBB: RES_nr(7, &r.e); break;
        case 0xBC: RES_nr(7, &r.h); break;
        case 0xBD: RES_nr(7, &r.l); break;
        case 0xBE: RES_nHLm(7); break;
        case 0xBF: RES_nr(7, &r.a); break;
        case 0xC0: SET_nr(0, &r.b); break;
        case 0xC1: SET_nr(0, &r.c); break;
        case 0xC2: SET_nr(0, &r.d); break;
        case 0xC3: SET_nr(0, &r.e); break;
        case 0xC4: SET_nr(0, &r.h); break;
        case 0xC5: SET_nr(0, &r.l); break;
        case 0xC6: SET_nHLm(0); break;
        case 0xC7: SET_nr(0, &r.a); break;
        case 0xC8: SET_nr(1, &r.b); break;
        case 0xC9: SET_nr(1, &r.c); break;
        case 0xCA: SET_nr(1, &r.d); break;
        case 0xCB: SET_nr(1, &r.e); break;
        case 0xCC: SET_nr(1, &r.h); break;
        case 0xCD: SET_nr(1, &r.l); break;
        case 0xCE: SET_nHLm(1); break;
        case 0xCF: SET_nr(1, &r.a); break;
        case 0xD0: SET_nr(2, &r.b); break;
        case 0xD1: SET_nr(2, &r.c); break;
        case 0xD2: SET_nr(2, &r.d); break;
        case 0xD3: SET_nr(2, &r.e); break;
        case 0xD4: SET_nr(2, &r.h); break;
        case 0xD5: SET_nr(2, &r.l); break;
        case 0xD6: SET_nHLm(2); break;
        case 0xD7: SET_nr(2, &r.a); break;
        case 0xD8: SET_nr(3, &r.b); break;
        case 0xD9: SET_nr(3, &r.c); break;
        case 0xDA: SET_nr(3, &r.d); break;
        case 0xDB: SET_nr(3, &r.e); break;
        case 0xDC: SET_nr(3, &r.h); break;
        case 0xDD: SET_nr(3, &r.l); break;
        case 0xDE: SET_nHLm(3); break;
        case 0xDF: SET_nr(3, &r.a); break;
        case 0xE0: SET_nr(4, &r.b); break;
        case 0xE1: SET_nr(4, &r.c); break;
        case 0xE2: SET_nr(4, &r.d); break;
        case 0xE3: SET_nr(4, &r.e); break;
        case 0xE4: SET_nr(4, &r.h); break;
        case 0xE5: SET_nr(4, &r.l); break;
        case 0xE6: SET_nHLm(4); break;
        case 0xE7: SET_nr(4, &r.a); break;
        case 0xE8: SET_nr(5, &r.b); break;
        case 0xE9: SET_nr(5, &r.c); break;
        case 0xEA: SET_nr(5, &r.d); break;
        case 0xEB: SET_nr(5, &r.e); break;
        case 0xEC: SET_nr(5, &r.h); break;
        case 0xED: SET_nr(5, &r.l); break;
        case 0xEE: SET_nHLm(5); break;
        case 0xEF: SET_nr(5, &r.a); break;
        case 0xF0: SET_nr(5, &r.b); break;
        case 0xF1: SET_nr(5, &r.c); break;
        case 0xF2: SET_nr(5, &r.d); break;
        case 0xF3: SET_nr(5, &r.e); break;
        case 0xF4: SET_nr(5, &r.h); break;
        case 0xF5: SET_nr(5, &r.l); break;
        case 0xF6: SET_nHLm(5); break;
        case 0xF7: SET_nr(5, &r.a); break;
        case 0xF8: SET_nr(6, &r.b); break;
        case 0xF9: SET_nr(6, &r.c); break;
        case 0xFA: SET_nr(6, &r.d); break;
        case 0xFB: SET_nr(6, &r.e); break;
        case 0xFC: SET_nr(6, &r.h); break;
        case 0xFD: SET_nr(6, &r.l); break;
        case 0xFE: SET_nHLm(6); break;
        case 0xFF: SET_nr(6, &r.a); break;
        default: printf("UNKNOWN CB OP %02x\n", op); assert(0);
    }
}

void dispatch(uint8_t op)
{
    switch(op)
    {
        case 0x00: NOP(); break;
        case 0x01: LD_rrnn(&r.b, &r.c); break;
        case 0x02: LD_rrmA(BC()); break;
        case 0x03: INC_rr(&r.b, &r.c); break;
        case 0x04: INC_r(&r.b); break;
        case 0x05: DEC_r(&r.b); break;
        case 0x06: LD_rn(&r.b); break;
        case 0x07: RLC_r(&r.a); break;
        case 0x08: LD_nnmSP(); break;
        case 0x09: ADD_HLrr(BC()); break;
        case 0x0A: LD_Arrm(BC()); break;
        case 0x0B: DEC_rr(&r.b, &r.c); break;
        case 0x0C: INC_r(&r.c); break;
        case 0x0D: DEC_r(&r.c); break;
        case 0x0E: LD_rn(&r.c); break;
        case 0x0F: RRC_r(&r.a); break;
        case 0x10: Mem_rb(r.pc++); STOP(); break;
        case 0x11: LD_rrnn(&r.d, &r.e); break;
        case 0x12: LD_rrmA(DE()); break;
        case 0x13: INC_rr(&r.d, &r.e); break;
        case 0x14: INC_r(&r.d); break;
        case 0x15: DEC_r(&r.d); break;
        case 0x16: LD_rn(&r.d); break;
        case 0x17: RL_r(&r.a); break;
        case 0x18: JR_dd(); break;
        case 0x19: ADD_HLrr(DE()); break;
        case 0x1A: LD_Arrm(DE()); break;
        case 0x1B: DEC_rr(&r.d, &r.e); break;
        case 0x1C: INC_r(&r.e); break;
        case 0x1D: DEC_r(&r.e); break;
        case 0x1E: LD_rn(&r.e); break;
        case 0x1F: RR_r(&r.a); break;
        case 0x20: JRNZ_dd(); break;
        case 0x21: LD_rrnn(&r.h, &r.l); break;
        case 0x22: LDI_HLmA(); break;
        case 0x23: INC_rr(&r.h, &r.l); break;
        case 0x24: INC_r(&r.h); break;
        case 0x25: DEC_r(&r.h); break;
        case 0x26: LD_rn(&r.h); break;
        case 0x27: DAA(); break;
        case 0x28: JRZ_dd(); break;
        case 0x29: ADD_HLrr(HL()); break;
        case 0x2A: LDI_AHLm(); break;
        case 0x2B: DEC_rr(&r.h, &r.l); break;
        case 0x2C: INC_r(&r.l); break;
        case 0x2D: DEC_r(&r.l); break;
        case 0x2E: LD_rn(&r.l); break;
        case 0x2F: CPL(); break;
        case 0x30: JRNC_dd(); break;
        case 0x31: LD_SPnn(); break;
        case 0x32: LDD_HLmA(); break;
        case 0x33: INC_SP(); break;
        case 0x34: INC_HLm(); break;
        case 0x35: DEC_HLm(); break;
        case 0x36: LD_HLmn(); break;
        case 0x37: SCF(); break;
        case 0x38: JRC_dd(); break;
        case 0x39: ADD_HLrr(r.sp); break;
        case 0x3A: LDD_AHLm(); break;
        case 0x3B: DEC_SP(); break;
        case 0x3C: INC_r(&r.a); break;
        case 0x3D: DEC_r(&r.a); break;
        case 0x3E: LD_rn(&r.a); break;
        case 0x3F: CCF(); break;
        case 0x40: LD_rr(&r.b, r.b); break;
        case 0x41: LD_rr(&r.b, r.c); break;
        case 0x42: LD_rr(&r.b, r.d); break;
        case 0x43: LD_rr(&r.b, r.e); break;
        case 0x44: LD_rr(&r.b, r.h); break;
        case 0x45: LD_rr(&r.b, r.l); break;
        case 0x46: LD_rHLm(&r.b); break;
        case 0x47: LD_rr(&r.b, r.a); break;
        case 0x48: LD_rr(&r.c, r.b); break;
        case 0x49: LD_rr(&r.c, r.c); break;
        case 0x4A: LD_rr(&r.c, r.d); break;
        case 0x4B: LD_rr(&r.c, r.e); break;
        case 0x4C: LD_rr(&r.c, r.h); break;
        case 0x4D: LD_rr(&r.c, r.l); break;
        case 0x4E: LD_rHLm(&r.c); break;
        case 0x4F: LD_rr(&r.c, r.a); break;
        case 0x50: LD_rr(&r.d, r.b); break;
        case 0x51: LD_rr(&r.d, r.c); break;
        case 0x52: LD_rr(&r.d, r.d); break;
        case 0x53: LD_rr(&r.d, r.e); break;
        case 0x54: LD_rr(&r.d, r.h); break;
        case 0x55: LD_rr(&r.d, r.l); break;
        case 0x56: LD_rHLm(&r.d); break;
        case 0x57: LD_rr(&r.d, r.a); break;
        case 0x58: LD_rr(&r.e, r.b); break;
        case 0x59: LD_rr(&r.e, r.c); break;
        case 0x5A: LD_rr(&r.e, r.d); break;
        case 0x5B: LD_rr(&r.e, r.e); break;
        case 0x5C: LD_rr(&r.e, r.h); break;
        case 0x5D: LD_rr(&r.e, r.l); break;
        case 0x5E: LD_rHLm(&r.e); break;
        case 0x5F: LD_rr(&r.e, r.a); break;
        case 0x60: LD_rr(&r.h, r.b); break;
        case 0x61: LD_rr(&r.h, r.c); break;
        case 0x62: LD_rr(&r.h, r.d); break;
        case 0x63: LD_rr(&r.h, r.e); break;
        case 0x64: LD_rr(&r.h, r.h); break;
        case 0x65: LD_rr(&r.h, r.l); break;
        case 0x66: LD_rHLm(&r.h); break;
        case 0x67: LD_rr(&r.h, r.a); break;
        case 0x68: LD_rr(&r.l, r.b); break;
        case 0x69: LD_rr(&r.l, r.c); break;
        case 0x6A: LD_rr(&r.l, r.d); break;
        case 0x6B: LD_rr(&r.l, r.e); break;
        case 0x6C: LD_rr(&r.l, r.h); break;
        case 0x6D: LD_rr(&r.l, r.l); break;
        case 0x6E: LD_rHLm(&r.l); break;
        case 0x6F: LD_rr(&r.l, r.a); break;
        case 0x70: LD_HLmr(r.b); break;
        case 0x71: LD_HLmr(r.c); break;
        case 0x72: LD_HLmr(r.d); break;
        case 0x73: LD_HLmr(r.e); break;
        case 0x74: LD_HLmr(r.h); break;
        case 0x75: LD_HLmr(r.l); break;
        case 0x76: HALT(); break;
        case 0x77: LD_HLmr(r.a); break;
        case 0x78: LD_rr(&r.a, r.b); break;
        case 0x79: LD_rr(&r.a, r.c); break;
        case 0x7A: LD_rr(&r.a, r.d); break;
        case 0x7B: LD_rr(&r.a, r.e); break;
        case 0x7C: LD_rr(&r.a, r.h); break;
        case 0x7D: LD_rr(&r.a, r.l); break;
        case 0x7E: LD_rHLm(&r.a); break;
        case 0x7F: LD_rr(&r.a, r.a); break;
        case 0x80: ADD_r(r.b); break;
        case 0x81: ADD_r(r.c); break;
        case 0x82: ADD_r(r.d); break;
        case 0x83: ADD_r(r.e); break;
        case 0x84: ADD_r(r.h); break;
        case 0x85: ADD_r(r.l); break;
        case 0x86: ADD_HLm(); break;
        case 0x87: ADD_r(r.a); break;
        case 0x88: ADC_r(r.b); break;
        case 0x89: ADC_r(r.c); break;
        case 0x8A: ADC_r(r.d); break;
        case 0x8B: ADC_r(r.e); break;
        case 0x8C: ADC_r(r.h); break;
        case 0x8D: ADC_r(r.l); break;
        case 0x8E: ADC_HLm(); break;
        case 0x8F: ADC_r(r.a); break;
        case 0x90: SUB_r(r.b); break;
        case 0x91: SUB_r(r.c); break;
        case 0x92: SUB_r(r.d); break;
        case 0x93: SUB_r(r.e); break;
        case 0x94: SUB_r(r.h); break;
        case 0x95: SUB_r(r.l); break;
        case 0x96: SUB_HLm(); break;
        case 0x97: SUB_r(r.a); break;
        case 0x98: SBC_r(r.b); break;
        case 0x99: SBC_r(r.c); break;
        case 0x9A: SBC_r(r.d); break;
        case 0x9B: SBC_r(r.e); break;
        case 0x9C: SBC_r(r.h); break;
        case 0x9D: SBC_r(r.l); break;
        case 0x9E: SBC_HLm(); break;
        case 0x9F: SBC_r(r.a); break;
        case 0xA0: AND_r(r.b); break;
        case 0xA1: AND_r(r.c); break;
        case 0xA2: AND_r(r.d); break;
        case 0xA3: AND_r(r.e); break;
        case 0xA4: AND_r(r.h); break;
        case 0xA5: AND_r(r.l); break;
        case 0xA6: AND_HLm(); break;
        case 0xA7: AND_r(r.a); break;
        case 0xA8: XOR_r(r.b); break;
        case 0xA9: XOR_r(r.c); break;
        case 0xAA: XOR_r(r.d); break;
        case 0xAB: XOR_r(r.e); break;
        case 0xAC: XOR_r(r.h); break;
        case 0xAD: XOR_r(r.l); break;
        case 0xAE: XOR_HLm(); break;
        case 0xAF: XOR_r(r.a); break;
        case 0xB0: OR_r(r.b); break;
        case 0xB1: OR_r(r.c); break;
        case 0xB2: OR_r(r.d); break;
        case 0xB3: OR_r(r.e); break;
        case 0xB4: OR_r(r.h); break;
        case 0xB5: OR_r(r.l); break;
        case 0xB6: OR_HLm(); break;
        case 0xB7: OR_r(r.a); break;
        case 0xB8: CP_r(r.b); break;
        case 0xB9: CP_r(r.c); break;
        case 0xBA: CP_r(r.d); break;
        case 0xBB: CP_r(r.e); break;
        case 0xBC: CP_r(r.h); break;
        case 0xBD: CP_r(r.l); break;
        case 0xBE: CP_HLm(); break;
        case 0xBF: CP_r(r.a); break;
        case 0xC0: RETNZ(); break;
        case 0xC1: POP(&r.b, &r.c); break;
        case 0xC2: JPNZ_nn(); break;
        case 0xC3: JP_nn(); break;
        case 0xC4: CALLNZ_nn(); break;
        case 0xC5: PUSH(BC()); break;
        case 0xC6: ADD_n(); break;
        case 0xC7: RST_n(0x00); break;
        case 0xC8: RETZ(); break;
        case 0xC9: RET(); break;
        case 0xCA: JPZ_nn(); break;
        case 0xCB: CB_PREFIX(); break;
        case 0xCC: CALLZ_nn(); break;
        case 0xCD: CALL_nn(); break;
        case 0xCE: ADC_n(); break;
        case 0xCF: RST_n(0x08); break;
        case 0xD0: RETNC(); break;
        case 0xD1: POP(&r.d, &r.e); break;
        case 0xD2: JPNC_nn(); break;
        case 0xD4: CALLNC_nn(); break;
        case 0xD5: PUSH(DE()); break;
        case 0xD6: SUB_n(); break;
        case 0xD7: RST_n(0x10); break;
        case 0xD8: RETC(); break;
        case 0xD9: RETI(); break;
        case 0xDA: JPC_nn(); break;
        case 0xDC: CALLC_nn(); break;
        case 0xDE: SBC_n(); break;
        case 0xDF: RST_n(0x18); break;
        case 0xE0: LD_IOnA(); break;
        case 0xE1: POP(&r.h, &r.l); break;
        case 0xE2: LD_IOCA(); break;
        case 0xE5: PUSH(HL()); break;
        case 0xE6: AND_n(); break;
        case 0xE7: RST_n(0x20); break;
        case 0xE8: ADD_SPdd(); break;
        case 0xE9: JP_HL(); break;
        case 0xEA: LD_nnmA(); break;
        case 0xEE: XOR_n(); break;
        case 0xEF: RST_n(0x28); break;
        case 0xF0: LD_AIOn(); break;
        case 0xF1: POP(&r.a, &r.f); break;
        case 0xF2: LD_AIOC(); break;
        case 0xF3: DI(); break;
        case 0xF5: PUSH(AF()); break;
        case 0xF6: OR_n(); break;
        case 0xF7: RST_n(0x30); break;
        case 0xF8: LD_HLSPdd(); break;
        case 0xF9: LD_SPHL(); break;
        case 0xFA: LD_Annm(); break;
        case 0xFB: EI(); break;
        case 0xFE: CP_n(); break;
        case 0xFF: RST_n(0x38); break;
        default: printf("UNKNOWN OP %02x\n", op); assert(0);
    }
}

