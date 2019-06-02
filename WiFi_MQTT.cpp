/*** INCLUDES ***/
#include "WiFi_MQTT.h"

#ifdef WIFI_ENABLED

#include <ArduinoJson.h>
#ifdef BOARD_ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif
#include <PubSubClient.h> // Note: MQTT_MAX_PACKET_SIZE was changed to 512 in this file
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

/*** DEFINES ***/
#define WIFI_DEBUG

#ifdef WIFI_DEBUG
    #define MSG_DBG(m)     Serial.print(m)
    #define MSG_DBG_LN(m)  Serial.println(m)
#else
    #define MSG_DBG(m)     
    #define MSG_DBG_LN(m)  
#endif

#define MS_TIMER_START(tim)               tim = millis();
#define MS_TIMER_ELAPSED(tim, delay)      ((millis() - tim) >= delay)

// JSON Settings
const int JSON_BUFFER_SIZE = JSON_OBJECT_SIZE(30);


/*** TYPE DEFINITIONS ***/
typedef enum {
    STATE_WIFI_DISCONNECTED,
    STATE_WIFI_CONNECTING,
    STATE_MQTT_CONNECTING,
    STATE_WIFI_MQTT_CONNECTED    
} WiFi_MQTT_State;


/*** FORWARD DECLARATIONS ***/
static void OTA_Setup(void);
static void MQTT_Callback(char* inTopic, byte* inPayload, unsigned int inLlength);
static bool MQTT_ParseJSON(char* inMessage);
static void MQTT_Reconnect(void);
static void MQTT_SetOnline(bool inOnline);
static void MQTT_Discovery(void);
static void MQTT_SendConfig(void);

/*** PRIVATE VARIABLES ***/
static WiFi_MQTT_State  s_State;

static WiFiClient       s_WiFiClient;
static PubSubClient     s_MQTTClient(s_WiFiClient);

static unsigned long s_Timer;
static char s_DiscoveryTopic[128];

/*** PUBLIC FUNCTIONS ***/
void WiFi_MQTT_Init()
{
    // Register MQTT Server and callback function
    s_MQTTClient.setServer(MQTT_SERVER, MQTT_PORT);
    s_MQTTClient.setCallback(MQTT_Callback);      
    s_State = STATE_WIFI_DISCONNECTED;
}

bool WiFi_MQTT_IsConnected()
{
  return (bool)(s_State == STATE_WIFI_MQTT_CONNECTED);
}

void WiFi_MQTT_Tick()
{
    if ((s_State != STATE_WIFI_DISCONNECTED) && (s_State != STATE_WIFI_CONNECTING) && (WiFi.status() != WL_CONNECTED))
    {
        MSG_DBG_LN("WIFI Disconnected! Attempting reconnection.");
        s_State = STATE_WIFI_DISCONNECTED;
    }
    
    switch(s_State)
    {
        case STATE_WIFI_DISCONNECTED:
            // We start by connecting to a WiFi network
            MSG_DBG("Connecting to ");
            MSG_DBG_LN(WIFI_SSID);

#ifdef BOARD_ESP32
            WiFi.mode(WIFI_STA);
#endif //BOARD_ESP32            
            WiFi.begin(WIFI_SSID, WIFI_PASS);
            s_State = STATE_WIFI_CONNECTING;
            break;
        case STATE_WIFI_CONNECTING:
            if (WiFi.status() == WL_CONNECTED)
            {
                MSG_DBG("WiFi connected. IP address: ");
                MSG_DBG_LN(WiFi.localIP());
                OTA_Setup();
                s_Timer = 0;    // no delay before initial connect attempt
                s_State = STATE_MQTT_CONNECTING;
            }
            break;
        case STATE_MQTT_CONNECTING:
            if ((s_Timer == 0) || MS_TIMER_ELAPSED(s_Timer, 5000))
            {
                // Attempt to connect
                MSG_DBG("Attempting MQTT connection...");
                if (s_MQTTClient.connect(OTA_DEVICENAME, MQTT_USERNAME, MQTT_PASSWORD, MQTT_TOPIC_STATUS, 0, true, MQTT_STATUS_OFFLINE)) 
                {
                  MSG_DBG_LN("connected!");

                  MSG_DBG_LN("Subscribe to topics:");
                  MSG_DBG_LN(MQTT_TOPIC_SET);
                  MSG_DBG_LN(MQTT_TOPIC_GROUP);

                  // Subscribe to topics
                  s_MQTTClient.subscribe(MQTT_TOPIC_SET);
                  s_MQTTClient.subscribe(MQTT_TOPIC_GROUP);
                  MQTT_SetOnline(true);
                  delay(10);
                  MQTT_Discovery();
                  delay(10);
                  MQTT_SendConfig();
                  delay(10);
                  MQTT_SendState();
                  s_State = STATE_WIFI_MQTT_CONNECTED;
                }
                else 
                {
                  MSG_DBG("failed, rc=");
                  MSG_DBG(s_MQTTClient.state());
                  MSG_DBG_LN(" try again in 5 seconds");
                  // Wait 5 seconds before retrying
                  MS_TIMER_START(s_Timer);
                }
            }
            break;
        case STATE_WIFI_MQTT_CONNECTED:
            if (!s_MQTTClient.connected())
            {
                MSG_DBG_LN("MQTT Connection Lost!");
                s_Timer = 0;    // no delay before reconnect attempt
                s_State = STATE_MQTT_CONNECTING;
            }
            else
            {
                // Service MQTT messages
                s_MQTTClient.loop();
            }
            break;
    }
    
    if (WiFi.status() == WL_CONNECTED)
    {
        // Handle Over-The-Air (OTA) update requests
        ArduinoOTA.handle();
    }
}
/*
    Example state JSON:
  {"state":"ON","color":{"r":0,"g":0,"b":255},"brightness":15,"effect":"juggle","transition":150}
*/

void MQTT_SendState() 
{
  if (s_MQTTClient.connected())
  {
    StaticJsonBuffer<JSON_BUFFER_SIZE> lvJSONBuffer;
  
    JsonObject& lvRoot = lvJSONBuffer.createObject();
  
  //lvRoot["Enabled"]             = g_GlobalSettings.Enabled;
    lvRoot["state"]               = (g_GlobalSettings.Enabled ? "ON" : "OFF");
    lvRoot["Speed"]               = g_GlobalSettings.Speed;
  //lvRoot["Brightness"]          = g_GlobalSettings.Brightness;
    lvRoot["brightness"]          = g_GlobalSettings.Brightness;
  //lvRoot["Hue"]                 = g_GlobalSettings.Hue;
    
    JsonObject& lvColor = lvRoot.createNestedObject("color");
    lvColor["h"]                  = map(g_GlobalSettings.Hue, 0, 255, 0, 360);
    lvColor["s"]                  = map(g_GlobalSettings.Saturation, 0, 255, 0, 100);
    
    lvRoot["AutoCycleHue"]        = g_GlobalSettings.AutoCycleHue;
    lvRoot["AutoCycleHueDelayMs"] = g_GlobalSettings.AutoCycleHueDelayMs;
    lvRoot["AutoCyclePrograms"]   = g_GlobalSettings.AutoCyclePrograms;
    lvRoot["AutoProgramDelaySec"] = g_GlobalSettings.AutoProgramDelaySec;
    lvRoot["Mirror"]              = g_GlobalSettings.Mirror;
    lvRoot["Reverse"]             = g_GlobalSettings.Reverse;
  
    if (g_NextProgramIndex >= 0)
    {
      lvRoot["effect"]            = g_LEDPrograms[g_NextProgramIndex]->Name;
    }
  
    char lvBuffer[lvRoot.measureLength() + 1];
    lvRoot.printTo(lvBuffer, sizeof(lvBuffer));
  
    MSG_DBG_LN("JSON Status:");
    MSG_DBG_LN(lvBuffer);
  
    MSG_DBG_LN("Publish to topic:");
    MSG_DBG_LN(MQTT_TOPIC_STATE);
  
    s_MQTTClient.publish(MQTT_TOPIC_STATE, lvBuffer, true);
  }
}

/*** PRIVATE FUNCTIONS ***/
static void OTA_Setup(void)
{
   //OTA SETUP
  ArduinoOTA.setPort(OTA_PORT);
  
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(OTA_DEVICENAME);

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)OTApassword);

  ArduinoOTA.onStart([]() 
  {
    Serial.println("OTA Starting");
  });
  ArduinoOTA.onEnd([]() 
  {
    Serial.println("\nOTA End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) 
  {
    Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) 
  {
    Serial.printf("OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

static void MQTT_Callback(char* inTopic, byte* inPayload, unsigned int inLength)
{
  MSG_DBG("Message arrived [");
  MSG_DBG(inTopic);
  MSG_DBG("] ");

  // copy message in order to append '\0' terminator
  char lvMessage[inLength + 1];
  for (int i = 0; i < inLength; i++) 
  {
    lvMessage[i] = (char)inPayload[i];
  }
  lvMessage[inLength] = '\0';
  MSG_DBG_LN(lvMessage);

  if (!MQTT_ParseJSON(lvMessage)) 
  {
    return;
  }

#if 0
  if (stateOn) {

    realRed = map(red, 0, 255, 0, brightness);
    realGreen = map(green, 0, 255, 0, brightness);
    realBlue = map(blue, 0, 255, 0, brightness);
  }
  else {

    realRed = 0;
    realGreen = 0;
    realBlue = 0;
  }

  MSG_DBG_LN(effect);

  startFade = true;
  inFade = false; // Kill the current fade
#endif

  MQTT_SendState();  
}

static bool MQTT_ParseJSON(char* inMessage) 
{
  StaticJsonBuffer<JSON_BUFFER_SIZE> lvJSONBuffer;

  JsonObject& lvRoot = lvJSONBuffer.parseObject(inMessage);

  if (!lvRoot.success()) 
  {
    MSG_DBG_LN("MQTT_ParseJSON: parseObject() failed");
    return false;
  }

  if (lvRoot.containsKey("Enabled") && lvRoot.is<bool>("Enabled"))
  {
    g_GlobalSettings.Enabled = lvRoot.get<bool>("Enabled");
    MSG_DBG("Enabled: ");
    MSG_DBG_LN(g_GlobalSettings.Enabled);
  }
  else if (lvRoot.containsKey("state"))
  {
    if (strcmp(lvRoot["state"], "ON") == 0) 
    {
      g_GlobalSettings.Enabled = true;
    }
    else if (strcmp(lvRoot["state"], "OFF") == 0) 
    {
      g_GlobalSettings.Enabled = false;
    }    
    MSG_DBG("Enabled: ");
    MSG_DBG_LN(g_GlobalSettings.Enabled);
  }
  
  if (lvRoot.containsKey("Speed") && lvRoot.is<unsigned char>("Speed"))
  {
    g_GlobalSettings.Speed = lvRoot.get<unsigned char>("Speed");
    MSG_DBG("Speed: ");
    MSG_DBG_LN(g_GlobalSettings.Speed);
  }
  
  if (lvRoot.containsKey("Brightness") && lvRoot.is<unsigned char>("Brightness"))
  {
    g_GlobalSettings.Brightness = lvRoot.get<unsigned char>("Brightness");
    MSG_DBG("Brightness: ");
    MSG_DBG_LN(g_GlobalSettings.Brightness);
  }
  if (lvRoot.containsKey("brightness") && lvRoot.is<unsigned char>("brightness"))
  {
    g_GlobalSettings.Brightness = lvRoot.get<unsigned char>("brightness");
    MSG_DBG("Brightness: ");
    MSG_DBG_LN(g_GlobalSettings.Brightness);
  }

  if (lvRoot.containsKey("Hue") && lvRoot.is<unsigned char>("Hue"))
  {
    g_GlobalSettings.Hue = lvRoot.get<unsigned char>("Hue");
    MSG_DBG("Hue: ");
    MSG_DBG_LN(g_GlobalSettings.Hue);
  }
  else if (lvRoot.containsKey("color")) 
  {
    JsonVariant lvTemp;
    lvTemp = lvRoot["color"]["h"];
    if (lvTemp.is<float>())
    {
      // Hue
      g_GlobalSettings.Hue = (uint8_t)map(lvTemp.as<float>(), 0.0f, 360.0f, 0, 255);
      g_GlobalSettings.AutoCycleHue = false;
      MSG_DBG("Hue: ");
      MSG_DBG_LN(g_GlobalSettings.Hue);
    }
    lvTemp = lvRoot["color"]["s"];
    if (lvTemp.is<float>())
    {
      // Saturation
      g_GlobalSettings.Saturation = (uint8_t)map(lvTemp.as<float>(), 0.0f, 100.0f, 0, 255);
      MSG_DBG("Saturation: ");
      MSG_DBG_LN(g_GlobalSettings.Saturation);
    }
  }
  if (lvRoot.containsKey("AutoCycleHue") && lvRoot.is<bool>("AutoCycleHue"))
  {
    g_GlobalSettings.AutoCycleHue = lvRoot.get<bool>("AutoCycleHue");
    MSG_DBG("AutoCycleHue: ");
    MSG_DBG_LN(g_GlobalSettings.AutoCycleHue);
  }
  if (lvRoot.containsKey("AutoCycleHueDelayMs") && lvRoot.is<unsigned short>("AutoCycleHueDelayMs"))
  {
    g_GlobalSettings.AutoCycleHueDelayMs = lvRoot.get<unsigned short>("AutoCycleHueDelayMs");
    MSG_DBG("AutoCycleHueDelayMs: ");
    MSG_DBG_LN(g_GlobalSettings.AutoCycleHueDelayMs);
  }
  if (lvRoot.containsKey("AutoCyclePrograms") && lvRoot.is<bool>("AutoCyclePrograms"))
  {
    g_GlobalSettings.AutoCyclePrograms = lvRoot.get<bool>("AutoCyclePrograms");
    MSG_DBG("AutoCyclePrograms: ");
    MSG_DBG_LN(g_GlobalSettings.AutoCyclePrograms);
  }
  if (lvRoot.containsKey("AutoProgramDelaySec") && lvRoot.is<unsigned short>("AutoProgramDelaySec"))
  {
    g_GlobalSettings.AutoProgramDelaySec = lvRoot.get<unsigned short>("AutoProgramDelaySec");
    MSG_DBG("AutoProgramDelaySec: ");
    MSG_DBG_LN(g_GlobalSettings.AutoProgramDelaySec);
  }
  if (lvRoot.containsKey("Mirror") && lvRoot.is<bool>("Mirror"))
  {
    g_GlobalSettings.Mirror = lvRoot.get<bool>("Mirror");
    MSG_DBG("Mirror: ");
    MSG_DBG_LN(g_GlobalSettings.Mirror);
  }
  if (lvRoot.containsKey("Reverse") && lvRoot.is<bool>("Reverse"))
  {
    g_GlobalSettings.Reverse = lvRoot.get<bool>("Reverse");
    MSG_DBG("Reverse: ");
    MSG_DBG_LN(g_GlobalSettings.Reverse);
  }
  if (lvRoot.containsKey("effect") && lvRoot.is<char *>("effect"))
  {
    const char *lvEffect = lvRoot.get<char *>("effect");
    int i;
    for (i = 0; i < g_NumPrograms; i++)
    {
      if (strcmp(g_LEDPrograms[i]->Name, lvEffect) == 0)
      {
        g_GlobalSettings.AutoCyclePrograms = false;
        g_NextProgramIndex = i;
        MSG_DBG("Next Effect: ");
        MSG_DBG(g_NextProgramIndex);
        MSG_DBG(" -> ");
        MSG_DBG_LN(lvEffect);
        break;
      }
    }
    if (i >= g_NumPrograms)
    {
      MSG_DBG("Unknown effect: ");
      MSG_DBG_LN(lvEffect);
    }
  }
  return true;
}

static void MQTT_SetOnline(bool inOnline)
{
  if (inOnline)
  {
     s_MQTTClient.publish(MQTT_TOPIC_STATUS, MQTT_STATUS_ONLINE, true);
  }
  else
  {
    s_MQTTClient.publish(MQTT_TOPIC_STATUS, MQTT_STATUS_OFFLINE, true);
  }
}

static void MQTT_Discovery()
{
  StaticJsonBuffer<JSON_BUFFER_SIZE> lvJSONBuffer;
  JsonObject& lvRoot = lvJSONBuffer.createObject();
  
  lvRoot["name"] = DEVICENAME;
  //lvRoot["platform"] = "mqtt";
  lvRoot["schema"] = "json";
  //lvRoot["rgb"] = true;
  lvRoot["hs"] = true;
  //lvRoot["color_temp"] = true;
  lvRoot["brightness"] = true;
  //lvRoot["white_value"] = true;
  lvRoot["state_topic"] = MQTT_TOPIC_STATE;
  lvRoot["command_topic"] = MQTT_TOPIC_SET;
  lvRoot["availability_topic"] = MQTT_TOPIC_STATUS;
  lvRoot["effect"] = true;

  JsonArray& lvEffects = lvRoot.createNestedArray("effect_list");
  for (int i = 0; i < g_NumPrograms; i++)
  {
    lvEffects.add(g_LEDPrograms[i]->Name);
  }

  char lvBuffer[lvRoot.measureLength() + 1];
  lvRoot.printTo(lvBuffer, sizeof(lvBuffer));
  
  snprintf(s_DiscoveryTopic, sizeof(s_DiscoveryTopic), "%s/light/%s/config", MQTT_HOMEASSISTANT_DISCOVERY_PREFIX, DEVICENAME);

  s_MQTTClient.publish(s_DiscoveryTopic, lvBuffer, true);
}


static void MQTT_SendConfig() 
{
  StaticJsonBuffer<JSON_BUFFER_SIZE> lvJSONBuffer;

  JsonObject& lvRoot = lvJSONBuffer.createObject();
  lvRoot["Name"]                = OTA_DEVICENAME;
  lvRoot["IP"]                  = WiFi.localIP().toString();
  lvRoot["NumLeds"]             = DEFAULT_NUM_LEDS;

  char lvBuffer[lvRoot.measureLength() + 1];
  lvRoot.printTo(lvBuffer, sizeof(lvBuffer));

  MSG_DBG_LN("JSON Config:");
  MSG_DBG_LN(lvBuffer);

  MSG_DBG_LN("Publish to topic:");
  MSG_DBG_LN(MQTT_TOPIC_CONFIG);

  s_MQTTClient.publish(MQTT_TOPIC_CONFIG, lvBuffer, true);
}
static void MQTT_Reconnect() 
{
  // Loop until we're reconnected
  while (!s_MQTTClient.connected()) 
  {
    MSG_DBG("Attempting MQTT connection...");
    // Attempt to connect
    if (s_MQTTClient.connect(OTA_DEVICENAME, MQTT_USERNAME, MQTT_PASSWORD)) 
    {
      MSG_DBG_LN("connected!");

      // Subscribe to topics
      s_MQTTClient.subscribe(MQTT_TOPIC_SET);
      s_MQTTClient.subscribe(MQTT_TOPIC_GROUP);
      MQTT_SendState();
    } 
    else 
    {
      MSG_DBG("failed, rc=");
      MSG_DBG(s_MQTTClient.state());
      MSG_DBG_LN(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


#endif // WIFI_ENABLED
