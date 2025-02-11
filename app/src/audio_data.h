#include <stdint.h>

#define SAMPLE_FREQUENCY   44100
#define BYTES_PER_SAMPLE   sizeof(int16_t)
#define NUMBER_OF_CHANNELS 2

/* Such block length provides an echo with the delay of 100 ms. */
#define SAMPLES_PER_BLOCK ((SAMPLE_FREQUENCY / 10) * NUMBER_OF_CHANNELS)
#define BLOCK_SIZE        (BYTES_PER_SAMPLE * SAMPLES_PER_BLOCK)

extern int16_t audio_data[BLOCK_SIZE];
