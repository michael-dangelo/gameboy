#include "timer.h"

#include "debug.h"

// FF04 - Divider
static uint8_t divider = 0;

// FF05 - Counter
static uint8_t counter = 0;

// FF06 - Modulo
static uint8_t modulo = 0;

// FF07 - Control
static uint8_t timerEnable = 0;
static uint8_t inputClockSelect = 0;

// Timer Interrupt Request
static uint8_t interruptRequest = 0;

void Timer_step(uint8_t ticks)
{
    static uint8_t dividerCounter = 0;
    static uint16_t countCounter = 0;
    static uint16_t clockDivisors[] = {
        256, 4, 16, 64
    };
    ticks /= 4; // t clock to m clock
    dividerCounter += ticks;
    if (dividerCounter >= 64)
    {
        divider++;
        dividerCounter -= 64;
    }

    if (!timerEnable)
        return;

    countCounter += ticks;
    uint16_t divisor = clockDivisors[inputClockSelect];
    while (countCounter >= divisor)
    {
        counter++;
        if (counter == 0)
        {
            counter = modulo;
            INT_PRINT(("timer requesting interrupt\n"));
            interruptRequest = 1;
        }
        countCounter -= divisor;
    }
    TIMER_PRINT(("timer counter %02x countCounter %04x\n", counter, countCounter));
}

uint8_t Timer_rb(uint16_t addr)
{
    uint8_t res = 0;
    switch (addr)
    {
        case 0xFF04:
            res = divider;
            MEM_PRINT(("timer read from divider, val %02x\n", res));
            break;
        case 0xFF05:
            res = counter;
            MEM_PRINT(("timer read from counter, val %02x\n", res));
            break;
        case 0xFF06:
            res = modulo;
            MEM_PRINT(("timer read from modulo, val %02x\n", res));
            break;
        case 0xFF07:
            res = (timerEnable << 2) | (inputClockSelect);
            MEM_PRINT(("timer read from control, val %02x\n", res));
            break;
    }
    TIMER_PRINT(("reading from timer addr %04x val %02x\n", addr, res));
    return res;
}

void Timer_wb(uint16_t addr, uint8_t val)
{
    switch (addr)
    {
        case 0xFF04:
            divider = 0;
            MEM_PRINT(("timer write to divider, setting to 0\n"));
            break;
        case 0xFF05:
            counter = val;
            MEM_PRINT(("timer write to counter, val %02x\n", val));
            break;
        case 0xFF06:
            modulo = val;
            MEM_PRINT(("timer write to modulo, val %02x\n", val));
            break;
        case 0xFF07:
            timerEnable = (val >> 2) & 1;
            inputClockSelect = val & 3;
            MEM_PRINT(("timer write to control, val %02x\n", val));
            break;
    }
    TIMER_PRINT(("writing to timer addr %04x val %02x\n", addr, val));
}

uint8_t Timer_interrupt(void)
{
    uint8_t interrupt = interruptRequest;
    interruptRequest = 0;
    return interrupt;
}