// Copy to secrets.h on the build machine. secrets.h is gitignored.
// Never put production keys, Wi-Fi passwords, or MQTT credentials in git.

#pragma once

#ifndef AMPED_WIFI_SSID
#define AMPED_WIFI_SSID ""
#endif

#ifndef AMPED_WIFI_PASS
#define AMPED_WIFI_PASS ""
#endif

#ifndef AMPED_API_KEY
#define AMPED_API_KEY ""
#endif

#ifndef AMPED_MQTT_HOST
#define AMPED_MQTT_HOST ""
#endif

#ifndef AMPED_MQTT_USER
#define AMPED_MQTT_USER ""
#endif

#ifndef AMPED_MQTT_PASS
#define AMPED_MQTT_PASS ""
#endif
