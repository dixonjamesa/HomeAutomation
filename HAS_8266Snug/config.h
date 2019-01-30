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

#define FRIENDLY1 "%s 1"          // [FriendlyName1] name used by, e.g. HomeAssistant
#define FRIENDLY2 "%s 2"          // [FriendlyName2] name used by, e.g. HomeAssistant
#define FRIENDLY3 "%s 3"          // [FriendlyName3] name used by, e.g. HomeAssistant
#define FRIENDLY4 "%s 4"          // [FriendlyName4] name used by, e.g. HomeAssistant
#define FRIENDLY5 "%s 5"          // [FriendlyName5] name used by, e.g. HomeAssistant
#define FRIENDLY6 "%s 6"          // [FriendlyName6] name used by, e.g. HomeAssistant
#define FRIENDLY7 "%s 7"          // [FriendlyName7] name used by, e.g. HomeAssistant
#define FRIENDLY8 "%s 8"          // [FriendlyName8] name used by, e.g. HomeAssistant

#define STA_SSID "PrettyFly24"    // [Ssid] Default wifi SSID
#define STA_PASS "Dixonia1"       // [Password] Default wifi password

#define WEBNAME "%s"              // [WebName] Hostname, Web server SoftAP name and the OTA name used

#define AP_SERVER 1               // [WebServer] 0 = Off, 1 = On. CURRENTLY NOT USED. Automatically starts if cannot onnect to WiFi, and stops otherwise.
#define AP_PASS "password123"     // [WebPassword] SoftAP password

//#define MQTT_HOST "pibrain.local"  // [MqttHost]
#define MQTT_HOST "192.168.0.99"  // [MqttHost]
//#define MQTT_FINGERPRINT1      "A5 02 FF 13 99 9F 8B 39 8E F1 83 4F 11 23 65 0B 32 36 FC 07"  // [MqttFingerprint1]
//#define MQTT_FINGERPRINT2      "A5 02 FF 13 99 9F 8B 39 8E F1 83 4F 11 23 65 0B 32 36 FC 07"  // [MqttFingerprint2]
#define MQTT_PORT     1883        // [MqttPort] MQTT port
#define MQTT_USER     ""          // [MqttUser] MQTT user
#define MQTT_PASS     ""          // [MqttPass] MQTT password
#define MQTT_CLIENT "%sClient"    // [MqttClient] MQTT client name

#define MQTT_TOPIC "thing"        // [Topic] (unique) MQTT device topic
#define MQTT_GRPTOPIC "allthings" // [GroupTopic] MQTT group topic

// similar mqtt system to the tasmota sonoff system, to keep things consistent:
#define SUB_PREFIX   "cmnd"       // [Prefix1] Subscribe to
#define PUB_PREFIX   "stat"       // [Prefix2] Publish to
#define INFO_PREFIX  "tele"       // [Prefix3] Publish telemetry data to

// switch behaviour
// types are: 0 disabled, 1 toggle on press; 2 toggle on release; 3 pushbutton (push on/release off); 4 pin1 on, pin2 off
#define SW1_TYPE  1               // [Switch1Type] 
#define SW1_TOPIC "alttest/cmnd/switch" // [Switch1Topic] additional topic to send messages on
#define SW2_TYPE  0               // [Switch2Type] 
#define SW2_TOPIC   ""            // [Switch2Topic] additional topic to send messages on
#define SW3_TYPE  1               // [Switch3Type] 
#define SW3_TOPIC   ""            // [Switch3Topic] additional topic to send messages on
#define SW4_TYPE  3               // [Switch4Type] 
#define SW4_TOPIC   ""            // [Switch4Topic] additional topic to send messages on
#define SW5_TYPE  0               // [Switch5Type]
#define SW5_TOPIC   ""            // [Switch5Topic] additional topic to send messages on
#define SW6_TYPE  0               // [Switch6Type]
#define SW6_TOPIC   ""            // [Switch6Topic] additional topic to send messages on
#define SW7_TYPE  0               // [Switch7Type]
#define SW7_TOPIC   ""            // [Switch7Topic] additional topic to send messages on
#define SW8_TYPE  0               // [Switch8Type]
#define SW8_TOPIC   ""            // [Switch8Topic] additional topic to send messages on



//////////////////////////////////////////////////////////
// HARDWARE CONFIG

// switches
#define NUM_SWITCHES 8

#define SW1_PIN1  D5              // [Sw1Pin1] GPIO pin (pushbutton and on), -1 disabled
#define SW1_PIN2  -1              // [Sw1Pin2] GPIO pin (off), -1 disabled
#define SW2_PIN1  -1              // [Sw2Pin1] GPIO pin (pushbutton and on), -1 disabled
#define SW2_PIN2  -1              // [Sw2Pin2] GPIO pin (off), -1 disabled
#define SW3_PIN1  D4              // [Sw3Pin1] GPIO pin (pushbutton and on), -1 disabled
#define SW3_PIN2  -1              // [Sw3Pin2] GPIO pin (off), -1 disabled
#define SW4_PIN1  D3              // [Sw4Pin1] GPIO pin (pushbutton and on), -1 disabled
#define SW4_PIN2  -1              // [Sw4Pin2] GPIO pin (off), -1 disabled
#define SW5_PIN1  -1              // [Sw5Pin1] GPIO pin (pushbutton and on), -1 disabled
#define SW5_PIN2  -1              // [Sw5Pin2] GPIO pin (off), -1 disabled
#define SW6_PIN1  -1              // [Sw6Pin1] GPIO pin (pushbutton and on), -1 disabled
#define SW6_PIN2  -1              // [Sw6Pin2] GPIO pin (off), -1 disabled
#define SW7_PIN1  -1              // [Sw7Pin1] GPIO pin (pushbutton and on), -1 disabled
#define SW7_PIN2  -1              // [Sw7Pin2] GPIO pin (off), -1 disabled
#define SW8_PIN1  -1              // [Sw8Pin1] GPIO pin (pushbutton and on), -1 disabled
#define SW8_PIN2  -1              // [Sw8Pin2] GPIO pin (off), -1 disabled

// outputs
#define NUM_OUTS 8

#define OUT1_PIN    D7            // [Out1Pin]
#define OUT2_PIN    -1            // [Out2Pin]
#define OUT3_PIN    -1            // [Out3Pin]
#define OUT4_PIN    -1            // [Out4Pin]
#define OUT5_PIN    -1            // [Out5Pin]
#define OUT6_PIN    -1            // [Out6Pin]
#define OUT7_PIN    -1            // [Out7Pin]
#define OUT8_PIN    -1            // [Out8Pin]
#define STATUS_LED  D6            // [StatusLED]
#define OUT1_LED    D6            // [Out1LED]
#define OUT2_LED    -1            // [Out2LED]
#define OUT3_LED    -1            // [Out3LED]
#define OUT4_LED    -1            // [Out4LED]
#define OUT5_LED    -1            // [Out5LED]
#define OUT6_LED    -1            // [Out6LED]
#define OUT7_LED    -1            // [Out7LED]
#define OUT8_LED    -1            // [Out8LED]

#define INVERT_LED  0             // [InvertLED]

// rotaries
#define NUM_ROTS 4

#define ROT1_PINU   D1            // [Rot1PinU]
#define ROT1_PIND   D2            // [Rot1PinD]
#define ROT2_PINU   -1            // [Rot2PinU]
#define ROT2_PIND   -1            // [Rot2PinD]
#define ROT3_PINU   -1            // [Rot3PinU]
#define ROT3_PIND   -1            // [Rot3PinD]
#define ROT4_PINU   -1            // [Rot4PinU]
#define ROT4_PIND   -1            // [Rot4PinD]

//////////////////////////////////////////////////////////////
////  COMPILE-TIME

#define HOMEASSISTANT_DISCOVERY 1 // Home Assistant MQTT discovery messages

#endif  // _CONFIG_H_
