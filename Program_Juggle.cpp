#include "Programs.h"
#include "Settings.h"

#define JUGGLE_SATURATION  255
#define JUGGLE_BRIGHTNESS  255

#define JUGGLE_MAX_DOTS     8

Program_Juggle::Program_Juggle() : CLEDProgram("Juggle") 
{
  TicksPerCycle = NUM_LEDS;
}
  
bool Program_Juggle::Start()
{
  NumDots = 4;
  FadeAmount = 20;
  
  return true;
}

bool Program_Juggle::Update()
{
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( g_LEDS, NUM_LEDS, FadeAmount);
  byte dothue = g_GlobalSettings.Hue;
  for( int i = 0; i < NumDots; i++) 
  {
    g_LEDS[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, g_GlobalSettings.Saturation, JUGGLE_BRIGHTNESS);
    dothue += 32;
  }
  Variate();
  return true;
}

void Program_Juggle::Variate()
{
}
