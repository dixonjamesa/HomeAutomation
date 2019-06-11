/*
 * HAS_Main.h
 * 
 * (C) 2019 James Dixon
 * 
 * Main setup functionality
 * 
 */
#ifndef _MAIN_H_
#define _MAIN_H_

#include "config.h"
// WebServer responses
#include <Arduino.h>
#include <PubSubClient.h>

void tryConnectWIFI(); 
void disconnectWIFI();
extern String WIFI_status;
extern PubSubClient PSclient;

// switch needs:
//   - pin
//   - type - toggle or on/off
//   - id => <unit>/<cmnd>/switch<id> ON/OFF/TOGGLE
//   -  topic 
class T_Switch
{
  public:
    T_Switch(int _id=0) { id = _id;}
    void Setup(int _id, byte _p1,byte _p2,byte _t,const char *_top);
    // call regularly:
    void Update( );

    void Set( const char *_val, bool _out, bool _tog );
    void Toggle();
    void On();
    void Off();
    String CmndString();
    
    byte id;
    // note doesn't support a single pin being on or off, as that puts state into the switch
    // pins assumed to be pullup, so press to pull low
    byte pin1;    // board pin toggle or on
    byte pin2;    // board pin off
    byte type;    // 0 toggle press; 1 toggle release; 2 pushbutton (push on/release off); 3 pin1 on, pin2 off
    const char *topic; // send this topic as well

    bool latch;   // pin1 latch
    bool latch2;  // pin2 latch
};

void PublishStatus();

/* 
 * Set output state 
 */ 
void SetOutput( int _id /* 1-n */, bool _state, bool _toggle=false );
void SetRGB(int r, int g, int b );
void GetRGB(int &r, int &g, int &b);
void SetBrightness(int bri );
int GetBrightness();
void SetHue(int hue); // 0 to 360
int GetHue();
void SetSaturation(int sat);
int GetSaturation();
void SetStripLength(int _c); // set number of RGB pixels in the neopixel string
bool GetOutput(int _id /* 1-n */);
int GetStripLength(); // get number of RGB pixels
extern T_Switch switches[NUM_SWITCHES];

// Used for controlling RGB strips:
#define MODE_BRIGHTNESS 0
#define MODE_HUE 1
#define MODE_SATURATION 2
#define MODE_LENGTH 3
#define MODE_MAX 4

int GetMode();
int ChangeMode(int _m = -1);
void ChangeValue(int _amount);


#endif // _MAIN_H_
