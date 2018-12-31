#include "Programs.h"
#include "Settings.h"

#define METEOR_DISTANCE   50
#define METEOR_HUE_SHIFT  64
#define METEOR_RANGE      (NUM_LEDS+20)

Program_Meteor::Program_Meteor() : CLEDProgram("Meteor")
{
  VariateEnabled = false;//inVariate;
  TicksPerCycle = METEOR_RANGE;
  if (VariateEnabled)
  {
    if (random(2))
    {
      Direction = -Direction;
    }
    MeteorSize = random8(5,10);
    BaseHue = random8(255);
    MeteorCount = random8(1, NUM_LEDS/30);
  }
}

bool Program_Meteor::Start()
{
  fill_solid(g_LEDS, NUM_LEDS, CRGB(0,0,0));
  BaseHue = g_GlobalSettings.Hue;
  RandomDecay = true;
  TrailDecay = 64;
  MeteorSize = 1;
  MeteorCount = 1;
  MeteorPos = 0;
  Direction = 1;
  return true;
}

bool Program_Meteor::Update()
{
  // fade brightness all LEDs one step
  if (RandomDecay)
  {
    for(int j=0; j<NUM_LEDS; j++) 
    {
      if(g_LEDS[j] && (random(2)))
      {
        g_LEDS[j].fadeToBlackBy( TrailDecay );
      }
    }
  }
  else
  {
    fadeToBlackBy(g_LEDS, NUM_LEDS, TrailDecay);
  }
  
  // draw meteor(s)
  for (int m = 0; m < MeteorCount; m++)
  {   
    for(int j = 0; j < MeteorSize; j++) 
    {
      int lvPos = MeteorPos-(m*METEOR_DISTANCE*Direction)-(j*Direction);
      if (lvPos < 0)
      {
        lvPos = METEOR_RANGE - lvPos;
      }
      else if (lvPos > METEOR_RANGE)
      {
        lvPos -= METEOR_RANGE;
      }
      if( (lvPos < NUM_LEDS) && (lvPos >= 0) )
      {
        g_LEDS[lvPos] = CHSV(g_GlobalSettings.Hue + (m * METEOR_HUE_SHIFT), 255, 255);
      }
    }
  }
  if (g_GlobalSettings.Reverse)
  {
     MeteorPos -= Direction;
  }
  else
  {
    MeteorPos += Direction;
  }
  
  if (MeteorPos < 0)
  {
    MeteorPos = METEOR_RANGE;
  }
  else if (MeteorPos > METEOR_RANGE)
  {
    MeteorPos = 0;
  }
  if (VariateEnabled)
  {
    Variate();
  }
  return (MeteorPos == 0);
}

void Program_Meteor::Variate()
{
  uint8_t lvSeconds = (millis() / 1000) % 60;
  static uint8_t s_PrevSeconds = 99;
  if (lvSeconds != s_PrevSeconds) 
  {                             
    s_PrevSeconds = lvSeconds;
    switch(lvSeconds) 
    {
      case 30: 
        if (random(2))
        {
          Direction = -Direction;
        }
        MeteorSize = random8(5,10);
        BaseHue = random8(255);
        MeteorCount = random8(1, NUM_LEDS/30);
        break;
    }
  }
}
