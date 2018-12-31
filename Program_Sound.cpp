#include "Programs.h"
#include "Settings.h"

#ifdef INCLUDE_PROGRAM_SOUND

//#define MEASURE_NOISE_FLOOR

#define OCTAVE 1    // Group buckets into octaves  (use the log output function LOG_OUT 1)
#define OCT_NORM 0  // Don't normalise octave intensities by number of bins
#define FHT_N 256   // set to 256 point fht
#include <FHT.h>    // include the library

// constants
static uint8_t c_NoiseCorrection[8] = {63, 80, 94, 102, 106, 107, 106, 103};
//static uint16_t c_NoiseCorrection[8] = {1227, 1191, 1131, 1181, 1261, 1368, 1444, 1513};
static uint8_t c_Threshold = 32;
static uint8_t c_Gain = 2;

static float s_FilteredMean = 0;
#ifdef MEASURE_NOISE_FLOOR
static float s_FilteredMeanBins[8];
#endif //MEASIRE_NOISE_FLOOR

bool Program_Sound::Start()
{
  ADCSRA = 0xe2;  // set the adc to free running mode
  ADMUX = 0x40;   // use adc0
  DIDR0 = 0x01;   // turn off the digital input for adc0
  return true;
}

bool Program_Sound::Stop()
{
  ADCSRA = 0;  // disable ADC
  return true;
}

bool Program_Sound::Update()
{
  uint8_t lvResult[8];
  uint16_t lvResultSum;

#ifdef DETECT_PEAKS
  uint16_t lvMinVal, lvMaxVal, lvPeakPeak;
  static uint16_t s_MaxPeakPeak = 0;
  lvMinVal = 1024;
  lvMaxVal = 0;
#endif //DETECT_PEAKS

  // Disable interrupts
  cli();  // UDRE interrupt slows this way down on arduino1.0
  for (int i = 0 ; i < FHT_N ; i++) // save 256 samples
  {
    while (!(ADCSRA & 0x10));     // wait for adc to be ready
    ADCSRA = 0xf2;                // restart adc
    byte m = ADCL;                // fetch adc data
    byte j = ADCH;
    int k = (j << 8) | m;         // form into an int
    s_FilteredMean = 0.99 * s_FilteredMean + 0.01 * k;
#ifdef DETECT_PEAKS
    if (k < lvMinVal)
    {
      lvMinVal = k;
    }
    if (k > lvMaxVal)
    {
      lvMaxVal = k;
    }
#endif //DETECT_PEAKS
    k -= (int)s_FilteredMean;    // subtract baseline offset
    k <<= 6;                    // form into a 16b signed int
    fht_input[i] = k;            // put real data into bins
  }
  fht_window();     // window the data for better frequency response
  fht_reorder();    // reorder the data before doing the fht
  fht_run();        // process the data in the fht
  fht_mag_octave();
  // Enable interrupts
  sei();
#ifdef DETECT_PEAKS
  lvPeakPeak = lvMaxVal - lvMinVal;
  if (lvPeakPeak > s_MaxPeakPeak)
  {
    s_MaxPeakPeak = lvPeakPeak;
  }
  lvResultSum = 0;
#endif //DETECT_PEAKS    
  for (byte i = 0; i < 8; i++)
  {
    int lvTemp = fht_oct_out[i];
#ifdef MEASURE_NOISE_FLOOR
    s_FilteredMeanBins[i] = 0.995 * s_FilteredMeanBins[i] + 0.05 * lvTemp;
    Serial.print(s_FilteredMeanBins[i]);
    Serial.print(", ");
#endif //MEASIRE_NOISE_FLOOR

    lvTemp -= c_NoiseCorrection[i];
    if (lvTemp < c_Threshold)
    {
      lvTemp = 0;
    }
    else
    {
      lvTemp -= c_Threshold;
      lvTemp = constrain(lvTemp * c_Gain, 0, 255);
    }

    lvResult[i] = (uint8_t)lvTemp;
    lvResultSum += lvResult[i];

  }
  lvResultSum = constrain(lvResultSum, 0, 255);

#ifndef MEASURE_NOISE_FLOOR
#ifdef DETECT_PEAKS
  Serial.println(s_MaxPeakPeak);
#else
  Serial.write(255);
  Serial.write(lvResult, 8); // send out the data
#endif //DETECT_PEAKS    
#else
  Serial.println();
#endif //MEASURE_NOISE_FLOOR

  // Update LEDs
  fadeToBlackBy( g_LEDS, NUM_LEDS, 50);
#ifdef DETECT_PEAKS
  uint8_t lvWidth = map(lvPeakPeak, 0, s_MaxPeakPeak, 0, NUM_LEDS);
  s_MaxPeakPeak--;
#else
  uint8_t lvWidth = map(lvResultSum, 0, 255, 0, NUM_LEDS);
#endif //DETECT_PEAKS    

  if (lvWidth > 0)
  {
    CHSV hsv;
    hsv.hue = 0;
    hsv.val = 255;
    hsv.sat = 240;
    for (int i = 0; i < (lvWidth / 2); i++)
    {
      g_LEDS[(NUM_LEDS / 2) + i] = hsv;
      g_LEDS[(NUM_LEDS / 2) - i] = hsv;
      hsv.hue += 5;
    }
  }

  return true;
}

#endif //INCLUDE_PROGRAM_SOUND
