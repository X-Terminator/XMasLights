
/*** INCLUDES ***/
#include "Settings.h"

#ifdef WIFI_ENABLED


void WiFi_MQTT_Init(void);
void WiFi_MQTT_Tick(void);
void MQTT_SendState(void);

#endif // WIFI_ENABLED
