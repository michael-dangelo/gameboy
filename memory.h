#include <stdint.h>

uint8_t Mem_rb(uint16_t addr);
uint16_t Mem_rw(uint16_t addr);
void Mem_wb(uint16_t addr, uint8_t val);
void Mem_ww(uint16_t addr, uint16_t val);

