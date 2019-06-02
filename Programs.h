#ifndef PROGRAMS_H
#define PROGRAMS_H

#include "Settings.h"
#include "CProgram.h"


class Program_Connecting : public CLEDProgram
{
  public:
    Program_Connecting() : CLEDProgram("Connecting") {  TicksPerCycle = NUM_LEDS; }
    int GetUpdatePeriod(uint8_t inSpeed) { return (1000 / NUM_LEDS); }
    bool Update() 
    {
      static uint8_t s_Offset = START_LED;
      fadeToBlackBy( g_LEDS, NUM_LEDS, 150);
      g_LEDS[s_Offset] = CRGB(0,0,255);
      s_Offset++;
      if (s_Offset >= NUM_LEDS)
      {
        s_Offset = START_LED;
      }
      return true;
    }
  private:
};

class Program_Solid : public CLEDProgram
{
  public:
    Program_Solid(const char *inName = "Solid", uint8_t inNumColors = 1, uint16_t inStartLED = 0) : CLEDProgram(inName) { NumColors = inNumColors; TicksPerCycle = NUM_LEDS/10; StartLED = inStartLED; }
    bool Start() { fill_solid(g_LEDS, NUM_LEDS, CRGB(0,0,0)); }
    bool Update() 
    {
      static uint8_t s_Offset = 0;
      uint8_t lvSpacing = 85;
      if (NumColors > 3)
      {
        lvSpacing = 42;
      }
      
      for (uint16_t i = StartLED; i < NUM_LEDS; i++)
      {
        g_LEDS[i] = CHSV(g_GlobalSettings.Hue + ((i + s_Offset) % NumColors) * lvSpacing, g_GlobalSettings.Saturation, 255);
      }
      if (g_GlobalSettings.Reverse)
      {
        s_Offset--;
      }
      else
      {
        s_Offset++;
      }
      return true;
    }
  private:
    uint8_t NumColors;
    uint16_t StartLED;
};

class Program_Breathe : public CLEDProgram
{
  public:
    Program_Breathe() : CLEDProgram("Breathe") { NoDelay = true; }
    bool Update() 
    {
      uint8_t BeatsPerMinute = g_GlobalSettings.Speed / 2;
      fill_solid(g_LEDS, NUM_LEDS, CHSV(g_GlobalSettings.Hue, g_GlobalSettings.Saturation, beatsin8( BeatsPerMinute, 128, 255)));
    }
  private:
};


class Program_Strobe : public CLEDProgram
{
  public:
    Program_Strobe() : CLEDProgram("Strobe") { NoDelay = true; IncludeInAutoProgram = false; }
    bool Update() 
    {
      static uint8_t s_PrevVal = 0;
      // g_GlobalSettings.Speed is interpreted as Frequency[Hz] * 8
      // ==> Frequency[Hz] = g_GlobalSettings.Speed / 8 = g_GlobalSettings.Speed >> 3
      // Hz = 1/s = 1/1000000 us = 286 / (256*1048576) = 286 / (256 * 2^20) = 286 / 2^23 = 286 >> 23
      if ((uint8_t)(((unsigned long)micros() * g_GlobalSettings.Speed * 286) >> 23) >= 128)
      {
        if (s_PrevVal)
        {
          FastLED.clear();
          s_PrevVal = 0;
        }
      }
      else
      {
        if (!s_PrevVal)
        {
          fill_solid(g_LEDS, NUM_LEDS, CHSV(g_GlobalSettings.Hue, g_GlobalSettings.Saturation, 255));
          s_PrevVal = 1;
        }
      }
    }
  private:
};

class Program_ColorWipe : public CLEDProgram
{
  public:
    Program_ColorWipe(uint16_t inStartLED = 0) : CLEDProgram("ColorWipe") { TicksPerCycle = NUM_LEDS; StartLED = inStartLED;}
    bool Start();
    bool Update();
  private:
    int WiperPos;
    uint8_t HueOffset;
    uint16_t StartLED;
};

class Program_Chase: public CLEDProgram
{
  public:
    Program_Chase(const char *inName = "Chase", uint8_t inGapSize = 1, uint16_t inStartLED = 0) : CLEDProgram(inName) { GapSize = inGapSize; TicksPerCycle = NUM_LEDS/10; StartLED = inStartLED; }
    bool Start() { fill_solid(g_LEDS, NUM_LEDS, CRGB(0,0,0)); }
    bool Update() 
    {
      static uint8_t s_Offset = 0;      
      for (uint16_t i = StartLED; i < NUM_LEDS; i++)
      {
        if (((i + s_Offset) % (GapSize+1)) == 0)
        {
          g_LEDS[i] = CHSV(g_GlobalSettings.Hue, g_GlobalSettings.Saturation, 255);
        }
        else
        {
          g_LEDS[i] = CRGB(0,0,0);
        }
      }   
      if (g_GlobalSettings.Reverse)
      {
        s_Offset++;
      }
      else
      {
        s_Offset--;
      }
      return true;
    }
  private:
    uint8_t GapSize;
    uint16_t StartLED;
};

class Program_Twinkle : public CLEDProgram {
  public:
    Program_Twinkle() : CLEDProgram("Twinkle") {}
    bool Start();
    bool Update();
  private:
    CRGB BaseColor;
    CRGB PeakColor;
};


class Program_Glitter : public CLEDProgram {
  public:
    Program_Glitter() : CLEDProgram("Glitter") {}
    bool Start() { fill_solid(g_LEDS, NUM_LEDS, CRGB(0,0,0)); }
    bool Update()
    {
      fadeToBlackBy( g_LEDS, NUM_LEDS, 20);
      if ( random8() < 80) 
      {
        g_LEDS[ random16(NUM_LEDS) ] += CHSV(g_GlobalSettings.Hue, g_GlobalSettings.Saturation, 255);
      }
    }
  private:
};


class Program_Rainbow : public CLEDProgram
{
  public:
    Program_Rainbow() : CLEDProgram("Rainbow") { TicksPerCycle = 255; }
    bool Start()
    {
      HueStart = g_GlobalSettings.Hue;
      fill_rainbow(g_LEDS, NUM_LEDS, HueStart);  
      return true;
    }
    bool Update()
    {
      if (g_GlobalSettings.Reverse)
      {
        HueStart--;
      }
      else
      {
        HueStart++;
      }      
      fill_rainbow(g_LEDS, NUM_LEDS, HueStart);
      return (HueStart == g_GlobalSettings.Hue);
    }
  private:
    uint8_t HueStart;
};


class Program_Gradient : public CLEDProgram
{
  #define HUE_DELTA            64
  public:
    Program_Gradient() : CLEDProgram("Gradient")     { TicksPerCycle = NUM_LEDS;   }
    bool Start()
    {
      HueStartOffset = 0;
      return true;
    }
    bool Update()
    {
      for( int i = 0; i < NUM_LEDS; i++) 
      {
        uint16_t a = triwave8((((i + HueStartOffset) * 255) + (NUM_LEDS/2)) / NUM_LEDS);
        g_LEDS[i] = CHSV(g_GlobalSettings.Hue + (a * HUE_DELTA) / 255, 255, 255);
      }
      if (g_GlobalSettings.Reverse)
      {
        if (HueStartOffset == 0)
        {
          HueStartOffset = NUM_LEDS-1;
        }
        else
        {
          HueStartOffset--;
        }
      }
      else
      {
        if (++HueStartOffset >= NUM_LEDS)
        {
          HueStartOffset = 0;
        }
      } 
      return (HueStartOffset == 0);
    }
  private:
    uint8_t HueStartOffset;
};


class Program_Fire : public CLEDProgram
{
  public:
    Program_Fire() : CLEDProgram("Fire") {}
    bool Start();
    bool Update();
  private:
    uint8_t HueStart;
};


class Program_Meteor : public CLEDProgram
{
  public:
    Program_Meteor(); //const char *inName = "Meteor", bool inVariate = false) ;
    bool Start();
    bool Update();
    void Variate();
  private:
    uint8_t BaseHue;
    bool RandomDecay;
    byte TrailDecay;
    byte MeteorSize;
    uint8_t MeteorCount;
    int MeteorPos;
    int Direction;
    bool VariateEnabled;
};

class Program_Confetti : public CLEDProgram
{
  public:
    Program_Confetti() : CLEDProgram("Confetti") {}
    bool Start();
    bool Update();
    void Variate();
  private:
    uint8_t FadeAmount;
    uint8_t HueRange;
    uint8_t CurrentHue;
    uint8_t HueInc;
};


class Program_Juggle : public CLEDProgram
{
  public:
    Program_Juggle();
    bool Start();
    bool Update();
    void Variate();
  private:
    uint8_t NumDots;
    uint8_t FadeAmount;
};

class Program_Bubble : public CLEDProgram
{
  public:
    Program_Bubble() : CLEDProgram("Bubble") {}
    bool Start();
    bool Update();
    void Variate();
  private:
  
};

class Program_Magnets : public CLEDProgram
{
  public:
    Program_Magnets();
    bool Start();
    bool Update();
    void Variate();
  private:
  
};

DECLARE_GRADIENT_PALETTE( bhw2_xmas_gp );

class Program_Christmas: public CLEDProgram
{
  public:
    Program_Christmas() : CLEDProgram("Christmas") { TicksPerCycle = NUM_LEDS; }
    bool Update() 
    {
      static uint8_t s_Offset = 0;
      uint8_t BeatsPerMinute = 62;
      CRGBPalette16 palette = bhw2_xmas_gp;
      uint8_t beat = 0;//beatsin8( BeatsPerMinute, 0, 20);
      for( int i = 0; i < NUM_LEDS; i++) 
      {
        g_LEDS[i] = ColorFromPalette(palette, (((uint8_t)((4*i)+beat)+ s_Offset)), 255);
      }
      if (g_GlobalSettings.Reverse)
      {
         s_Offset--;
      }
      else
      {
        s_Offset++;
      }
      return true;
    }
  private:
    uint8_t GapSize;
    
};

#ifdef INCLUDE_PROGRAM_SOUND
class Program_Sound : public CLEDProgram
{
  public:
    Program_Sound() : CLEDProgram("Sound") { NoDelay = true; }
    bool Start();
    bool Update();
    bool Stop();
  private:
};
#endif //INCLUDE_PROGRAM_SOUND


#ifdef INCLUDE_PROGRAM_E131
class Program_E131 : public CLEDProgram
{
  public:
    Program_E131() : CLEDProgram("E131") { NoDelay = true; IncludeInAutoProgram = false; }
    bool Start();
    bool Update();
    bool Stop();
  private:
};
#endif //INCLUDE_PROGRAM_E131

#endif //PROGRAMS_H
