#include "Programs.h"
#include "Settings.h"


#define LED_SET_BRIGHTENING(l)   {l.r &= ~0x01;l.g &= ~0x01;l.b |=  0x01;}
#define LED_SET_DIMMING(l)       {l.r |=  0x01;l.g &= ~0x01;l.b &= ~0x01;}
#define LED_SET_IDLE(l)          {l.r &= ~0x01;l.g &= ~0x01;l.b &= ~0x01;}

#define LED_IS_BRIGHTENING(l)   (l.b & 0x01)
#define LED_IS_DIMMING(l)       (l.r & 0x01)
#define LED_IS_IDLE(l)          (!LED_IS_BRIGHTENING(l) && !LED_IS_DIMMING(l))

#define BASE_BRIGHTNESS       32
#define TWINKLE_BRIGHTNESS    80

// Amount to increment the color by each loop as it gets brighter:
#define DELTA_COLOR_UP        CRGB(4,4,4)

// Amount to decrement the color by each loop as it gets dimmer:
#define DELTA_COLOR_DOWN      CRGB(2,2,2)

// Chance of each pixel starting to brighten up.  
// 1 or 2 = a few brightening pixels at a time.
// 10 = lots of pixels brightening at a time.
#define CHANCE_OF_TWINKLE     1

bool Program_Twinkle::Start()
{
  return true;
}

bool Program_Twinkle::Update()
{
  static uint8_t s_DelayCounter = 0;
  static uint8_t s_PrevHue = DEFAULT_HUE + 10;

  if (abs((int)s_PrevHue - g_GlobalSettings.Hue) > 2)
  {
    hsv2rgb_spectrum(CHSV(g_GlobalSettings.Hue, 255, TWINKLE_BRIGHTNESS), PeakColor);
    hsv2rgb_spectrum(CHSV(g_GlobalSettings.Hue, 255, BASE_BRIGHTNESS), BaseColor);
    
    LED_SET_IDLE(BaseColor);
    LED_SET_IDLE(PeakColor);
  
    // Fill LED array with base color  
    fill_solid( g_LEDS, NUM_LEDS, BaseColor);

    s_PrevHue = g_GlobalSettings.Hue;
  }
  
  if (s_DelayCounter++ > 5)
  {
    s_DelayCounter = 0;
  }
  for( uint16_t i = 0; i < NUM_LEDS; i++) 
  {
    if( LED_IS_IDLE(g_LEDS[i]) && (s_DelayCounter == 0))
    {
      // LED is IDLE
      // Randomly consider making it start getting brighter
      if(random8() <= (g_GlobalSettings.Speed/64)) 
      {
        LED_SET_BRIGHTENING(g_LEDS[i]);
      }      
    }
    else if(LED_IS_BRIGHTENING(g_LEDS[i])) 
    {
      // LED is getting brighter
      // if it's at peak color, switch it to getting dimmer again
      if( g_LEDS[i] >= PeakColor) 
      {
        LED_SET_DIMMING(g_LEDS[i]);
      } 
      else 
      {
        // otherwise, just keep brightening it:
        g_LEDS[i] += DELTA_COLOR_UP;
        LED_SET_BRIGHTENING(g_LEDS[i]);
      }
    } 
    else
    {
      // getting dimmer again
      // if it's back to base color, switch it to IDLE
      if( g_LEDS[i] <= BaseColor ) 
      {
        g_LEDS[i] = BaseColor; // reset to exact base color, in case we overshot
        LED_SET_IDLE(g_LEDS[i]);
      } 
      else 
      {
        // otherwise, just keep dimming it down:
        g_LEDS[i] -= DELTA_COLOR_DOWN;
        LED_SET_DIMMING(g_LEDS[i]);
      }
    }
  }
  return true;
}
