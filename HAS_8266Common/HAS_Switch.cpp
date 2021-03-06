/*
 * HAS_Switch.cpp
 *
 * (C) 2019 James Dixon
 *
 */
#include "HAS_Switch.h"
#include "HAS_option.h"


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
  // first publish switch command message.
  // this is the mqtt message for when the switch is pressed, so other mqtt listeners might use it as a trigger.
  // it is ignored on our own incoming mqtt listener, as it makes no sense (and would cause the switch to trigger twice)
  // incoming message responded to is cmnd/POWER if another device is to actually change state of this module
  String t = CmndString();
  PSclient.publish(t.c_str(), _val, false);

  // and additional topic if set:
  if( *topic != 0 )
  {
    // topic may be multiple, separated by ;'s
    char top[256];
    strcpy(top, topic);
    char *pch = strtok(top, ";");
    while(pch != NULL)
    {
      PSclient.publish(pch, _val, false);
      pch = strtok(NULL, ";");
    }
  }
  //Serial.print(t);
  //Serial.print(": ");
  //Serial.println(_val);

  // and now make the change to the hardware:
  SetOutput(id, _out, _tog);
}

/*
 * Handle switch when a single "hit" occurs
 * such as via web interface or RF433
 */
void T_Switch::Hit()
{
  switch(type)
  {
    case SWTYPE_NONE:
    case SWTYPE_PRESS:
    case SWTYPE_RELEASE:
    case SWTYPE_REMOTE:
      Toggle();
      break;
    case SWTYPE_PUSHBUTTON:
      On();
      Off();
      break;
    case SWTYPE_DUAL:
    case SWTYPE_DELAY:
      On();
      break;
  }
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
  if( type == SWTYPE_DELAY )
  {
    delayTimer = options.SwDelay(id);
    Serial.print("Delay is ");
    Serial.println(delayTimer);
  }
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
  if( type == SWTYPE_DELAY )
  {
    if( delayTimer > 0 )
    {
      delayTimer -= _timeStep;
      if( delayTimer <= 0 )
      {
        delayTimer = 0;
        // switch off again:
        Serial.println("Delay switch timeout - OFF now");
        Off();
      }
    }
  }

  if( pin1 == 255 ) return;

  // DUAL switch needs to consider both pins...
  if(type == SWTYPE_DUAL)
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
  else // all types exxcept DUAL
  {
    if( !digitalRead(pin1) )
    {
      if( !latch )
      {
        // set latch to make sure this only happens once:
        latch = true;
        if( type == SWTYPE_PRESS || type == SWTYPE_REMOTE )
        {
          Toggle();
        }
        if( type == SWTYPE_PUSHBUTTON || type == SWTYPE_DELAY )
        {
          On();
        }
      }
    }
    else
    {
      if( latch )
      {
        // unlatch once button is released
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
}
