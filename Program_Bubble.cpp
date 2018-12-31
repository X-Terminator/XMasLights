#include "Programs.h"
#include "Settings.h"

#define BUBBLE_SIZE           9
#define MAX_BUBBLES           (DEFAULT_NUM_LEDS / 15)
#define BUBBLES_DECAY         100
#define BUBBLES_THRESHOLD     85

static int s_BubblePos[MAX_BUBBLES];
static unsigned long s_LastBubbleTime = 0;

bool Program_Bubble::Start()
{
  fill_solid(g_LEDS, NUM_LEDS, CRGB(0,0,0));
 // BaseHue = g_GlobalSettings.Hue;
  return true;
}

bool Program_Bubble::Update()
{
  //fadeToBlackBy(g_LEDS, NUM_LEDS, BUBBLES_DECAY);

  // build pressure
  for (int m = 0; m < BUBBLE_SIZE; m++)
  {   
     g_LEDS[m] += CHSV(g_GlobalSettings.Hue, 128, random8(BUBBLES_DECAY)+1);
//     if (m > 1)
//     {
//      g_LEDS[m].r /= 10;
//      g_LEDS[m].g /= 10;
//      g_LEDS[m].b /= 10;
//      g_LEDS[m-1].r -= g_LEDS[m-1].r / 10;
//      g_LEDS[m-1].g -= g_LEDS[m-1].g / 10;
//      g_LEDS[m-1].b -= g_LEDS[m-1].b / 10;
//     }
  }
  for (int i = NUM_LEDS-1; i >= 0; i--)
  {   
    if ((i > 0) && ((g_LEDS[i-1].r > BUBBLES_THRESHOLD) || (g_LEDS[i-1].g > BUBBLES_THRESHOLD) || (g_LEDS[i-1].b > BUBBLES_THRESHOLD)))
    {
      // bubble up
      g_LEDS[i] |= g_LEDS[i-1];
      g_LEDS[i-1].fadeToBlackBy(BUBBLES_DECAY);
    }
    else
    {
      g_LEDS[i].fadeToBlackBy(BUBBLES_DECAY/2);
    }
  }
    
  return true;
}

void Program_Bubble::Variate()
{
  uint8_t lvSeconds = (millis() / 1000) % 60;
  static uint8_t s_PrevSeconds = 99;
  if (lvSeconds != s_PrevSeconds) 
  {                             
    s_PrevSeconds = lvSeconds;
    switch(lvSeconds) 
    {
      case 30: 
        break;
    }
  }
}
