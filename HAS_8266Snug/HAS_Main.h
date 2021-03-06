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
    void Setup(int _id, byte _p1,byte _p2,byte _t,String _top) {id=_id;pin1=_p1;pin2=_p2;type=_t;topic=_top;}
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
    byte type;    // see switch types defined in config.h
    String topic; // send this topic as well

    bool latch;   // pin1 latch
    bool latch2;  // pin2 latch
};

/*
 * Set output state
 */
void SetOutput( int _id, bool _state, bool _toggle=false );
bool GetOutput(int _id);
extern T_Switch switches[4];

#endif // _MAIN_H_
