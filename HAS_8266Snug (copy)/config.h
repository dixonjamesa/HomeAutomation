/*
 * config.h
 * 
 * (C) 2019 James Dixon
 * 
 * General config settings for ESP8266 HA endpoint unit
 * each PARAMETER default values defined here.
 * Can be changed with MQTT commands, as shown in []
 * 
 */
#ifndef _CONFIG_H_
#define _CONFIG_H_

#define UNIT "Thing"              // [Unit] Name of the unit

#define WIFI_AUTOCONNECT 1        // Auto-connect the wifi, using details below
#define STA_SSID "PrettyFly24"    // [Ssid] Default wifi SSID
#define STA_PASS "Dixonia1"       // [Password] Default wifi password

#define AP_SERVER 1               // [WebServer] 0 = Off, 1 = On
#define AP_SSID "Thing101"        // [FriendlyName] Web server SoftAP name - how to connect to this unit
#define AP_PASS "password123"     // [WebPassword] SoftAP password

#define MQTT_HOST "192.168.0.99"  // [MqttHost]
//#define MQTT_FINGERPRINT1      "A5 02 FF 13 99 9F 8B 39 8E F1 83 4F 11 23 65 0B 32 36 FC 07"  // [MqttFingerprint1]
//#define MQTT_FINGERPRINT2      "A5 02 FF 13 99 9F 8B 39 8E F1 83 4F 11 23 65 0B 32 36 FC 07"  // [MqttFingerprint2]
#define MQTT_PORT     1883        // [MqttPort] MQTT port
#define MQTT_USER     ""          // [MqttUser] MQTT user
#define MQTT_PASS     ""          // [MqttPassword] MQTT password

#define MQTT_TOPIC "thing"        // [Topic] (unique) MQTT device topic
#define MQTT_GRPTOPIC "allthings" // [GroupTopic] MQTT group topic

// similar mqtt system to the tasmota sonoff system, to eep things consistent:
#define SUB_PREFIX   "cmnd"       // [Prefix1] Subscribe to
#define PUB_PREFIX   "stat"       // [Prefix2] Publish to
#define PUB_PREFIX2  "tele"       // [Prefix3] Publish telemetry data to

#define HOMEASSISTANT_DISCOVERY 1 // [HADisco] Home Assistant MQTT discovery messages

#endif  // _CONFIG_H_
