#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <HACommon.h>
#include <HAHelper.h>

#define DOSERIAL 1

bool allInitialised = false;

// Node 2 is a simple HUB node - nothing complicated, just used as a network message relay

// Constants that identify this node and the node to send data to
const uint16_t this_node = 002;
const uint16_t control_node = 0;

// Time between checks (in ms)
const unsigned long loopTime = 100;

// set up the network:
const byte RFCE = 9; // transmitter
const byte RFCSN = 10; // transmitter
RF24 Radio(RFCE, RFCSN);
RF24Network Network(Radio);

class MyHANet: public HomeAutoNetwork
{
  public:
  MyHANet(RF24 *_r, RF24Network *_n):HomeAutoNetwork(_r, _n) {}
  virtual void OnMessage(uint16_t from_node, message_data *message)
  {
    switch( message->code )
    {
      // not actually expecting to receive any other messages:
      default:
        Serial.print(F("Received unexpected data from node "));
        Serial.print(from_node);
        Serial.print(F(" {Code: "));
        Serial.print(message->code);
        Serial.print(F(", Value: "));
        Serial.print(message->data[0]);
        Serial.println(F("}"));
      break;
    }
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

  // when we receive a MSG_UNKNOWN code (with data payload)
  virtual void OnUnknown(uint16_t from_node, message_data *_message) 
  {
   #if DOSERIAL
    // let's at least report the problem...
    Serial.print(F("Message returned UNKNOWN: "));
    Serial.println(_message->code);
   #endif
    sprintf(StatusMessage , "Received unknown code %d", _message->code);
    if( allInitialised )
    {
      // assume it's because of lost signal, so just reset ourselves...
      InitialiseMessaging(true);
    }
  }

  // Controller has told us to reset
  virtual void OnResetNeeded()
  {
   #if DOSERIAL
    // all we need to do is re-register the channels...
    Serial.println(F("RESET"));
   #endif
    strcpy(StatusMessage, "Resetting");
    SetStatus((char *)"Resetting");
    if(allInitialised)
    {
      InitialiseMessaging(true);
    }
  }
  // Controller has told us to reset
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

  // set up all the channels we care about...
  // _restart = true if this isn't the first intialise...
  void InitialiseMessaging(bool _restart=false)
  {
    char channel[64];
    sprintf(channel, "n%o/status", this_node);
    RegisterChannel( DT_TEXT, 101, StatusMessage, channel, _restart); // common status reporting method
    allInitialised = false;
  }

} HANetwork(&Radio, &Network);


void setup(void)
{
#if DOSERIAL
  // Set up the Serial Monitor
  Serial.begin(9600);
  Serial.println("Start");
#endif 
  // Initialize everything:
  //Radio.setPALevel(RF24_PA_MAX);
  Serial.println("SPI");
  SPI.begin();
  delay(5);
  Serial.println("HAN");
  HANetwork.Begin(this_node);
}

void loop() 
{
  // Update network data
  HANetwork.Update(loopTime);

  if( !allInitialised )
  {
    if( HANetwork.QueueEmpty() ) 
    {
      allInitialised = true;
      strcpy(StatusMessage, "OK");
      HANetwork.SetStatus((char *)"OK.");
    }
    else
    {
      //Serial.println("Queue still full...");
    }
  }
  // Wait a bit before we start over again
  delay(loopTime);
}
