#include "Programs.h"
#include "Settings.h"


// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100 
#define COOLING  8 //55

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 100 //120


#define HEAT_ARRAY_SIZE     50

static byte s_Heat[HEAT_ARRAY_SIZE];

bool Program_Fire::Start()
{
  memset(s_Heat, 0, sizeof(s_Heat));
  return true;
}

bool Program_Fire::Update()
{
  // Array of temperature readings at each simulation cell
  

  // Step 1.  Cool down every cell a little
  for( int i = 0; i < HEAT_ARRAY_SIZE; i++) 
  {
    s_Heat[i] = qsub8( s_Heat[i],  random8(0, ((COOLING * 10) / HEAT_ARRAY_SIZE) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  //for( int k = (NUM_LEDS - 1); k >= 2; k--) 
  //for( int k = 2; k < (NUM_LEDS-2); k++) 
  //{
    //s_Heat[k] = (s_Heat[k - 1] + s_Heat[k - 2] + s_Heat[k] + s_Heat[k + 1] + s_Heat[k + 2]) / 5;
  //}
    
  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if( random8() < SPARKING ) 
  {
    int y = random16(HEAT_ARRAY_SIZE);
    if (s_Heat[y] < 90)
    {
      s_Heat[y] = qadd8( s_Heat[y], random8(100,150) );
    }
  }

  // Step 4.  Map from heat cells to LED colors
  for( int j = 0; j < NUM_LEDS; j++) 
  {
    CRGB color = HeatColor( s_Heat[j % HEAT_ARRAY_SIZE]);
    g_LEDS[j] = color;
  }

  blur1d(g_LEDS, NUM_LEDS, 64);  
  return true;
}
