#include "cpu.h"

#include "debug.h"
#include "memory.h"

static struct
{
    uint8_t a, b, c, d, e, h, l, f;
    uint16_t pc, sp;
    uint8_t m;
    uint8_t ime;
    uint8_t halted;
} r;

static void dispatch(uint8_t op);
#if defined(DEBUG_CPU) && defined(DEBUG)
static const char *opName(uint8_t op);
static const char *cbOpName(uint8_t op);
#endif
static void printCpu(void);

void Cpu_init(void)
{
#ifdef SKIP_BOOTROM
    if (r.pc == 0)
        r.pc = 0x100;
#endif
}

static uint16_t BREAK = 0x0272;

uint8_t Cpu_step(void)
{
    if (r.halted)
        return 4;
    CPU_PRINT(("--------------\n"));
    uint8_t op = Mem_rb(r.pc++);
    CPU_PRINT(("op %s\n", opName(op)));
    dispatch(op);
    printCpu();
    if (r.pc == BREAK)
        enableDebugPrints = 1;
    return r.m * 4;
}

// --- Instructions

// Helpers
static uint16_t rr(uint8_t high, uint8_t low) { return ((uint16_t)high << 8) + low; }
static uint16_t BC(void) { return rr(r.b, r.c); }
static uint16_t DE(void) { return rr(r.d, r.e); }
static uint16_t HL(void) { return rr(r.h, r.l); }
static uint16_t AF(void) { return rr(r.a, r.f); }
static uint8_t CY(void) { return (r.f >> 4) & 1; }
static uint8_t H(void) { return (r.f >> 5) & 1; }
static uint8_t N(void) { return (r.f >> 6) & 1; }
static uint8_t ZF(void) { return (r.f >> 7) & 1; }
static void setHL(uint16_t val) { r.h = val >> 8; r.l = val & 0xFF; }
static void setFlag(uint8_t val, uint8_t pos) { if (val) r.f |= (1 << pos); else r.f &= ~(1 << pos); }
static void setCY(uint8_t val) { setFlag(val, 4); }
static void setH(uint8_t val) { setFlag(val, 5); }
static void setN(uint8_t val) { setFlag(val, 6); }
static void setZF(uint8_t val) { setFlag(val, 7); }
static uint8_t HCAdd(uint8_t a, uint8_t b) { return (((a & 0xF) + (b & 0xF)) & 0x10) == 0x10; }
static uint8_t HCSub(uint8_t a, uint8_t b) { return (a & 0xF) < (b & 0xF); }

// 8-bit loads
static void LD_rr(uint8_t *dest, uint8_t src) { *dest = src; r.m = 1; }
static void LD_rn(uint8_t *dest) { *dest = Mem_rb(r.pc++); r.m = 2; }
static void LD_rHLm(uint8_t *dest) { *dest = Mem_rb(HL()); r.m = 2; }
static void LD_HLmr(uint8_t src) { Mem_wb(HL(), src); r.m = 2; }
static void LD_HLmn(void) { Mem_wb(HL(), Mem_rb(r.pc++)); r.m = 3; }
static void LD_Arrm(uint16_t addr) { r.a = Mem_rb(addr); r.m = 2; }
static void LD_Annm(void) { r.a = Mem_rb(Mem_rw(r.pc)); r.pc += 2; r.m = 4; }
static void LD_rrmA(uint16_t addr) { Mem_wb(addr, r.a); r.m = 2; }
static void LD_nnmA(void) { Mem_wb(Mem_rw(r.pc), r.a); r.pc += 2; r.m = 4; }
static void LD_AIOn(void) { r.a = Mem_rb(0xFF00 + Mem_rb(r.pc++)); r.m = 3; }
static void LD_IOnA(void) { Mem_wb(0xFF00 + Mem_rb(r.pc++), r.a); r.m = 3; }
static void LD_AIOC(void) { r.a = Mem_rb(0xFF00 + r.c); r.m = 2; }
static void LD_IOCA(void) { Mem_wb(0xFF00 + r.c, r.a); r.m = 2; }
static void LDI_HLmA(void) { Mem_wb(HL(), r.a); setHL(HL() + 1); r.m = 2; }
static void LDI_AHLm(void) { r.a = Mem_rb(HL()); setHL(HL() + 1); r.m = 2; }
static void LDD_HLmA(void) { Mem_wb(HL(), r.a); setHL(HL() - 1); r.m = 2; }
static void LDD_AHLm(void) { r.a = Mem_rb(HL()); setHL(HL() - 1); r.m = 2; }

// 16-bit loads
static void LD_rrnn(uint8_t *high, uint8_t *low) { *low = Mem_rb(r.pc++); *high = Mem_rb(r.pc++); r.m = 3; }
static void LD_SPnn(void) { r.sp = Mem_rw(r.pc); r.pc += 2; r.m = 3; }
static void LD_nnmSP(void) { Mem_ww(Mem_rw(r.pc), r.sp); r.pc += 2; r.m = 5; }
static void LD_SPHL(void) { r.sp = HL(); r.m = 2; }
static void LD_HLSPdd(void) { int8_t d = Mem_rb(r.pc++); uint8_t lowSp = r.sp & 0xFF; setH(HCAdd(lowSp, d)); setHL(r.sp + d); lowSp += d; setZF(0); setCY((uint8_t)(lowSp - d) > lowSp); setN(0); r.m = 3; }
static void PUSH(uint16_t val) { r.sp -= 2; Mem_ww(r.sp, val); r.m = 4; }
static void POP(uint8_t *high, uint8_t *low) { uint16_t w = Mem_rw(r.sp); *low = w & 0xFF; *high = w >> 8; r.sp += 2; r.m = 3; }
static void POP_AF() { uint16_t w = Mem_rw(r.sp); r.f = w & 0xF0; r.a = w >> 8; r.sp += 2; r.m = 3; }

// 8-bit arithmetic/logical
static void ADD_r(uint8_t src) { setH(HCAdd(r.a, src)); r.a += src; setZF(r.a == 0); setCY((uint8_t)(r.a - src) > r.a); setN(0); r.m = 1; }
static void ADD_n(void) { uint8_t n = Mem_rb(r.pc++); setH(HCAdd(r.a, n)); r.a += n; setZF(r.a == 0); setCY((uint8_t)(r.a - n) > r.a); setN(0); r.m = 2; }
static void ADD_HLm(void) { uint8_t n = Mem_rb(HL()); setH(HCAdd(r.a, n)); r.a += n; setZF(r.a == 0); setCY((uint8_t)(r.a - n) > r.a); setN(0); r.m = 2; }
static void ADC_r(uint8_t src) { uint8_t hc = HCAdd(src, CY()); src += CY(); uint8_t newCy = (uint8_t)(src - CY()) > src; setH(hc | HCAdd(r.a, src)); r.a += src; setZF(r.a == 0); setCY(newCy | ((uint8_t)(r.a - src) > r.a)); setN(0); r.m = 1; }
static void ADC_n(void) { uint8_t n = Mem_rb(r.pc++); uint8_t hc = HCAdd(n, CY()); n += CY(); uint8_t newCy = (uint8_t)(n - CY()) > n; setH(hc | HCAdd(r.a, n)); r.a += n; setCY(newCy | ((uint8_t)(r.a - n) > r.a)); setZF(r.a == 0); setN(0); r.m = 2; }
static void ADC_HLm(void) { uint8_t n = Mem_rb(HL()); uint8_t hc = HCAdd(n, CY()); n += CY(); uint8_t newCy = (uint8_t)(n - CY()) > n; setH(hc | HCAdd(r.a, n)); r.a += n; setCY(newCy | ((uint8_t)(r.a - n) > r.a)); setZF(r.a == 0); setN(0); r.m = 2; }
static void SUB_r(uint8_t src) { setH(HCSub(r.a, src)); r.a -= src; setZF(r.a == 0); setCY((uint8_t)(r.a + src) < r.a); setN(1); r.m = 1; }
static void SUB_n(void) { uint8_t n = Mem_rb(r.pc++); setH(HCSub(r.a, n)); r.a -= n; setZF(r.a == 0); setCY((uint8_t)(r.a + n) < r.a); setN(1); r.m = 2; }
static void SUB_HLm(void) { uint8_t n = Mem_rb(HL()); setH(HCSub(r.a, n)); r.a -= n; setZF(r.a == 0); setCY((uint8_t)(r.a + n) < r.a); setN(1); r.m = 2; }
static void SBC_r(uint8_t src) { uint8_t hc = HCSub(r.a, CY()); r.a -= CY(); uint8_t newCy = (uint8_t)(r.a + CY()) < r.a; setH(hc | HCSub(r.a, src));  r.a -= src; setCY(newCy | ((uint8_t)(r.a + src) < r.a)); setZF(r.a == 0); setN(1); r.m = 1; }
static void SBC_n(void) { uint8_t n = Mem_rb(r.pc++); uint8_t hc = HCSub(r.a, CY()); r.a -= CY(); uint8_t newCy = (uint8_t)(r.a + CY()) < r.a; setH(hc | HCSub(r.a, n)); r.a -= n; setCY(newCy | ((uint8_t)(r.a + n) < r.a)); setZF(r.a == 0); setN(1); r.m = 2; }
static void SBC_HLm(void) { uint8_t n = Mem_rb(HL()); uint8_t hc = HCSub(r.a, CY()); r.a -= CY(); uint8_t newCy = (uint8_t)(r.a + CY()) < r.a; setH(hc | HCSub(r.a, n));r.a -= n; setCY(newCy | ((uint8_t)(r.a + n) < r.a)); setZF(r.a == 0); setN(1); r.m = 2; }
static void AND_r(uint8_t src) { r.a &= src; setZF(r.a == 0); setCY(0); setN(0); setH(1); r.m = 1; }
static void AND_n(void) { r.a &= Mem_rb(r.pc++); setZF(r.a == 0); setCY(0); setN(0); setH(1); r.m = 2; }
static void AND_HLm(void) { r.a &= Mem_rb(HL()); setZF(r.a == 0); setCY(0); setN(0); setH(1); r.m = 2; }
static void XOR_r(uint8_t src) { r.a ^= src; setZF(r.a == 0); setCY(0); setN(0); setH(0); r.m = 1; }
static void XOR_n(void) { r.a ^= Mem_rb(r.pc++); setZF(r.a == 0); setCY(0); setN(0); setH(0); r.m = 2; }
static void XOR_HLm(void) { r.a ^= Mem_rb(HL()); setZF(r.a == 0); setCY(0); setN(0); setH(0); r.m = 2; }
static void OR_r(uint8_t src) { r.a |= src; setZF(r.a == 0); setCY(0); setN(0); setH(0); r.m = 1; }
static void OR_n(void) { r.a |= Mem_rb(r.pc++); setZF(r.a == 0); setCY(0); setN(0); setH(0); r.m = 2; }
static void OR_HLm(void) { r.a |= Mem_rb(HL()); setZF(r.a == 0); setCY(0); setN(0); setH(0); r.m = 2; }
static void CP_r(uint8_t src) { setH(HCSub(r.a, src)); setZF(r.a - src == 0); setCY((uint8_t)(r.a - src) > r.a); setN(1); r.m = 1; }
static void CP_n(void) { uint8_t n = Mem_rb(r.pc++); setH(HCSub(r.a, n)); setZF(r.a - n == 0); setCY((uint8_t)(r.a - n) > r.a); setN(1); r.m = 2; }
static void CP_HLm(void) { uint8_t n = Mem_rb(HL()); setH(HCSub(r.a, n)); setZF(r.a - n == 0); setCY((uint8_t)(r.a - n) > r.a); setN(1); r.m = 2; }
static void INC_r(uint8_t *src) { setH(HCAdd(*src, 1)); (*src)++; setZF(*src == 0); setN(0); r.m = 1; }
static void INC_HLm(void) { uint8_t n = Mem_rb(HL()); setH(HCAdd(n, 1)); Mem_wb(HL(), n + 1); setZF(Mem_rb(HL()) == 0); setN(0); r.m = 3; }
static void DEC_r(uint8_t *src) { setH(HCSub(*src, 1)); (*src)--; setZF(*src == 0); setN(1); r.m = 1; }
static void DEC_HLm(void) { uint8_t n = Mem_rb(HL()); setH(HCSub(n, 1)); Mem_wb(HL(), n - 1); setZF(Mem_rb(HL()) == 0); setN(1); r.m = 3; }
static void DAA(void) { if (!N()) { if (CY() || r.a > 0x99) { r.a += 0x60; setCY(1); } if (H() || (r.a & 0xF) > 0x9) r.a += 0x6; } else { if (CY()) { r.a -= 0x60; setCY(1); } if (H()) r.a -= 0x6; } setZF(r.a == 0); setH(0); r.m = 1; }
static void CPL(void) { r.a ^= 0xFF; setN(1); setH(1); r.m = 1; }
static void SCF(void) { setCY(1); setN(0); setH(0); r.m = 1; }
static void CCF(void) { setCY(CY() ^ 1); setN(0); setH(0); r.m = 1; }

// 16-bit arithmetic/logical
static void ADD_HLrr(uint16_t src) { setH((HL() & 0xFFF) + (src & 0xFFF) >= 0x1000); setHL(HL() + src); setCY((uint16_t)(HL() - src) > HL()); setN(0); r.m = 2; }
static void INC_rr(uint8_t *high, uint8_t *low) { (*low)++; if (!*low) (*high)++; r.m = 2; }
static void INC_SP(void) { r.sp++; r.m = 2; }
static void DEC_rr(uint8_t *high, uint8_t *low) { if (!*low) (*high)--; (*low)--; r.m = 2; }
static void DEC_SP(void) { r.sp--; r.m = 2; }
static void ADD_SPdd(void) { int8_t d = Mem_rb(r.pc++); uint8_t lowSp = r.sp & 0xFF; setH(HCAdd(lowSp, d)); r.sp += d; lowSp += d; setZF(0); setCY((uint8_t)(lowSp - d) > lowSp); setN(0); r.m = 4; }

// Rotate/shift
static void RLCA(void) { uint8_t v = (r.a >> 7) & 1; r.a <<= 1; r.a |= v; setZF(0); setCY(v); setN(0); setH(0); r.m = 1; }
static void RLC_r(uint8_t *reg) { uint8_t v = (*reg >> 7) & 1; *reg <<= 1; *reg |= v; setZF(*reg == 0); setCY(v); setN(0); setH(0); r.m = 2; }
static void RLC_HLm(void) { uint8_t m = Mem_rb(HL()); uint8_t v = (m >> 7) & 1; m <<= 1; m |= v; Mem_wb(HL(), m); setZF(m == 0); setCY(v); setN(0); setH(0); r.m = 4; }
static void RLA(void) { uint8_t v = CY(); setCY((r.a >> 7) & 1); r.a <<= 1; r.a |= v; setZF(0); setN(0); setH(0); r.m = 1; }
static void RL_r(uint8_t *reg) { uint8_t v = CY(); setCY((*reg >> 7) & 1); *reg <<= 1; *reg |= v; setZF(*reg == 0); setN(0); setH(0); r.m = 2; };
static void RL_HLm(void) { uint8_t m = Mem_rb(HL()); uint8_t v = CY(); setCY((m >> 7) & 1); m <<= 1; m |= v; Mem_wb(HL(), m); setZF(m == 0); setN(0); setH(0); r.m = 4; }
static void RRCA(void) { uint8_t v = r.a & 1; r.a >>= 1; r.a |= v << 7; setZF(0); setCY(v == 1); setN(0); setH(0); r.m = 1; }
static void RRC_r(uint8_t *reg) { uint8_t v = *reg & 1; *reg >>= 1; *reg |= v << 7; setZF(*reg == 0); setCY(v == 1); setN(0); setH(0); r.m = 2; }
static void RRC_HLm(void) { uint8_t m = Mem_rb(HL()); uint8_t v = m & 1; m >>= 1; m |= v << 7; Mem_wb(HL(), m); setZF(m == 0); setCY(v == 1); setN(0); setH(0); r.m = 4; }
static void RRA(void) { uint8_t v = CY(); setCY(r.a & 1); r.a >>= 1; r.a |= v << 7; setZF(0); setN(0); setH(0); r.m = 1; }
static void RR_r(uint8_t *reg) { uint8_t v = CY(); setCY(*reg & 1); *reg >>= 1; *reg |= v << 7; setZF(*reg == 0); setN(0); setH(0); r.m = 2; }
static void RR_HLm(void) { uint8_t m = Mem_rb(HL()); uint8_t v = CY(); setCY(m & 1); m >>= 1; m |= v << 7; Mem_wb(HL(), m); setZF(m == 0); setN(0); setH(0); r.m = 4; }
static void SLA_r(uint8_t *reg) { uint8_t v = (*reg >> 7) & 1; *reg <<= 1; setZF(*reg == 0); setCY(v == 1); setN(0); setH(0); r.m = 2; }
static void SLA_HLm(void) { uint8_t v = (Mem_rb(HL()) >> 7) & 1;  Mem_wb(HL(), Mem_rb(HL()) << 1); setZF(Mem_rb(HL()) == 0); setCY(v == 1); setN(0); setH(0); r.m = 4;  }
static void SRA_r(uint8_t *reg) { uint8_t v = *reg & (1 << 7); setCY(*reg & 1); *reg >>= 1; *reg |= v; setZF(*reg == 0); setN(0); setH(0); r.m = 2; }
static void SRA_HLm(void) { uint8_t c = Mem_rb(HL()); uint8_t v = c & (1 << 7); setCY(c & 1); c >>= 1; c |= v; Mem_wb(HL(), c); setZF(Mem_rb(HL()) == 0); setN(0); setH(0); r.m = 4; }
static void SWAP_r(uint8_t *reg) { uint8_t v = *reg; *reg >>= 4; *reg += v << 4; setZF(*reg == 0); setCY(0); setN(0); setH(0); r.m = 2; }
static void SWAP_HLm(void) { uint8_t c = Mem_rb(HL()); uint8_t v = c; c >>= 4; c += v << 4; Mem_wb(HL(), c); setZF(Mem_rb(HL()) == 0); setCY(0); setN(0); setH(0); r.m = 4; }
static void SRL_r(uint8_t *reg) { uint8_t v = *reg & 1; *reg >>= 1; setZF(*reg == 0); setCY(v == 1); setN(0); setH(0); r.m = 2; }
static void SRL_HLm(void) { uint8_t v = Mem_rb(HL()) & 1; Mem_wb(HL(), Mem_rb(HL()) >> 1); setZF(Mem_rb(HL()) == 0); setCY(v == 1); setN(0); setH(0); r.m = 4; }

// Single-bit
static void BIT_nr(uint8_t n, uint8_t *reg) { setZF(!((*reg >> n) & 1)); setN(0); setH(1); r.m = 2; }
static void BIT_nHLm(uint8_t n) { setZF(!((Mem_rb(HL()) >> n) & 1)); setN(0); setH(1); r.m = 4; }
static void SET_nr(uint8_t n, uint8_t *reg) { *reg |= 1 << n; r.m = 2; }
static void SET_nHLm(uint8_t n) { Mem_wb(HL(), Mem_rb(HL()) | 1 << n); r.m = 4; }
static void RES_nr(uint8_t n, uint8_t *reg) { *reg &= ~(1 << n); r.m = 2; }
static void RES_nHLm(uint8_t n) { Mem_wb(HL(), Mem_rb(HL()) & ~(1 << n)); r.m = 4; }

// Control
static void NOP(void) { r.m = 1; }
static void HALT(void) { r.halted = 1; r.m = 1; }
static void STOP(void) { Mem_rb(r.pc++); r.m = 1; }
static void DI(void) { INT_PRINT(("ime disabled by DI\n")); r.ime = 0; r.m = 1; }
static void EI(void) { INT_PRINT(("ime enabled by EI\n"));r.ime = 1; r.m = 1; }

// Jumps
static void JP_nn(void) { r.pc = Mem_rw(r.pc); r.m = 4; }
static void JP_HL(void) { r.pc = HL(); r.m = 1; }
static void JPNZ_nn(void) { uint16_t n = Mem_rw(r.pc); r.pc += 2; r.m = 3; if (!ZF()) { r.pc = n; r.m = 4; } }
static void JPZ_nn(void) { uint16_t n = Mem_rw(r.pc); r.pc += 2; r.m = 3; if (ZF()) { r.pc = n; r.m = 4; } }
static void JPNC_nn(void) { uint16_t n = Mem_rw(r.pc); r.pc += 2; r.m = 3; if (!CY()) { r.pc = n; r.m = 4; } }
static void JPC_nn(void) { uint16_t n = Mem_rw(r.pc); r.pc += 2; r.m = 3; if (CY()) { r.pc = n; r.m = 4; } }
static void JR_dd(void) { int8_t n = Mem_rb(r.pc++); r.pc += n; r.m = 3; }
static void JRNZ_dd(void) { int8_t n = Mem_rb(r.pc++); r.m = 2; if (!ZF()) { r.pc += n; r.m = 3; } }
static void JRZ_dd(void) { int8_t n = Mem_rb(r.pc++); r.m = 2; if (ZF()) { r.pc += n; r.m = 3; } }
static void JRNC_dd(void) { int8_t n = Mem_rb(r.pc++); r.m = 2; if (!CY()) { r.pc += n; r.m = 3; } }
static void JRC_dd(void) { int8_t n = Mem_rb(r.pc++); r.m = 2; if (CY()) { r.pc += n; r.m = 3; } }
static void CALL(uint16_t addr) { r.sp -= 2; Mem_ww(r.sp, r.pc); r.pc = addr; }
static void CALL_nn(void) { uint16_t n = Mem_rw(r.pc); r.pc += 2; CALL(n); }
static void CALLNZ_nn(void) { if (!ZF()) { CALL_nn(); r.m = 6; } else { r.pc += 2; r.m = 3; } }
static void CALLZ_nn(void) { if (ZF()) { CALL_nn(); r.m = 6; } else { r.pc += 2; r.m = 3; } }
static void CALLNC_nn(void) { if (!CY()) { CALL_nn(); r.m = 6; } else { r.pc += 2; r.m = 3; } }
static void CALLC_nn(void) { if (CY()) { CALL_nn(); r.m = 6; } else { r.pc += 2; r.m = 3; } }
static void RET(void) { r.pc = Mem_rw(r.sp); r.sp += 2; r.m = 4; }
static void RETNZ(void) { if (!ZF()) { RET(); r.m = 5; } else r.m = 2; }
static void RETZ(void) { if (ZF()) { RET(); r.m = 5; } else r.m = 2; }
static void RETNC(void) { if (!CY()) { RET(); r.m = 5; } else r.m = 2; }
static void RETC(void) { if (CY()) { RET(); r.m = 5; } else r.m = 2; }
static void RETI(void) { EI(); RET(); r.m = 4; }
static void RST_n(uint8_t n) { CALL(n); r.m = 4; }

static void CB_PREFIX()
{
    uint8_t op = Mem_rb(r.pc++);
    CPU_PRINT(("op %s\n", cbOpName(op)));
    switch(op)
    {
        case 0x00: RLC_r(&r.b); break;
        case 0x01: RLC_r(&r.c); break;
        case 0x02: RLC_r(&r.d); break;
        case 0x03: RLC_r(&r.e); break;
        case 0x04: RLC_r(&r.h); break;
        case 0x05: RLC_r(&r.l); break;
        case 0x06: RLC_HLm(); break;
        case 0x07: RLC_r(&r.a); break;
        case 0x08: RRC_r(&r.b); break;
        case 0x09: RRC_r(&r.c); break;
        case 0x0A: RRC_r(&r.d); break;
        case 0x0B: RRC_r(&r.e); break;
        case 0x0C: RRC_r(&r.h); break;
        case 0x0D: RRC_r(&r.l); break;
        case 0x0E: RRC_HLm(); break;
        case 0x0F: RRC_r(&r.a); break;
        case 0x10: RL_r(&r.b); break;
        case 0x11: RL_r(&r.c); break;
        case 0x12: RL_r(&r.d); break;
        case 0x13: RL_r(&r.e); break;
        case 0x14: RL_r(&r.h); break;
        case 0x15: RL_r(&r.l); break;
        case 0x16: RL_HLm(); break;
        case 0x17: RL_r(&r.a); break;
        case 0x18: RR_r(&r.b); break;
        case 0x19: RR_r(&r.c); break;
        case 0x1A: RR_r(&r.d); break;
        case 0x1B: RR_r(&r.e); break;
        case 0x1C: RR_r(&r.h); break;
        case 0x1D: RR_r(&r.l); break;
        case 0x1E: RR_HLm(); break;
        case 0x1F: RR_r(&r.a); break;
        case 0x20: SLA_r(&r.b); break;
        case 0x21: SLA_r(&r.c); break;
        case 0x22: SLA_r(&r.d); break;
        case 0x23: SLA_r(&r.e); break;
        case 0x24: SLA_r(&r.h); break;
        case 0x25: SLA_r(&r.l); break;
        case 0x26: SLA_HLm(); break;
        case 0x27: SLA_r(&r.a); break;
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
        case 0xF0: SET_nr(6, &r.b); break;
        case 0xF1: SET_nr(6, &r.c); break;
        case 0xF2: SET_nr(6, &r.d); break;
        case 0xF3: SET_nr(6, &r.e); break;
        case 0xF4: SET_nr(6, &r.h); break;
        case 0xF5: SET_nr(6, &r.l); break;
        case 0xF6: SET_nHLm(6); break;
        case 0xF7: SET_nr(6, &r.a); break;
        case 0xF8: SET_nr(7, &r.b); break;
        case 0xF9: SET_nr(7, &r.c); break;
        case 0xFA: SET_nr(7, &r.d); break;
        case 0xFB: SET_nr(7, &r.e); break;
        case 0xFC: SET_nr(7, &r.h); break;
        case 0xFD: SET_nr(7, &r.l); break;
        case 0xFE: SET_nHLm(7); break;
        case 0xFF: SET_nr(7, &r.a); break;
    }
}

static void dispatch(uint8_t op)
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
        case 0x07: RLCA(); break;
        case 0x08: LD_nnmSP(); break;
        case 0x09: ADD_HLrr(BC()); break;
        case 0x0A: LD_Arrm(BC()); break;
        case 0x0B: DEC_rr(&r.b, &r.c); break;
        case 0x0C: INC_r(&r.c); break;
        case 0x0D: DEC_r(&r.c); break;
        case 0x0E: LD_rn(&r.c); break;
        case 0x0F: RRCA(); break;
        case 0x10: STOP(); break;
        case 0x11: LD_rrnn(&r.d, &r.e); break;
        case 0x12: LD_rrmA(DE()); break;
        case 0x13: INC_rr(&r.d, &r.e); break;
        case 0x14: INC_r(&r.d); break;
        case 0x15: DEC_r(&r.d); break;
        case 0x16: LD_rn(&r.d); break;
        case 0x17: RLA(); break;
        case 0x18: JR_dd(); break;
        case 0x19: ADD_HLrr(DE()); break;
        case 0x1A: LD_Arrm(DE()); break;
        case 0x1B: DEC_rr(&r.d, &r.e); break;
        case 0x1C: INC_r(&r.e); break;
        case 0x1D: DEC_r(&r.e); break;
        case 0x1E: LD_rn(&r.e); break;
        case 0x1F: RRA(); break;
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
        case 0xF1: POP_AF(); break;
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
    }
}

void Cpu_interrupts(void)
{
    uint8_t interruptFlag = Mem_rb(0xFF0F);
    if (interruptFlag)
        r.halted = 0;

    if (!r.ime)
        return;

    uint8_t interruptEnable = Mem_rb(0xFFFF);
    uint8_t permittedInterrupts = interruptFlag & interruptEnable;
    if (permittedInterrupts)
    {
        r.ime = 0;
        if (permittedInterrupts & 0x1)
        {
            // vblank interrupt
            INT_PRINT(("cpu handling vblank interrupt\n"));
            interruptFlag &= ~0x1;
            CALL(0x40);
        }
        else if (permittedInterrupts & 0x2)
        {
            // lcd status
            INT_PRINT(("cpu handling lcd status interrupt\n"));
            interruptFlag &= ~0x2;
            CALL(0x48);
        }
        else if (permittedInterrupts & 0x4)
        {
            // timer
            INT_PRINT(("cpu handling timer interrupt\n"));
            interruptFlag &= ~0x4;
            CALL(0x50);
        }
        else if (permittedInterrupts & 0x8)
        {
            // serial
            INT_PRINT(("cpu handling serial interrupt\n"));
            interruptFlag &= ~0x8;
            CALL(0x58);
        }
        else if (permittedInterrupts & 0x10)
        {
            // joypad
            INT_PRINT(("cpu handling joypad interrupt\n"));
            interruptFlag &= ~0x10;
            CALL(0x60);
        }
        Mem_wb(0xFF0F, interruptFlag);
    }
}

const char *opNames[256] = {
    "NOP", "LD BC,d16", "LD (BC),A", "INC BC", "INC B", "DEC B", "LD B,d8", "RLCA",
    "LD (a16),SP", "ADD HL,BC", "LD A,(BC)", "DEC BC", "INC C", "DEC C", "LD C,d8", "RRCA",
    "STOP", "LD DE,d16", "LD (DE),A", "INC DE", "INC D", "DEC D", "LD D,d8", "RLA",
    "JR r8", "ADD HL,DE", "LD A,(DE)", "DEC DE", "INC E", "DEC E", "LD E,d8", "RRA",
    "JR NZ,r8", "LD HL,d16", "LDI (HL),A", "INC HL", "INC H", "DEC H", "LD H,d8", "DAA",
    "JR Z,r8", "ADD HL,HL", "LDI A,(HL)", "DEC HL", "INC L", "DEC L", "LD L,d8", "CPL",
    "JR NC,r8", "LD SP,d16", "LDD (HL),A", "INC SP", "INC (HL)", "DEC (HL)", "LD (HL),d8", "SCF",
    "JR C,r8", "ADD HL,SP", "LDD A,(HL)", "DEC SP", "INC A", "DEC A", "LD A,d8", "CCF",
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

#if defined(DEBUG_CPU) && defined(DEBUG)
const char *opName(uint8_t op)
{
    return opNames[op];
}

const char *cbOpName(uint8_t op)
{
    return cbOpNames[op];
}
#endif

static void printCpu(void)
{
    CPU_PRINT(("a: %02x b: %02x c: %02x d: %02x e: %02x "
               "h: %02x l: %02x f: %02x pc: %04x sp: %04x "
               "zf: %d cy: %d n: %0d hc: %d m: %x ime %d\n",
        r.a, r.b, r.c, r.d, r.e, r.h, r.l, r.f,
        r.pc, r.sp, ZF(), CY(), N(), H(), r.m, r.ime));
}
