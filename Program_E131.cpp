#include "Programs.h"
#include "Settings.h"

#ifdef INCLUDE_PROGRAM_E131

#include <WiFi.h>
#include <ESPAsyncE131.h>


#define E131_LEDSTRIP1_CH_START  1
#define E131_LEDSTRIP1_CH_END    (E131_LEDSTRIP1_CH_START+(LEDSTRIP1_NUM_LEDS*3))
#define E131_LEDSTRIP2_CH_START  (E131_LEDSTRIP1_CH_END+1)
#define E131_LEDSTRIP2_CH_END    (E131_LEDSTRIP2_CH_START+(LEDSTRIP2_NUM_LEDS*3))
#define E131_LEDSTRIP3_CH_START  (E131_LEDSTRIP2_CH_END+1)
#define E131_LEDSTRIP3_CH_END    (E131_LEDSTRIP3_CH_START+(LEDSTRIP3_NUM_LEDS*3))

#define E131_UNIVERSE(ch)         (((uint8_t)ch / E131_MAX_CHANNELS_PER_UNIVERSE)+1)

#define _CONCAT(a,b,c)             a##b##c

#define _E131_LEDSTRIP_CH_START(d) _CONCAT(E131_LEDSTRIP,d,_CH_START)
#define E131_LEDSTRIP_CH_START     _E131_LEDSTRIP_CH_START(DEVICENR)
#define _E131_LEDSTRIP_CH_END(d)   _CONCAT(E131_LEDSTRIP,d,_CH_END)
#define E131_LEDSTRIP_CH_END       _E131_LEDSTRIP_CH_END(DEVICENR)

#define E131_UNIVERSE_START        E131_UNIVERSE(E131_LEDSTRIP_CH_START)
#define E131_UNIVERSE_END          E131_UNIVERSE(E131_LEDSTRIP_CH_END)
#define E131_UNIVERSE_COUNT       (E131_UNIVERSE_START-E131_UNIVERSE_END+1)

#define E131_CH_OFFSET            (E131_LEDSTRIP_CH_START - ((E131_UNIVERSE_START-1)*E131_MAX_CHANNELS_PER_UNIVERSE))


// ESPAsyncE131 instance with UNIVERSE_COUNT buffer slots
static ESPAsyncE131 s_E131(E131_UNIVERSE_COUNT);

 
  
bool Program_E131::Start()
{
 // Choose one to begin listening for E1.31 data
  //if (s_E131.begin(E131_UNICAST))                               // Listen via Unicast
  if (s_E131.begin(E131_MULTICAST, E131_UNIVERSE_START, E131_UNIVERSE_COUNT))   // Listen via Multicast
  {
      Serial.println(F("Listening for data..."));
      Serial.printf("\nE131_UNIVERSE_START: %d\nE131_UNIVERSE_COUNT: %d\nE131_CH_OFFSET: %d\n", E131_UNIVERSE_START, E131_UNIVERSE_COUNT, E131_CH_OFFSET);
  }
  else 
  {
    Serial.println(F("*** e131.begin failed ***"));
    return false;
  }
      
  return true;
}

bool Program_E131::Stop()
{
  return true;
}

bool Program_E131::Update()
{    
  if (!s_E131.isEmpty()) 
  {
        e131_packet_t lvPacket;
        s_E131.pull(&lvPacket);     // Pull packet from ring buffer

        
        uint8_t lvUniverse = htons(lvPacket.universe);
        //Serial.printf("Universe %u / %u Channels | Packet#: %u / Errors: %u / CH1: %u\n",
        //        htons(lvPacket.universe),                 // The Universe for this packet
        //        htons(lvPacket.property_value_count) - 1, // Start code is ignored, we're interested in dimmer data
        //        s_E131.stats.num_packets,                 // Packet counter
        //        s_E131.stats.packet_errors,               // Packet error counter
        //        lvPacket.property_values[1]);             // Dimmer data for Channel 1
        if ((lvUniverse >= E131_UNIVERSE_START) && (lvUniverse <= E131_UNIVERSE_END))
        {
          // We might be interested in this packet
          int16_t lvPacketDataSize = htons(lvPacket.property_value_count) - 1;
          if (lvPacketDataSize > E131_MAX_CHANNELS_PER_UNIVERSE)
          {
            lvPacketDataSize = E131_MAX_CHANNELS_PER_UNIVERSE;
          }
          
          uint8_t lvUniverseOffset = lvUniverse - E131_UNIVERSE_START;
          uint16_t lvPacketDataOffset = 0;
          if (lvUniverseOffset == 0)
          {
            // offset only applies for first universe
            lvPacketDataOffset = E131_CHANNEL_START - 1;
            lvPacketDataSize = lvPacketDataSize - lvPacketDataOffset + 1;
          }

          int16_t lvLedDataOffset = lvUniverseOffset * E131_MAX_CHANNELS_PER_UNIVERSE - lvPacketDataOffset;
          if (lvLedDataOffset < 0)
          {
            lvLedDataOffset = 0;
          }

          if ((lvLedDataOffset + lvPacketDataSize) > (DEFAULT_NUM_LEDS*3))
          {
            lvPacketDataSize = (DEFAULT_NUM_LEDS*3) - lvLedDataOffset;
          }
                    
          uint8_t *lvDataPtr = lvPacket.property_values + 1 + lvPacketDataOffset;
          memcpy((uint8_t*)g_LEDS+lvLedDataOffset, lvDataPtr, lvPacketDataSize);

          //Serial.printf("Universe: %u, PacketDataOffset: %d, PacketDataSize: %d, LedDataOffset, :%d\n", lvUniverse, lvPacketDataOffset, lvPacketDataSize, lvLedDataOffset);


        }

    }
  
  return true;
}

#endif //INCLUDE_PROGRAM_E131
