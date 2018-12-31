/*** INCLUDES ***/
#include "Programs.h"
#include "Settings.h"

/*** DEFINES ***/
//#define  MAGNET_DEBUG

#define POSITION_SHIFT      6             // shift factor between magnet position and LED position
#define LED_POS(idx)        (idx << POSITION_SHIFT)
#define LED_IDX(pos)        (pos >> POSITION_SHIFT)
#define MAX_POS             (LED_POS(NUM_LEDS+1)-1)

#define SPAWN_DELAY         40          // delay (ticks) before spawning a new magnet

//#define SIZE_TO_MASS(s)     (s/64)    // determine mass based on size
#define SIZE_TO_MASS(s)     (2)         // use fixed mass

#define POLARITY_REVERSAL_CHANCE    32    // chance for polarity reversal when 2 magnets collide

/*** TYPES ***/
typedef struct Magnet
{
  // Magnet Attributes
  int       Pos;
  uint16_t  Size;
  long      Accelleration;     // pos / tick^2
  long      Velocity;          // pos / tick
  uint8_t   Orientation;    // Magnet polarity

  // Magnet Operations
  void Spawn(int inPos, uint16_t inSize, uint8_t inOrientation)
  {
    Pos = inPos; 
    Size = inSize;
    Orientation = inOrientation;    
    Accelleration = 0;
    Velocity = 0;    
  }
  
  void ApplyForce(long inForce)
  {
    long lvMass = SIZE_TO_MASS(Size);
    if (((LED_IDX(Pos) <= 0) && (inForce < 0)) || ((LED_IDX(Pos+Size) >= (NUM_LEDS-1))) && (inForce > 0))
    {
      Accelleration = 0;
      Velocity = 0;
    }
    else
    {      
      // Newton's 2nd:
      // F = m*a
      // a = F/m
      long lvNewAccelleration = inForce / lvMass;
      
      if (abs(lvNewAccelleration) > abs(Accelleration))
      {
        // Only accept new accelleration if it is bigger than the one we already have
        Accelleration = lvNewAccelleration;
      }
    }
  }

  void Print()
  {
    Serial.print("Pos: ");
    Serial.print(Pos);
    Serial.print(" [");
    Serial.print(LED_IDX(Pos));
    Serial.print("]; Size: ");
    Serial.print(Size);
    Serial.print(" [");
    Serial.print(LED_IDX(Size));
    Serial.print("]; Acc: ");
    Serial.print(Accelleration);
    Serial.print("; Vel: ");
    Serial.print(Velocity);
  }
  
  void Move()
  {
    Velocity += Accelleration;        // v = a*t
    if (Velocity != 0)
    {
      if (Velocity > 0)
      {
        Pos += (Velocity >> 10);        // Scaling factor empirically chosen to have some decent speed
      }
      else if (Velocity < 0)
      {
        Pos -= (abs(Velocity) >> 10);   // Scaling factor empirically chosen to have some decent speed
      }
      
      if (Pos < 0)
      {
        // Hit the end stop. Reset accelleration and velocity to 0.
        Pos = 0;
        Accelleration = 0;
        Velocity = 0;
      }
      else if ((Pos + Size) > MAX_POS)
      {
        // Hit the end stop. Reset accelleration and velocity to 0.
        Pos = MAX_POS - Size;
        Accelleration = 0;
        Velocity = 0;
      }
    }
  }
  
  void Draw()
  {
    uint16_t lvLedPos = LED_IDX(Pos);
    uint16_t lvLedSize = LED_IDX(Size);
    
    uint8_t lvHues[] = {g_GlobalSettings.Hue, g_GlobalSettings.Hue - 255/3};
    
    for (int i = lvLedPos; i < (lvLedPos + lvLedSize); i++)
    {
      if ((i >= 0) && (i < NUM_LEDS))
      {
        g_LEDS[i] = CHSV(lvHues[(i-lvLedPos+Orientation) % 2], 255, 255);
      }
    }
  }
} Magnet;

/*** PRIVATE FUNCTIONS ***/
static bool Magnets_Attract(Magnet *inMagnet1, Magnet *inMagnet2)
{
  if (inMagnet1->Pos > inMagnet2->Pos)
  {
    // Swap arguments so Magnet1 is to the left of Magnet2
    Magnet *lvTemp = inMagnet1;
    inMagnet1 = inMagnet2;
    inMagnet2 = lvTemp;
  }

  // Determine attractive force between the magnets, based on the distance between them.
  long lvDistance = (long)inMagnet2->Pos - inMagnet1->Pos + inMagnet1->Size;
  long lvForce = 0;
  
  if (lvDistance > 0)
  {
    // Magnetic force (F) is inversely proportional to distance squared (r)
    // F = 1 / (r^2)
    lvForce = (1L << 20)/(lvDistance /* *lvDistance */ );   // Scaling factor (1 << 30) added to maintain resolution
  }
  
  if (inMagnet1->Orientation != inMagnet2->Orientation)
  {
    // Magnets have opposite orientation (reverse polarity): repel instead of attract
    lvForce = -lvForce;
  }
  
  inMagnet1->ApplyForce(lvForce);
  inMagnet2->ApplyForce(-lvForce);  // Newton's 3rd
  
  return (lvForce != 0);
}

static bool Magnets_DetectCollision(Magnet *inMagnet1, Magnet *inMagnet2)
{
  if (inMagnet1->Pos > inMagnet2->Pos)
  {
    // Swap arguments so Magnet1 is to the left of Magnet2
    Magnet *lvTemp = inMagnet1;
    inMagnet1 = inMagnet2;
    inMagnet2 = lvTemp;
  }
  if (LED_IDX(inMagnet1->Pos + inMagnet1->Size) >= LED_IDX(inMagnet2->Pos))
  {
    // Collision: Clip position of Magnet1
    inMagnet1->Pos = LED_POS(LED_IDX(inMagnet2->Pos - inMagnet1->Size));
    if (inMagnet1->Pos < 0)
    {
      inMagnet1->Pos = 0;
    }  
    return true;
  }
  return false;
}

static bool Magnets_Merge(Magnet *inMagnet1, Magnet *inMagnet2)
{
  inMagnet1->Pos = min(inMagnet1->Pos, inMagnet2->Pos);
  inMagnet1->Size += inMagnet2->Size;
  inMagnet1->Accelleration = 0;
  inMagnet1->Velocity = 0;

  inMagnet2->Pos = -1;
  inMagnet2->Size = 0;
  inMagnet2->Accelleration = 0;
  inMagnet2->Velocity = 0;

   // chance for polarity reversal          
  if (random8() < POLARITY_REVERSAL_CHANCE)
  { 
    uint8_t lvIdx = random(2);

#ifdef MAGNET_DEBUG    
    Serial.println("Polarity Swap!");    
#endif //MAGNET_DEBUG

    inMagnet1->Orientation = 1 - inMagnet1->Orientation;
  }
  return true;
}

/*** STATIC VARIABLES ***/
static Magnet s_Magnets[3];
static uint8_t s_MagnetCount = 0;
static uint8_t s_Delay = 0;
enum { STATE_INITIAL, STATE_SPAWN, STATE_ATTRACT} s_State;

/*** CLASS FUNCTIONS ***/
Program_Magnets::Program_Magnets() : CLEDProgram("Magnets") 
{
  TicksPerCycle = NUM_LEDS;
}
  
bool Program_Magnets::Start()
{
  s_State = STATE_INITIAL;
  return true;
}


bool Program_Magnets::Update()
{
  bool lvIdle = true;
  
  // clear all LEDs
  fill_solid(g_LEDS, NUM_LEDS, CRGB(0,0,0));
  
  if (s_MagnetCount > 0)
  {
    for (int i = 0; i < s_MagnetCount; i++)
    {
      // Move magnet according to it's velocity and accelleration
      s_Magnets[i].Move();
      
      if (i < (s_MagnetCount - 1))
      {
        // Calculate magnetic attraction force between 2 adjacent magnets
        Magnets_Attract(&s_Magnets[i], &s_Magnets[i+1]);

        //Determine if magnets have collided
        if (Magnets_DetectCollision(&s_Magnets[i], &s_Magnets[i+1]))
        {
          // When magnets collide, merge them into one bigger magnet          
          Magnets_Merge(&s_Magnets[i], &s_Magnets[i+1]);
          
          if (i < (s_MagnetCount - 2))
          {
            // magnet was removed from middle of array, shift the right side of the array to fill the gap
            s_Magnets[i+1] = s_Magnets[i+2];
          }
          s_MagnetCount--;
        }
      }
      
      if (s_Magnets[i].Velocity != 0)
      {
        // magnet is still moving, not idle
        lvIdle = false;
      }
      
      // update LEDs, based on magnet position and size
      s_Magnets[i].Draw();

#ifdef MAGNET_DEBUG
        Serial.print("Magnet[");
        Serial.print(i);
        Serial.print("]: ");
        s_Magnets[i].Print();
        Serial.print(";  ");
#endif //MAGNET_DEBUG        
    }    
#ifdef MAGNET_DEBUG    
     Serial.println();
#endif //MAGNET_DEBUG     
  }

  switch (s_State)
  {
    case STATE_INITIAL:
      s_MagnetCount = 0;
      s_Delay = 0;
      s_State = STATE_SPAWN;
      break;
    case STATE_SPAWN:
      if (s_Delay-- == 0)
      {
        if (s_MagnetCount < 3)
        {
          // find largest open section
          uint16_t lvOpenSectionStart = 0;
          uint16_t lvOpenSectionSize = 0;
          for (int i = 0; i < NUM_LEDS; i++)
          {
            uint16_t lvCurrentSectionStart = i;
            uint16_t lvCurrentSectionSize = 0;
            while ((!g_LEDS[i]) && (i < NUM_LEDS))
            {
              // empty position
              lvCurrentSectionSize++;
              i++;
            }
            if ((lvCurrentSectionSize > 0) && (lvCurrentSectionSize > lvOpenSectionSize))
            {
              // keep track of largest found section
              lvOpenSectionStart = lvCurrentSectionStart;
              lvOpenSectionSize = lvCurrentSectionSize;
            }
          }
          
          if (lvOpenSectionSize < 4)
          {
             // largest section is too small. Restart program.
             s_State = STATE_INITIAL;
             break;
          }

          // Determine a random location inside the open section
          uint16_t lvMagnetIdx = random8(lvOpenSectionStart+1, lvOpenSectionStart+lvOpenSectionSize-2);          

          // To maintain order in the s_Magnets array we determine the position where to insert the new magnet, based on its position.
          uint8_t lvArrayIdx = s_MagnetCount;
          if (s_MagnetCount > 0)
          {
            for (int i = 0; i < s_MagnetCount; i++)
            {
              if (LED_POS(lvMagnetIdx) < s_Magnets[i].Pos)
              {
                lvArrayIdx = i;
                // shift rest of array
                for (int j = s_MagnetCount; j > lvArrayIdx; j--)
                {
                  s_Magnets[j] = s_Magnets[j-1];
                }
                break;
              }
            }            
          }

          // Spawn magnet
          s_Magnets[lvArrayIdx].Spawn(LED_POS(lvMagnetIdx), LED_POS(2), random(2));   // randomize magnet orientation
          s_MagnetCount++;

#ifdef MAGNET_DEBUG  
          Serial.print("Spawn Magnet @ Array Index ");
          Serial.print(lvArrayIdx);
          Serial.print(" -> ");
          s_Magnets[lvArrayIdx].Print();
          Serial.println();
#endif //MAGNET_DEBUG  
  
//          if (s_MagnetCount > 1)
//          {
//            s_State = STATE_ATTRACT;
//          }
//          else
          {
            // Only one magnet: spawn another one
            s_Delay = random(10,SPAWN_DELAY+10);
            s_State = STATE_SPAWN;
          }
        }
      }
      break;
//    case STATE_ATTRACT:
//      if ((s_MagnetCount < 2))
//      {
//        // Less than 2 magnets: spawn another one
//        s_Delay = SPAWN_DELAY;
//        s_State = STATE_SPAWN;
//      }
//      else
//      {
//        s_Delay = SPAWN_DELAY+30;
//        s_State = STATE_SPAWN;
        // 2 or more magnets: wait for motion to stop
//        if (lvIdle)
//        {
//          if (s_Delay == 0)
//          {
//            s_Delay = SPAWN_DELAY;
//            s_State = STATE_SPAWN;
//          }
//          else
//          {
//            s_Delay--;
//          }
//        }
//        else
//        {
//          s_Delay = 10;
//        }
//      }
//      break;
  }
  return true;
}
