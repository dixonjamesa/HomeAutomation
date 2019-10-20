/*
 * HAS_Switch.cpp
 *
 * (C) 2019 James Dixon
 *
 * Class for a switch
 *
 */

#ifndef _SWITCH_H_
#define _SWITCH_H_

// switch needs:
//   - pin
//   - type - toggle etc.
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
    byte type;    // See config.h for SWTYPE_XXX
    const char *topic; // send this topic as well

    bool latch;   // pin1 latch
    bool latch2;  // pin2 latch
    int delay; // delay for auto off mode
};

#endif //_SWITCH_H_
