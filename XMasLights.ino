// Board: ESP32 Dev Module

/*** INCLUDES ***/
#include "Settings.h"
#include "Programs.h"

#include "WiFi_MQTT.h"

// Gradient palette "bhw2_xmas_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/bhw/bhw2/tn/bhw2_xmas.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 48 bytes of program space.

DEFINE_GRADIENT_PALETTE( bhw2_xmas_gp ) {
    0,   0, 12,  0,
   40,   0, 55,  0,
   66,   1,117,  2,
   77,   1, 84,  1,
   81,   0, 55,  0,
  119,   0, 12,  0,
  153,  42,  0,  0,
  181, 121,  0,  0,
  204, 255, 12,  8,
  224, 121,  0,  0,
  244,  42,  0,  0,
  255,  42,  0,  0};
    
/*** TYPE DEFINITIONS ***/
typedef void (*LEDPatternFcn)(void);

/*** FORWARD DECLARATIONS ***/
static void LEDPattern_Sparkles(void);

/*** GLOBALS ***/
CRGB g_LEDS[DEFAULT_NUM_LEDS];

GlobalSettings g_GlobalSettings =
{
  .Enabled = true,
  
  .Speed = DEFAULT_SPEED,
  .Brightness = DEFAULT_BRIGHTNESS,
  .Hue = DEFAULT_HUE,
  .Saturation = DEFAULT_SATURATION,
  
  .AutoCycleHue = AUTO_CYCLE_HUE,  
  .AutoCycleHueDelayMs = AUTO_HUE_CYCLE_TIME_MS,
  
  .AutoCyclePrograms = AUTO_CYCLE_PROGRAMS,
  .AutoProgramDelaySec = PROGRAM_CYCLE_TIME_SEC,

  .Mirror = DEFAULT_MIRROR_MODE,
  .Reverse = DEFAULT_REVERSE_MODE
};

CLEDProgram *g_LEDPrograms[] = {
  new Program_Solid("Solid", 1),
  new Program_Solid("Solid2", 2),
  new Program_Solid("Solid3", 3),  
#ifdef LEDSTRIP4  
  new Program_Solid("Solid1a", 1, 1),
  new Program_Solid("Solid2a", 2, 1),
  new Program_Solid("Solid3a", 3, 1),  
  new Program_Solid("Solid6a", 6, 1),  
#endif //LEDSTRIP4
#ifndef LEDSTRIP4    
  new Program_Chase("Chase", 1),
  new Program_Chase("Chase2", 5),
  new Program_Chase("Chase3", 10),
#else 
  new Program_Chase("Chase", 1, 0),
  new Program_Chase("Chase2", 2, 1),
  new Program_Chase("Chase3", 1, 1),
#endif //LEDSTRIP4    
  new Program_Breathe,
  new Program_Strobe,
  new Program_ColorWipe(START_LED),
  new Program_Rainbow, 
  new Program_Gradient, 
#ifndef LEDSTRIP4  
  new Program_Twinkle, 
  new Program_Glitter,
  new Program_Fire, 
  new Program_Meteor,//("Meteor", false),
  //new Program_Meteor("Meteor2", true), 
  new Program_Confetti, 
  new Program_Juggle,
  new Program_Bubble,
  new Program_Magnets, 
  new Program_Christmas
#endif //LEDSTRIP4  
#ifdef INCLUDE_PROGRAM_SOUND
  ,new Program_Sound
#endif //INCLUDE_PROGRAM_SOUND
#ifdef INCLUDE_PROGRAM_E131
  ,new Program_E131
#endif //INCLUDE_PROGRAM_E131
};

#ifdef WIFI_ENABLED
CLEDProgram *g_Program_Connecting = new Program_Connecting();
#endif WIFI_ENABLED

#define NUM_PROGRAMS    (sizeof(g_LEDPrograms)/sizeof(g_LEDPrograms[0]))
const uint8_t g_NumPrograms = NUM_PROGRAMS;
uint16_t g_NumLeds = DEFAULT_NUM_LEDS;

CLEDProgram *g_CurrentProgram = g_LEDPrograms[0];

static int8_t s_ProgramIndex = -1;
int8_t g_NextProgramIndex = 0;

#ifdef ENABLE_PROFILING
  static unsigned long s_DurationMin;
  static unsigned long s_DurationMax;
  static unsigned long s_DurationSum;
  static unsigned long s_DurationCount;
#endif //ENABLE_PROFILING


/*** SETUP ***/
void setup() 
{
  // 1 sec startup delay
  delay(1000);

  #ifdef ENABLE_DEBUG
    Serial.begin(115200);
  #endif
  
  // Set LED strip configuration
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(g_LEDS, DEFAULT_NUM_LEDS).setCorrection(TypicalLEDStrip);

  // Limit current
  FastLED.setMaxPowerInVoltsAndMilliamps(LED_VOLTAGE,LED_MAX_CURRENT_MA); 

  // Set master brightness control
  FastLED.setBrightness(g_GlobalSettings.Brightness);

#ifdef WIFI_ENABLED
  WiFi_MQTT_Init();
#endif // WIFI_ENABLED


  #ifdef HAS_DIGITAL_INPUTS
    pinMode(DIGITAL_IN_0, INPUT);
    pinMode(DIGITAL_IN_1, INPUT);
  #endif //HAS_DIGITAL_INPUTS
  
  #ifdef ENABLE_PROFILING
    s_DurationSum = 0;
    s_DurationCount = 0;
    s_DurationMin = 9999999;
    s_DurationMax = 0;
  #endif // ENABLE_PROFILING


  Serial.println("Program List:");
  for (int i = 0; i < NUM_PROGRAMS; i++)
  {
    Serial.print(i);
    Serial.print(": ");
    Serial.println(g_LEDPrograms[i]->Name);
  }
}

/*** MAIN LOOP ***/
void loop() 
{
  static unsigned long s_LastRunTimeMs = 0;
  static unsigned long s_LastProgramStartTimeMs = 0;
  static unsigned long s_LastHueChangeTimeMs = 0;
  static bool s_WasEnabled = true;
  
#ifdef WIFI_ENABLED
  static bool s_SendUpdate = false;
  WiFi_MQTT_Tick();
#endif // WIFI_ENABLED

  if (!g_GlobalSettings.Enabled)
  {
    if (s_WasEnabled)
    {
      Serial.println(F("Output Disabled")); 
      FastLED.clear();
      FastLED.show();
    }
    s_WasEnabled = false;
  }
  else
  {
    bool lvDoUpdate = false;
    bool lvRunDone = false;
    if (!s_WasEnabled)
    {
      s_LastHueChangeTimeMs = millis();
      s_LastProgramStartTimeMs = millis();
      lvDoUpdate = true;
    }
    s_WasEnabled = true;

    // Handle program change

    #ifdef WIFI_ENABLED
      if (!WiFi_MQTT_IsConnected())
      {
        g_CurrentProgram = g_Program_Connecting;
        s_ProgramIndex = -1;
      }
      else
    #endif WIFI_ENABLED  
    if (g_NextProgramIndex != s_ProgramIndex)
    { 
      g_CurrentProgram->Stop();

      // change program
      s_ProgramIndex = g_NextProgramIndex;
      g_CurrentProgram = g_LEDPrograms[s_ProgramIndex];
      
      // Start new program
      #ifdef ENABLE_DEBUG
        Serial.print(F("Starting Program: ")); 
        Serial.print(s_ProgramIndex);
        Serial.println();
      #endif // ENABLE_DEBUG    
      
      g_CurrentProgram->Start();
      s_LastProgramStartTimeMs = millis();

      // Force Update
      lvDoUpdate = true;
      
      #ifdef WIFI_ENABLED
        s_SendUpdate = true;
      #endif // WIFI_ENABLED
    }
 
    // Insert delay if not running at max speed
    if ((g_GlobalSettings.Speed < MAX_SPEED) && (!g_CurrentProgram->NoDelay))
    {
      unsigned long lvUpdatePeriodMs = g_CurrentProgram->GetUpdatePeriod(g_GlobalSettings.Speed);
      // insert a delay to keep the framerate modest
      #ifdef USE_NONBLOCKING_DELAY
        // non-blocking delay      
        if ((millis() - s_LastRunTimeMs) >= lvUpdatePeriodMs)
        {
          lvDoUpdate = true;
        }
      #else
        // blocking delay
        FastLED.delay(1000UL/(2*g_GlobalSettings.Speed)); 
        lvDoUpdate = true;
      #endif // USE_NONBLOCKING_DELAY
    }
    else if (g_GlobalSettings.Speed == 0)
    {
      // stop updating
      lvDoUpdate = false;
    }
    else
    {
      // max speed
      lvDoUpdate = true;
    }
    if (lvDoUpdate)
    {
      unsigned long lvPeriod = millis() - s_LastRunTimeMs;
      s_LastRunTimeMs = millis();

      if (FastLED.getBrightness() != g_GlobalSettings.Brightness)
      {
        FastLED.setBrightness(g_GlobalSettings.Brightness);
      }
      
      #ifdef ENABLE_PROFILING
        unsigned long lvDuration;
        lvDuration = micros();
      #endif    
            
      lvRunDone = g_CurrentProgram->Update();        

      if (g_GlobalSettings.Mirror)
      {
        NUM_LEDS = DEFAULT_NUM_LEDS / 2;
        for (int i = 0; i < NUM_LEDS; i++)
        {
          g_LEDS[DEFAULT_NUM_LEDS-1-i] = g_LEDS[i];
        }
      }
      else
      {
        NUM_LEDS = DEFAULT_NUM_LEDS;
      }

      FastLED.show();
      
      #ifdef ENABLE_PROFILING
        lvDuration = micros() - lvDuration;
        s_DurationSum += lvDuration;
        s_DurationCount++;
        if (lvDuration < s_DurationMin)
        {
          s_DurationMin = lvDuration;
        }
        if (lvDuration > s_DurationMax)
        {
          s_DurationMax = lvDuration;
        }
        EVERY_N_MILLISECONDS(1000) 
        { 
          Serial.print(F("Count: ")); 
          Serial.print(s_DurationCount);
          Serial.print(F("; Avg: ")); 
          Serial.print((float)s_DurationSum / s_DurationCount);
          Serial.print(F("; Min: ")); 
          Serial.print(s_DurationMin);
          Serial.print(F("; Max: ")); 
          Serial.print(s_DurationMax);
          Serial.print(F("; Period: ")); 
          Serial.print(lvPeriod);
          Serial.print(F("; Freq: ")); 
          Serial.println(1000.0f / lvPeriod);
          
          s_DurationSum = 0;
          s_DurationCount = 0;
          s_DurationMin = 9999999;
          s_DurationMax = 0;
        }
      #endif // ENABLE_PROFILING
    }
    else
    {
      // call show anyway for temporal dithering
     // FastLED.show();
    }

    // Handle automatic Hue/Program change
    if (g_GlobalSettings.AutoCycleHue)
    {
      if ((millis() - s_LastHueChangeTimeMs) >= g_GlobalSettings.AutoCycleHueDelayMs)
      {
        g_GlobalSettings.Hue++;        
        s_LastHueChangeTimeMs = millis();
        #ifdef WIFI_ENABLED
          //s_SendUpdate = true;
        #endif // WIFI_ENABLED
      }
    }
    if (g_GlobalSettings.AutoCyclePrograms)
    {
      if ((millis() - s_LastProgramStartTimeMs) >= (1000UL*g_GlobalSettings.AutoProgramDelaySec))
      {
        do
        {
          g_NextProgramIndex++;
          if (g_NextProgramIndex >= NUM_PROGRAMS)
          {
            g_NextProgramIndex = 0;
          }      
        } while (!g_LEDPrograms[g_NextProgramIndex]->IncludeInAutoProgram);
        s_LastProgramStartTimeMs = millis();
      }
    }
    
  }

  // Process Serial input
  #ifdef ENABLE_DEBUG
//    EVERY_N_MILLISECONDS(5000)
//    {
//      Serial.print("FPS: ");
//      Serial.println(FastLED.getFPS());      
//    }
    // Basic CLI
    int lvRecvByte = Serial.read();
    if (lvRecvByte >= '0' && lvRecvByte < ('0' + NUM_PROGRAMS))
    {
      g_NextProgramIndex = lvRecvByte - '0';
    }
    else if (lvRecvByte == '+')
    {
      // speed up
      if (g_GlobalSettings.Speed < (MAX_SPEED - 10))
      {
        g_GlobalSettings.Speed += 10;
      }
      else
      {
        g_GlobalSettings.Speed = MAX_SPEED;
      }
      Serial.print(F("Speed: ")); 
      Serial.print(g_GlobalSettings.Speed);
      Serial.println();
    }
    else if (lvRecvByte == '-')
    {
      // slow down
      if (g_GlobalSettings.Speed > (MIN_SPEED + 10))
      {
        g_GlobalSettings.Speed -= 10;
      }
      else
      {
        g_GlobalSettings.Speed = MIN_SPEED;        
      }
      Serial.print(F("Speed: ")); 
      Serial.print(g_GlobalSettings.Speed);
      Serial.println();
    }
    else if (lvRecvByte == '>')
    {
      // shift base Hue up
      g_GlobalSettings.Hue += 10;
      Serial.print(F("Hue: ")); 
      Serial.print(g_GlobalSettings.Hue);
      Serial.println();
      g_CurrentProgram->Start();
    }
    else if (lvRecvByte == '<')
    {
      // shift base Hue down
      g_GlobalSettings.Hue -= 10;
      Serial.print(F("Hue: ")); 
      Serial.print(g_GlobalSettings.Hue);
      Serial.println();
      g_CurrentProgram->Start();
    }
    else if (lvRecvByte == '*')
    {
      g_GlobalSettings.AutoCyclePrograms = !g_GlobalSettings.AutoCyclePrograms;
      Serial.print(F("AutoCyclePrograms: ")); 
      Serial.print(g_GlobalSettings.AutoCyclePrograms);
      Serial.println();
    }
  #endif

  // Process analog inputs
  #ifdef HAS_ANALOG_INPUTS
    EVERY_N_MILLISECONDS(20)
    {
      uint16_t lvAnalog0 = analogRead(ANALOG_IN_0);
      uint16_t lvAnalog1 = analogRead(ANALOG_IN_1);
      static uint8_t s_AnalogSampleCount = 0;
      static uint16_t s_Analog0Sum = 0;
      static uint16_t s_Analog1Sum = 0;

      s_Analog0Sum+=lvAnalog0;
      s_Analog1Sum+=lvAnalog1;
      s_AnalogSampleCount++;
      
      if (s_AnalogSampleCount >= 4)
      {
        static int s_PrevAnalogIn0 = 1024;
        static int s_PrevAnalogIn1 = 1024;
        lvAnalog0 = s_Analog0Sum >> 4;
        lvAnalog1 = s_Analog1Sum >> 4;
        s_AnalogSampleCount = 0;
        s_Analog0Sum = 0;
        s_Analog1Sum = 0;
        if (abs(s_PrevAnalogIn0 - lvAnalog0) > 2)
        {
          g_GlobalSettings.Speed = constrain(lvAnalog0, MIN_SPEED, MAX_SPEED);
          Serial.print("Speed: ");
          Serial.print(g_GlobalSettings.Speed);
          
          unsigned long lvUpdatePeriodMs1 = ((MAX_CYCLE_TIME_MS * (255 - g_GlobalSettings.Speed)) / 255) / g_CurrentProgram->TicksPerCycle;
          Serial.print("; Period: ");
          Serial.println(lvUpdatePeriodMs1);
          
          s_PrevAnalogIn0 = lvAnalog0;
        }
        if (abs(s_PrevAnalogIn1 - lvAnalog1) > 2)
        {
          if (lvAnalog1 < 250)
          {
            g_GlobalSettings.Hue = constrain(lvAnalog1, 0, 255);
            Serial.print("Hue: ");
            Serial.println(g_GlobalSettings.Hue);
            g_GlobalSettings.AutoCycleHue = false;
          }
          else
          {
            Serial.print("Auto Hue Enabled");
            g_GlobalSettings.AutoCycleHue = true;
          }
          s_PrevAnalogIn1 = lvAnalog1;
        }
      }
    }
  #endif //HAS_ANALOG_INPUTS

  // Process digital inputs
  #ifdef HAS_DIGITAL_INPUTS
    //EVERY_N_MILLISECONDS(200)
    {
      static uint8_t s_PrevDigIn0 = 1;
      static uint8_t s_PrevDigIn1 = 1;
      static unsigned long s_PressTime0 = 0;
      static unsigned long s_PressTime1 = 0;
        
      uint8_t lvDigIn0 = digitalRead(DIGITAL_IN_0);
      uint8_t lvDigIn1 = digitalRead(DIGITAL_IN_1);
    
      if (s_PrevDigIn0 != lvDigIn0)
      {
        //Serial.print("DIN0: ");
        //Serial.println(lvDigIn0);
        s_PrevDigIn0 = lvDigIn0;
        if (lvDigIn0 == 1)
        {
          uint8_t lvBrighness = FastLED.getBrightness();
          lvBrighness += 16;
          FastLED.setBrightness(lvBrighness);
          Serial.print("Brighness: ");
          Serial.println(lvBrighness);
        }
      }
      if (s_PrevDigIn1 != lvDigIn1)
      {
        //Serial.print("DIN1: ");
        //Serial.println(lvDigIn1);
        s_PrevDigIn1 = lvDigIn1;
        if (lvDigIn1 == 1)
        {
          if (millis() - s_PressTime1 >= LONG_PRESS_TIME_MS)
          {
            g_GlobalSettings.AutoCyclePrograms = true;
          }
          else
          {
            // goto next program
            g_NextProgramIndex++;
            g_GlobalSettings.AutoCyclePrograms = false;
          }
        }
        else 
        {
          s_PressTime1 = millis();
        }
      }
    }
  #endif //HAS_ANALOG_INPUTS
  #ifdef WIFI_ENABLED
    EVERY_N_MILLISECONDS(500)
    {
      if (s_SendUpdate)
      {
        MQTT_SendState();
        s_SendUpdate = false;
      }
    }
  #endif // WIFI_ENABLED  
}


/*** PRIVATE FUNCTIONS ***/
