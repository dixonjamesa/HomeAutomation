#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <HACommon.h>
#include <HAHelper.h>

#define DOSERIAL 1

// Node 2 is a simple HUB node - nothing complicated, just used as a network message relay
const byte RFCE = 9; // transmitter
const byte RFCSN = 10; // transmitter

// Constants that identify this node and the node to send data to
const uint16_t this_node = 002;
const uint16_t control_node = 0;

// Radio with CE & CSN pins
RF24 radio(RFCE, RFCSN);
RF24Network RFNetwork(radio);

bool allInitialised = false;

// Time between checks (in ms)
const unsigned long loopTime = 100;

class MyHANet: public HomeAutoNetwork
{
  public:
  MyHANet(RF24Network *_net):HomeAutoNetwork(_net) {}
  virtual void OnMessage(uint16_t from_node, message_data *message)
  {
    switch( message->code )
    {
      case 202: // reset
        Serial.println(F("Reset"));
      break;
      case 1:
        Serial.print(F("Data received from node "));
        Serial.print(from_node);
        // Check value and change the pin...
        Serial.print(F(" {Code: "));
        Serial.print(message->code);
        Serial.print(F(", Value: "));
        Serial.print(message->data[0]);
        Serial.println(F("}"));
        digitalWrite(message->code, (bool)message->data[0]);
      break;
      default:
        Serial.print(F("Received unexpected code "));
        Serial.println(message->code);
      break;
    }
  }
  
  // when we receive a MSG_UNKNOWN code (with data payload)
  virtual void OnUnknown(uint16_t from_node, message_data *_message) 
  {
    // let's at least report the problem...
    Serial.print(F("Message returned UNKNOWN: "));
    Serial.println(_message->code);
    if( allInitialised)
    {
      // assume it's because of lost signal, so just reset ourselves...
      InitialiseMessaging(true);
    }
  }

  // Controller has told us to reset
  virtual void OnResetNeeded()
  {
    // all we need to do is re-register the channels...
    Serial.println(F("RESET"));
    InitialiseMessaging(true);
  }
  
  void InitialiseMessaging(bool _restart=false)
  {
    char channel[64];
    sprintf(channel, "n%o/status", this_node);
    RegisterChannel( DT_TEXT, 101, StatusMessage, channel, _restart); // common status reporting method
    strcpy(StatusMessage, "Initialising...");
    sprintf(channel, "n%o/reset", this_node);
    SubscribeChannel( DT_TEXT, 202, channel); // we will be told to reset here
    allInitialised = false;
  }

} HANetwork(&RFNetwork);


void setup(void)
{
#if DOSERIAL
  // Set up the Serial Monitor
  Serial.begin(9600);
  Serial.println("Start");
#endif 

  // Initialize all radio related modules
  SPI.begin();
  radio.begin();
  delay(5);
  RFNetwork.begin(120, this_node);
  radio.setRetries(8,11);
  radio.setPALevel(RF24_PA_MAX);
  RFNetwork.txTimeout = 483;
  
  HANetwork.Begin(this_node);

  delay(50);

  HANetwork.InitialiseMessaging();
#if DOSERIAL
  Serial.println(F("Initialised"));
#endif
}

unsigned long mils;

void loop() 
{
  // Update network data
  RFNetwork.update();
  HANetwork.Update(loopTime);

  if( !allInitialised )
  {
    if( HANetwork.QueueEmpty() ) 
    {
      allInitialised = true;
      strcpy(StatusMessage, "OK.");
    }
  }
  delay(loopTime);
}
