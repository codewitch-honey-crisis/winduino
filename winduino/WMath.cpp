
extern "C" {
#include <stdlib.h>
#include <stdint.h>
}

// Allows the user to choose between Real Hardware
// or Software Pseudo random generators for the
// Arduino random() functions
static bool s_useRandomHW = true;
void useRealRandomGenerator(bool useRandomHW) {
  s_useRandomHW = useRandomHW;
}

// Calling randomSeed() will force the
// Pseudo Random generator like in 
// Arduino mainstream API
void randomSeed(unsigned long seed)
{
    if(seed != 0) {
        srand(seed);
        s_useRandomHW = false;
    }
}

long random( long howsmall, long howbig );
long random( long howbig )
{
  if ( howbig == 0 )
  {
    return 0 ;
  }
  if (howbig < 0) {
    return (random(0, -howbig));
  }
  // if randomSeed was called, fall back to software PRNG
  uint32_t val = (s_useRandomHW) ? rand() : rand();
  return val % howbig;
}

long random(long howsmall, long howbig)
{
    if(howsmall >= howbig) {
        return howsmall;
    }
    long diff = howbig - howsmall;
    return random(diff) + howsmall;
}

long map(long x, long in_min, long in_max, long out_min, long out_max) {
    const long run = in_max - in_min;
    if(run == 0){
        // log_e("map(): Invalid input range, min == max");
        return -1; // AVR returns -1, SAM returns 0
    }
    const long rise = out_max - out_min;
    const long delta = x - in_min;
    return (delta * rise) / run + out_min;
}

uint16_t makeWord(uint16_t w)
{
    return w;
}

uint16_t makeWord(uint8_t h, uint8_t l)
{
    return (h << 8) | l;
}