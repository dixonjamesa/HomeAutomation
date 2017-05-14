#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <HACommon.h>
#include <HAHelper.h>

#define SERIALOUT 1

bool allInitialised = false;

// PIN Configuration:
byte outputPin = 2;

// Constants that identify this node and the node to send data to
const uint16_t this_node = 1;
const uint16_t control_node = 0;

// Time between checks (in ms)
const unsigned long interval = 100;

// Network setup
const byte RFCE = 9; // transmitter
const byte RFCSN = 10; // transmitter
// Radio with CE & CSN connected to pins 9 & 10
RF24 RFRadio(RFCE, RFCSN);
RF24Network RFNetwork(RFRadio);

class MyHANet: public HomeAutoNetwork
{
  public:
  MyHANet():HomeAutoNetwork(&RFRadio, &RFNetwork) {}
  virtual void OnMessage(uint16_t from_node, message_data *message)
  {
      bool state = (bool)message->data[0];
   #if SERIALOUT
      Serial.print("Data received from node ");
      Serial.print(from_node);
      // Check value and change the pin...
      Serial.print(" {Id: ");
      Serial.print(message->code);
      Serial.print(", Value: ");
      Serial.print(state);
      Serial.println("}");
    #endif
      digitalWrite(message->code, !state); // relay is closed on 0, open on 1 (ie please invert message)
  }

  virtual void OnAwake(bool _ack)
  {
    // good practice to send subscriptions again just in case there's been a problem
    if( !_ack)
    {
     #if DOSERIAL
      Serial.println("Initialising");
     #endif
      strcpy(StatusMessage, "Initialising");
      SetStatus((char *)"Initialising");
    }
    InitialiseMessaging(_ack);
  }

  virtual void OnUnknown(uint16_t from_node, message_data *_message) 
  {
   #if SERIALOUT
    // let's at least report the problem...
    Serial.print("Message returned UNKNOWN: ");
    Serial.println(_message->code);
   #endif
    sprintf(StatusMessage , "Received unknown code %d", _message->code);
  }
  
  virtual void OnResetNeeded()
  {
   #if SERIALOUT
    // all we need to do is re-register the channels...
    Serial.println("RESET");
   #endif
    strcpy(StatusMessage, "Resetting");
    SetStatus((char *)"Resetting");
    if(allInitialised)
    {
      InitialiseMessaging(true);
    }
  }
  // Controller has told us to resend
  virtual void OnResendNeeded()
  {
   #if DOSERIAL
    // all we need to do is re-register the channels...
    Serial.println(F("RESEND"));
   #endif
    strcpy(StatusMessage, "Resending");
    SetStatus((char *)"Resending");
    if(allInitialised)
    {
      InitialiseMessaging(true);
    }
  }


  void InitialiseMessaging(bool _restart=false)
  {
    SubscribeChannel( DT_BOOL, outputPin, "kitchen/lights/l1");
    RegisterChannel( DT_TEXT, 101, StatusMessage, "kitchen/hub/status", _restart); // common status reporting method
    allInitialised = false;
  }

} HANetwork;


void setup(void)
{
#if SERIALOUT
  // Set up the Serial Monitor
  Serial.begin(9600);
  Serial.println("Start");
#endif
  // Initialize all radio related modules
  SPI.begin();
  delay(5);
  HANetwork.Begin(this_node);

  pinMode(outputPin, OUTPUT);

  delay(50);
}

void loop() 
{ 
  // Update network data
  HANetwork.Update(interval);

  if( !allInitialised )
  {
    if( HANetwork.QueueEmpty() ) 
    {
      allInitialised = true;
      strcpy(StatusMessage, "OK.");
      HANetwork.SetStatus((char *)"OK.");
    }
  }
  // Wait a bit before we start over again
  delay(interval);
}
