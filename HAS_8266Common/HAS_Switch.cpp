/*
 * HAS_Switch.cpp
 *
 * (C) 2019 James Dixon
 *
 *
 *
 */
#include "HAS_Switch.h"

void T_Switch::Setup(int _id, byte _p1,byte _p2,byte _t,const char *_top)
{
  id=_id;pin1=_p1;pin2=_p2;type=_t;topic=_top;

  if( pin1 != 255 ) {pinMode(pin1, INPUT_PULLUP);Serial.print("Pullup input pin: ");Serial.println(pin1);}
  if( pin2 != 255 ) {pinMode(pin2, INPUT_PULLUP);Serial.print("Pullup input pin: ");Serial.println(pin2);}
}

String T_Switch::CmndString()
{
  return String(options.MqttTopic()) + "/" + options.Prefix1() + "/switch" + id;
}

void T_Switch::Set( const char *_val, bool _out, bool _tog )
{
  // first publish switch command message:
  String t = CmndString();
  PSclient.publish(t.c_str(), _val, false);

  // and additional topic if set:
  if( *topic != 0 )
  {
    PSclient.publish(topic, _val, false);
  }
  Serial.print(t);
  Serial.print(": ");
  Serial.println(_val);

  // and now make the change to the hardware:
  SetOutput(id, _out, _tog);
}

void T_Switch::Toggle()
{
  Serial.println("Toggle switch");
  Set("TOGGLE", false, true);
}

void T_Switch::On()
{
  Serial.println("On switch");
  Set("ON", true, false);
}
void T_Switch::Off()
{
  Serial.println("Off switch");
  Set("OFF", false, false);
}

/*
 * Update a switch
 * work out which output it relates to (id)
 */
void T_Switch::Update( int _timeStep )
{
  if( pin1 == -1 ) return;
  if( type == SWTYPE_PRESS || type == SWTYPE_RELEASE || type == SWTYPE_PUSHBUTTON || type == SWTYPE_DELAY)
  { // pushbutton or on/off
    if( !digitalRead(pin1) )
    {
      if( !latch )
      {
        latch = true;
        if( type == SWTYPE_PRESS )
        {
          Toggle();
        }
        if( type == SWTYPE_PUSHBUTTON || type == SWTYPE_DELAY )
        {
          On();
          delayTimer = options.SwDelay(id);
        }
      }
    }
    else
    {
      if( latch )
      {
        latch = false;
        if( type == SWTYPE_RELEASE )
        {
          Toggle();
        }
        if( type == SWTYPE_PUSHBUTTON )
        {
          Off();
        }
      }
    }
  }
  else if(type == SWTYPE_DUAL)
  {
    if( !digitalRead(pin1) )
    {
      if( !latch )
      {
        latch = true;
        On();
      }
    }
    else
    {
      if( latch )
      {
        latch = false;
      }
    }
    if( !digitalRead(pin2) )
    {
      if( !latch2 )
      {
        latch2 = true;
        Off();
      }
    }
    else
    {
      if( latch2 )
      {
        latch2 = false;
      } 
    }
  }

  if( type == SWTYPE_DELAY )
  {
    if( delay > 0 )
      delay -= _timeStep;
      if( delay <= 0 )
      {
        delay = 0;
        // switch off again:
        Off();
      }
  }
}
