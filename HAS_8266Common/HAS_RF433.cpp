/*
 * HAS_RF433.cpp
 *
 * listen for and assing 433MHz signals to actions
 * (C) 2019 James Dixon
 *
 */
#include "HAS_RF433.h"
#include "HAS_option.h"
#include <RCSwitch.h>

RCSwitch my433Switch = RCSwitch();
int AssignNext = -1;
int waitTimer = 0;
const int waitDelay = 1000;

void RF433Initialise(int _pin)
{
  Serial.println("RF433 setup:");
  if( _pin != -1 )
  {
    my433Switch.enableReceive(_pin);  // Receiver on RF433Pin
    Serial.print("RF433 listening on pin ");
    Serial.println(_pin);
  }
  else
  {
    Serial.println("No hardware pin");
  }
}

 // assign next received signal to code _id
void RF433AssignNextReceived(int _id)
{
  AssignNext = _id;
  Serial.print("Waiting to assign RF433 code to switch ");
  Serial.println(AssignNext);
  options.RFCode(_id, -2);
}

void RF433Update( int _dt )
{
  waitTimer = max(0, waitTimer - _dt);

  if( waitTimer <= 0 )
  {
    if (my433Switch.available())
    {
      int code, bl;
      waitTimer = waitDelay; // use this to stop multiple hits
      code = my433Switch.getReceivedValue();
      bl = my433Switch.getReceivedBitlength();
  #if 0
      Serial.print("Received ");
      Serial.print( code );
      Serial.print(" / ");
      Serial.print( bl );
      Serial.print("bit ");
      Serial.print("Protocol: ");
      Serial.println( my433Switch.getReceivedProtocol() );
  #endif
      if( options.RF433Pin()!= -1 )
      {
        if( AssignNext != -1 )
        {
          options.RFCode(AssignNext, code);
          Serial.print("Assigned RF433 code ");
          Serial.print(code);
          Serial.print(" to switch ");
          Serial.println(AssignNext);
          AssignNext = -1;
        }
        else
        {
          int i;
          for(i=0 ; i<NUM_SWITCHES ; i++)
          {
            if( options.RFCode(i+1) == code )
            {
              // matched RF code i, so hit correspoding switch
              switches[i].Hit();
              Serial.print("RF433 code matches switch ");
              Serial.println(i+1);
            }
          }
          if( i == NUM_SWITCHES )
          {
            Serial.print("RF433 code ");
            Serial.print(code);
            Serial.println(" not matched");
          }
        }
      }
      my433Switch.resetAvailable();
    }
  }
}
