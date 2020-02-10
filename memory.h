#include <stdint.h>

void loadBootRom();
uint8_t getByte(uint16_t addr);
void writeByte(uint16_t addr, uint8_t val);
