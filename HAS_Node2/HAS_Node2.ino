#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <HACommon.h>
#include <HAHelper.h>

#define DOSERIAL 1

bool allInitialised = false;

// Node 2 is a simple HUB node - nothing complicated, just used as a network message relay

const byte RFCE = 9; // transmitter
const byte RFCSN = 10; // transmitter

// Constants that identify this node and the node to send data to
const uint16_t this_node = 002;
const uint16_t control_node = 0;

// Time between checks (in ms)
const unsigned long loopTime = 100;

class MyHANet: public HomeAutoNetwork
{
  public:
  MyHANet(byte CE, byte CSN):HomeAutoNetwork(CE,CSN) {}
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
  
  // when we receive a MSG_UNKNOWN code (with data payload)
  virtual void OnUnknown(uint16_t from_node, message_data *_message) 
  {
   #if DOSERIAL
    // let's at least report the problem...
    Serial.print(F("Message returned UNKNOWN: "));
    Serial.println(_message->code);
   #endif
    sprintf(StatusMessage , "Received unknown code %d", _message->code);
  }

  // Controller has told us to reset
  virtual void OnResetNeeded()
  {
   #if DOSERIAL
    // all we need to do is re-register the channels...
    Serial.println(F("RESET"));
   #endif
    strcpy(StatusMessage, "Resetting");
    if(allInitialised)
    {
      InitialiseMessaging(true);
    }
  }
  
  void InitialiseMessaging(bool _restart=false)
  {
    char channel[64];
    sprintf(channel, "n%o/status", this_node);
    RegisterChannel( DT_TEXT, 101, StatusMessage, channel, _restart); // common status reporting method
   #if DOSERIAL
    Serial.println("Initialised");
   #endif
    //sprintf(channel, "n%o/reset", this_node);
    //SubscribeChannel( DT_TEXT, 202, channel); // we will be told to reset here
    strcpy(StatusMessage, "Initialising");
    allInitialised = false;
  }

} HANetwork(RFCE, RFCSN);


void setup(void)
{
#if DOSERIAL
  // Set up the Serial Monitor
  Serial.begin(9600);
  Serial.println("Start");
#endif 
  // Initialize everything:
  SPI.begin();
  delay(5);
  HANetwork.Begin(120, this_node, 483);
  delay(50);
  HANetwork.InitialiseMessaging();
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
      strcpy(StatusMessage, "OK.");
    }
  }
  // Wait a bit before we start over again
  delay(loopTime);
}
