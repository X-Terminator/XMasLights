#include "Programs.h"
#include "Settings.h"

#define COLORWIPE_HUE_DELTA     (255/3)

bool Program_ColorWipe::Start()
{
  WiperPos = 0;
  HueOffset = 0;
  fill_solid(g_LEDS, NUM_LEDS, CRGB(0,0,0));
  return true;
}

bool Program_ColorWipe::Update()
{
  uint8_t lvHue;
  for (int i = StartLED; i < NUM_LEDS; i++)
  {
    if (i < WiperPos)
    {
      // new color
      lvHue = g_GlobalSettings.Hue + HueOffset + COLORWIPE_HUE_DELTA; 
    }
    else
    {
      // previous color
      lvHue = g_GlobalSettings.Hue + HueOffset;
    }
    g_LEDS[i] = CHSV(lvHue, 255, 255);
  }
  if (g_GlobalSettings.Reverse)
  {
    WiperPos--;
    if (WiperPos < StartLED)
    {
      WiperPos = NUM_LEDS-1;
      if (HueOffset <= (255-COLORWIPE_HUE_DELTA))
      {
        HueOffset += COLORWIPE_HUE_DELTA;
      }
      else
      {
        HueOffset = COLORWIPE_HUE_DELTA;
      }
    }
  }
  else
  {
    WiperPos++;
    if (WiperPos >= NUM_LEDS)
    {
      WiperPos = StartLED;
      if (HueOffset <= (255-COLORWIPE_HUE_DELTA))
      {
        HueOffset += COLORWIPE_HUE_DELTA;
      }
      else
      {
        HueOffset = COLORWIPE_HUE_DELTA;
      }
    }
  }

  return (WiperPos == 0);
}
