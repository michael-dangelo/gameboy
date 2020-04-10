#include "sound.h"

#include "debug.h"

#include "SDL/SDL.h"

#include "assert.h"
#include "math.h"

#define WAV_RAM_SIZE 16
#define SAMPLE_BUFFER_SIZE 1000

#define CLOCK_HZ 4194304.0
#define CLOCKS_PER_FRAME 70224.0

static SDL_AudioDeviceID audioDevice;
static SDL_AudioSpec audioSpec;

static uint32_t lastTime = 0;

// Channel 2

static uint16_t channel2FreqCounter = 0;
static uint16_t channel2WaveIndex = 0;
static uint8_t channel2Duty = 0;

// FF10-FF14 - Channel 1
static uint8_t channel1Sweep = 0;
static uint8_t channel1Length = 0;
static uint8_t channel1VolEnvelope = 0;
static uint8_t channel1FreqLow = 0;
static uint8_t channel1FreqHigh = 0;

// FF16-FF19 - Channel 2
static uint8_t channel2Length = 0;
static uint8_t channel2VolEnvelope = 0;
static uint8_t channel2FreqLow = 0;
static uint8_t channel2FreqHigh = 0;

// FF1A-FF1E - Channel 3
static uint8_t channel3Enable = 0;
static uint8_t channel3Length = 0;
static uint8_t channel3OutputLevel = 0;
static uint8_t channel3FreqLow = 0;
static uint8_t channel3FreqHigh = 0;

// FF20-FF23 - Channel 4
static uint8_t channel4Length = 0;
static uint8_t channel4VolEnvelope = 0;
static uint8_t channel4PolyCounter = 0;
static uint8_t channel4CounterConsecutive = 0;

// FF24 - Channel Control
static uint8_t channelControl = 0;

// FF25 - Sound output selection
static uint8_t soundOutputSelect = 0;

// FF26 - Sound Enable
static uint8_t soundEnable = 1;

// FF30-FF3F - Wave Pattern RAM
static uint8_t waveRam[WAV_RAM_SIZE];

void Sound_init(void)
{
    SDL_InitSubSystem(SDL_INIT_AUDIO);

    SDL_zero(audioSpec);
    audioSpec.freq = 44100;
    audioSpec.format = AUDIO_U8;
    audioSpec.channels = 2;
    audioSpec.samples = 1024;
    audioSpec.callback = NULL;

    // TODO: We should be closing this device.
    // Need to move atexit from graphics to main,
    // call down into each module to shut down.
    audioDevice = SDL_OpenAudioDevice(
        NULL, 0, &audioSpec, NULL, 0
    );

    SDL_PauseAudioDevice(audioDevice, 0);
    lastTime = SDL_GetTicks();
}

uint16_t frequency(uint8_t freqLow, uint8_t freqHigh)
{
    uint16_t freq = freqLow | (uint16_t)(freqHigh & 0x7) << 8;
    freqLow = freq;
    return 131072 / (2048 - freq);
}

uint32_t frequencyPeriod(uint16_t frequency)
{
    return (CLOCK_HZ / frequency) / 8;
}

uint8_t duty[4] =
{
    0x01, 0x81, 0x87, 0x7E
};

void Sound_step(uint8_t ticks)
{
    const uint32_t clocksPerSample = CLOCK_HZ / audioSpec.freq;
    static uint32_t sampleCount = 0;
    static uint32_t tickCount = 0;
    static uint32_t frameTickCount = 0;
    static uint8_t samples[SAMPLE_BUFFER_SIZE] = {};

    if ((uint16_t)(channel2FreqCounter - ticks) > channel2FreqCounter)
    {
        uint16_t freq = frequency(channel2FreqLow, channel2FreqHigh);
        channel2FreqCounter = frequencyPeriod(freq);
        channel2WaveIndex = (channel2WaveIndex + 1) % 8;
    }
    else
    {
        channel2FreqCounter -= ticks;
    }
    tickCount += ticks;
    frameTickCount += ticks;
    if (frameTickCount >= CLOCKS_PER_FRAME)
    {
        SDL_QueueAudio(audioDevice, samples, sampleCount);
        sampleCount = 0;
        tickCount = 0;
        frameTickCount = 0;
    }
    else if (tickCount >= clocksPerSample)
    {
        tickCount = 0;
        uint8_t sample = ((duty[channel2Duty] >> (7 - channel2WaveIndex)) & 1) * 100;
        samples[sampleCount] = sample;
        sampleCount++;
    }
}

uint8_t Sound_rb(uint16_t addr)
{
    uint8_t val = 0;

    if (0xFF30 <= addr && addr <= 0xFF3F)
        val = waveRam[addr - 0xFF30];

    switch (addr)
    {
        case 0xFF10:
            val = channel1Sweep;
            break;
        case 0xFF11:
            val = channel1Length;
            break;
        case 0xFF12:
            val = channel1VolEnvelope;
            break;
        case 0xFF13:
            val = 0xFF;
            break;
        case 0xFF14:
            val = channel1FreqHigh;
            break;
        case 0xFF16:
            val = channel2Duty << 6;
            break;
        case 0xFF17:
            val = channel2VolEnvelope;
            break;
        case 0xFF19:
            val = channel2FreqHigh;
            break;
        case 0xFF1A:
            val = channel3Enable;
            break;
        case 0xFF1B:
            val = channel3Length;
            break;
        case 0xFF1C:
            val = channel3OutputLevel;
            break;
        case 0xFF1E:
            val = channel3FreqHigh;
            break;
        case 0xFF20:
            val = channel4Length;
            break;
        case 0xFF21:
            val = channel4VolEnvelope;
            break;
        case 0xFF22:
            val = channel4PolyCounter;
            break;
        case 0xFF23:
            val = channel4CounterConsecutive;
            break;
        case 0xFF24:
            val = channelControl;
            break;
        case 0xFF25:
            val = soundOutputSelect;
            break;
        case 0xFF26:
            val = soundEnable;
            break;
        default:
            assert(0);
    }
    return val;
}

void Sound_wb(uint16_t addr, uint8_t val)
{
    if (0xFF30 <= addr && addr <= 0xFF3F)
        waveRam[addr - 0xFF30] = val;

    switch (addr)
    {
        case 0xFF10:
            channel1Sweep = val;
            break;
        case 0xFF11:
            channel1Length = val;
            break;
        case 0xFF12:
            channel1VolEnvelope = val;
            break;
        case 0xFF13:
            channel1FreqLow = val;
            break;
        case 0xFF14:
            channel1FreqHigh = val;
            break;
        case 0xFF16:
            channel2Length = val & 0x1F;
            channel2Duty = ((val & 0xC0) >> 6) & 3;
            break;
        case 0xFF17:
            channel2VolEnvelope = val;
            break;
        case 0xFF18:
            channel2FreqLow = val;
            break;
        case 0xFF19:
            channel2FreqHigh = val;
            if (val & 0x80)
            {
                uint16_t freq = frequency(channel2FreqLow, channel2FreqHigh);
                channel2FreqCounter = frequencyPeriod(freq);
                // TODO: trigger effects
            }
            break;
        case 0xFF1A:
            channel3Enable = val;
            break;
        case 0xFF1B:
            channel3Length = val;
            break;
        case 0xFF1C:
            channel3OutputLevel = val;
            break;
        case 0xFF1D:
            channel3FreqLow = val;
            break;
        case 0xFF1E:
            channel3FreqHigh = val;
            break;
        case 0xFF20:
            channel4Length = val;
            break;
        case 0xFF21:
            channel4VolEnvelope = val;
            break;
        case 0xFF22:
            channel4PolyCounter = val;
            break;
        case 0xFF23:
            channel4CounterConsecutive = val;
            break;
        case 0xFF24:
            channelControl = val;
            break;
        case 0xFF25:
            soundOutputSelect = val;
            break;
        case 0xFF26:
            soundEnable = val;
            break;
        default:
            assert(0);
    }
}
