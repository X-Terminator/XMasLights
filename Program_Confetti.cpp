#include "Programs.h"
#include "Settings.h"

//#define CONFETTI_SATURATION  255
#define CONFETTI_BRIGHTNESS  255

bool Program_Confetti::Start()
{
  FadeAmount = 5;
  CurrentHue = g_GlobalSettings.Hue;
  HueRange = 255;
  HueInc = 1;
  return true;
}

bool Program_Confetti::Update()
{
  CRGBPalette16 lvPalletes[] = {OceanColors_p, LavaColors_p, ForestColors_p, RainbowColors_p};
  
  fadeToBlackBy(g_LEDS, NUM_LEDS, FadeAmount);                // Low values = slower fade.
  int pos = random16(NUM_LEDS);                               // Pick an LED at random.
  //g_LEDS[pos] += CHSV((CurrentHue + random16(HueRange))/4 , CONFETTI_SATURATION, CONFETTI_BRIGHTNESS);  
  g_LEDS[pos] = ColorFromPalette(lvPalletes[g_GlobalSettings.Hue >> 6], CurrentHue + random16(HueRange)/4 , CONFETTI_BRIGHTNESS, LINEARBLEND);
  CurrentHue += HueInc;                                // It increments here.
  Variate();
  return true;
}

void Program_Confetti::Variate()
{
  uint8_t lvSeconds = (millis() / 1000) % 60;
  static uint8_t s_PrevSeconds = 99;
  if (lvSeconds != s_PrevSeconds) 
  {                             
    s_PrevSeconds = lvSeconds;
    switch(lvSeconds) 
    {
      case 30: 
        HueInc = random8(1, 3); 
        CurrentHue = random8(255); 
        FadeAmount = random8(3, 8); 
        HueRange = random8(64, 255);
        break;
    }
  }
}
