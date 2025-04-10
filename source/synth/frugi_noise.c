#include "frugi_noise.h"

/* TODO Noise seems quieter than other oscillators, probably filtered too much */

static uint32_t noise_state = 1;
static float pink_state[4] = {0.0f};
static const uint32_t pink_update_bits = 0x0F; // 4 stages

void frugi_noise_init(struct frugi_noise *noise, float fsr, float *sample_buf)
{
    RTT_ASSERT(sample_buf != NULL);
    RTT_ASSERT(noise);

    noise->note_on = false;
    noise->level = 0;
    noise->sample_buf = sample_buf;
    noise_state = 1;
    for (int i = 0; i < 4; i++)
    {
        pink_state[i] = 0.0f;
    }
}

void frugi_noise_reset(struct frugi_noise *noise)
{
    RTT_ASSERT(noise);
    noise->note_on = false;
    for (int i = 0; i < 4; i++)
    {
        pink_state[i] = 0.0f;
    }
}

static inline float generate_white_sample(void)
{
    noise_state = noise_state * 1664525 + 1013904223;
    return ((float)(int32_t)noise_state) * (1.0f / 4294967296.0f);
}

static inline float generate_pink_sample(void)
{
    float white = generate_white_sample();
    float pink = white * 0.5f;

    uint32_t bits = noise_state & pink_update_bits;
    for (int i = 0; i < 4; i++)
    {
        if (!(bits & (1 << i)))
        {
            pink_state[i] = white;
        }
        pink += pink_state[i] * 0.125f; // Equal weights for simplicity
    }

    return pink;
}

void frugi_noise_render(struct frugi_noise *noise, size_t block_size)
{
    RTT_ASSERT(noise);
    RTT_ASSERT(noise->sample_buf);

    if (!noise->note_on)
    {
        return;
    }

    float *restrict ptr = noise->sample_buf;
    float *restrict end = ptr + block_size;
    float level = noise->level * 0.3f;

    if (noise->type == NOISE_TYPE_WHITE)
    {
#pragma GCC unroll 4
        while (ptr < end)
        {
            *ptr++ += generate_white_sample() * level;
        }
    }
    else
    {
#pragma GCC unroll 4
        while (ptr < end)
        {
            *ptr++ += generate_pink_sample() * level;
        }
    }
}

void frugi_noise_note_on(struct frugi_noise *noise)
{
    RTT_ASSERT(noise);

    noise->note_on = true;
}

void frugi_noise_note_off(struct frugi_noise *noise)
{
    RTT_ASSERT(noise);
    noise->note_on = false;
}

void frugi_noise_update_params(struct frugi_noise *noise, float level, float type)
{
    RTT_ASSERT(noise);
    noise->level = level;
    noise->type = type;
}