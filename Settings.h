#ifndef SETTINGS_H
#define SETTINGS_H

/*** DEVICE SELECTION ***/
//#define LEDSTRIP1
#define LEDSTRIP2
//#define LEDSTRIP3

/*** BOARD SELECTION ***/
#define BOARD_ESP32
//#define BOARD_ARDUINO_NANO
//#define BOARD_ARDUINO_NANO_HW_CONTROLS

/*** LED STRIP SELECTION ***/

#define LEDSTRIP1_NUM_LEDS          120
#define LEDSTRIP2_NUM_LEDS          300
#define LEDSTRIP3_NUM_LEDS          200

#define E131_MAX_CHANNELS_PER_UNIVERSE    510

#ifdef LEDSTRIP1
  #define DEVICENAME          "ledstrip1"
  #define DEVICENR            1
  
  // 2x CHINLY WS2812B Ledlichtstrip SMD5050 RGB, 1 meter, 60 leds,
  #define LED_TYPE            WS2812
  #define COLOR_ORDER         GRB
  #define DEFAULT_NUM_LEDS    LEDSTRIP1_NUM_LEDS
  
  #define E131_UNIVERSE_START 1                // First DMX Universe to listen for
  #define E131_UNIVERSE_END   1                // Last DMX Universe to listen for
  #define E131_CHANNEL_START  1                // First channel in first universe
#endif
#ifdef LEDSTRIP2
  #define DEVICENAME          "ledstrip2"
  #define DEVICENR            2
  
  // CHINLY WS2812B Ledlichtstrip SMD5050 RGB, 5 meter, 300 leds
  #define LED_TYPE            WS2812
  #define COLOR_ORDER         GRB
  #define DEFAULT_NUM_LEDS    LEDSTRIP2_NUM_LEDS
  
  #define E131_UNIVERSE_START 1                // First DMX Universe to listen for
  #define E131_UNIVERSE_END   3                // Last DMX Universe to listen for
  #define E131_CHANNEL_START  (120*3)          // First channel in first universe
  
#endif
#ifdef LEDSTRIP3
  #define DEVICENAME          "ledstrip3"
  #define DEVICENR            3
  
    // 4x 50 PC 's ws2811 RGB Full Color 12 mm Pixel Digital addressable LED String DC 5 V    
  #define LED_TYPE            WS2811
  #define COLOR_ORDER         RGB
  #define DEFAULT_NUM_LEDS    LEDSTRIP3_NUM_LEDS
  
#endif

#define DEVICETYPE        "ledstrip"
#define GROUPNAME         "group"

#define LED_VOLTAGE           5
#define LED_MAX_CURRENT_MA    3800

#ifdef BOARD_ESP32
  #define FASTLED_INTERRUPT_RETRY_COUNT 0
  #define WIFI_ENABLED
  //#define INCLUDE_PROGRAM_E131
#endif //BOARD_ESP32

#define FASTLED_INTERNAL  // suppress FastLED pragma message warning
#include <FastLED.h>

FASTLED_USING_NAMESPACE

#include <stdint.h>

/*** DEFINES ***/
#define ENABLE_DEBUG
//#define ENABLE_PROFILING

#ifdef BOARD_ESP32
  #define DATA_PIN            4
#else
// BOARD_ARDUINO_NANO
// BOARD_ARDUINO_NANO_HW_CONTROLS
  #define DATA_PIN            3
#endif


//#define INCLUDE_PROGRAM_SOUND

#ifdef BOARD_ARDUINO_NANO_HW_CONTROLS
  #define HAS_ANALOG_INPUTS
  #define HAS_DIGITAL_INPUTS
#endif //BOARD_ARDUINO_NANO_HW_CONTROLS


/*** Analog Inputs ***/
#ifdef HAS_ANALOG_INPUTS
  #define ANALOG_IN_0 A1
  #define ANALOG_IN_1 A2

#endif //HAS_ANALOG_INPUTS

/*** Digital Inputs ***/
#ifdef HAS_DIGITAL_INPUTS
  #define DIGITAL_IN_0 4
  #define DIGITAL_IN_1 5
  #define LONG_PRESS_TIME_MS 1000
#endif //HAS_DIGITAL_INPUTS


/*** WIFI/MQTT Settings ***/
#ifdef WIFI_ENABLED  
  #define WIFI_DEBUG

  #include "Settings_Private.h"
  // WiFi Settings
  //#define WIFI_SSID         "wifi_ssid"
  //#define WIFI_PASS         "wifi_pass"
  
  // MQTT Settings
  //#define MQTT_SERVER       "mqtt_ip"
  //#define MQTT_USERNAME     "mqtt_user"
  //#define MQTT_PASSWORD     "mqtt_pass"
  //#define MQTT_PORT         1883
  
  #define MQTT_TOPIC_STATE                      DEVICETYPE "/" DEVICENAME
  #define MQTT_TOPIC_STATUS                     DEVICETYPE "/" DEVICENAME "/status"
  #define MQTT_TOPIC_SET                        DEVICETYPE "/" DEVICENAME "/set"  
  #define MQTT_TOPIC_CONFIG                     DEVICETYPE "/" DEVICENAME "/config"
  #define MQTT_TOPIC_GROUP                      DEVICETYPE "/" GROUPNAME
  #define MQTT_HOMEASSISTANT_DISCOVERY_PREFIX   "homeassistant"
  
  #define MQTT_PAYLOAD_ON                       "ON"
  #define MQTT_PAYLOAD_OFF                      "OFF"
  
  #define MQTT_STATUS_ONLINE                    "online"
  #define MQTT_STATUS_OFFLINE                   "offline"
  
  #define MQTT_MAX_PACKET_SIZE 512
  
  // OTA Settings
  #define OTA_DEVICENAME    DEVICENAME      //change this to whatever you want to call your device
  #define OTA_PASSWORD      "123"           //the password you will need to enter to upload remotely via the ArduinoIDE
  #define OTA_PORT          8266

#endif //WIFI_ENABLED


/*** General Settings ***/
#define USE_NONBLOCKING_DELAY

#define FRAMES_PER_SECOND         30 //120

#define MIN_SPEED                 0
#define MAX_SPEED                 255
#define DEFAULT_SPEED             FRAMES_PER_SECOND
#define MAX_CYCLE_TIME_MS         10000UL   // ms

#define DEFAULT_BRIGHTNESS        96  
#define DEFAULT_HUE               128   // Aqua
#define DEFAULT_SATURATION        255

#define AUTO_CYCLE_PROGRAMS       true
#define PROGRAM_CYCLE_TIME_SEC    60

#define AUTO_CYCLE_HUE            true
#define AUTO_HUE_CYCLE_TIME_MS    200

#define DEFAULT_MIRROR_MODE       false
#define DEFAULT_REVERSE_MODE      false
/*** TYPE DEFINITIONS ***/
typedef struct 
{
  bool Enabled;
  
  uint8_t Speed;
  uint8_t Brightness;
  uint8_t Hue;
  uint8_t Saturation;
  
  bool AutoCycleHue;
  uint16_t AutoCycleHueDelayMs;
  
  bool AutoCyclePrograms;
  uint16_t AutoProgramDelaySec;

  bool Mirror;
  bool Reverse;
} GlobalSettings;


extern CRGB g_LEDS[DEFAULT_NUM_LEDS];
extern GlobalSettings g_GlobalSettings;

extern uint16_t         g_NumLeds;
#define NUM_LEDS        g_NumLeds

#include "CProgram.h"
extern CLEDProgram *g_LEDPrograms[];

extern const uint8_t g_NumPrograms;
extern int8_t g_NextProgramIndex;


#endif //SETTINGS_H
