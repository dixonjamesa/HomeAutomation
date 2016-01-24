#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <HACommon.h>
#include <HAHelper.h>

#define SERIALOUT 0

// PIN Configuration:
byte outputPin = 2;
const byte RFCE = 9; // transmitter
const byte RFCSN = 10; // transmitter

// Constants that identify this node and the node to send data to
const uint16_t this_node = 1;
const uint16_t control_node = 0;

// Radio with CE & CSN connected to pins 9 & 10
RF24 radio(RFCE, RFCSN);
RF24Network RFNetwork(radio);

// Time between checks (in ms)
const unsigned long interval = 100;

class MyHANet: public HomeAutoNetwork
{
  public:
  MyHANet(RF24Network *_net):HomeAutoNetwork(_net) {}
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
    InitialiseMessaging(true);
  }
  void InitialiseMessaging(bool _restart=false)
  {
    SubscribeChannel( DT_BOOL, outputPin, "kitchen/light1");
    RegisterChannel( DT_TEXT, 101, StatusMessage, "kitchen/hub/status", _restart); // common status reporting method
   #if SERIALOUT
    Serial.println("Initialised");
   #endif
    strcpy(StatusMessage,"Initialised OK");
  }

} HANetwork(&RFNetwork);


void setup(void)
{
#if SERIALOUT
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
  RFNetwork.txTimeout = 553;
  HANetwork.Begin(this_node);

  pinMode(outputPin, OUTPUT);

  delay(50);

  HANetwork.InitialiseMessaging();
}

void loop() 
{ 
  // Update network data
  RFNetwork.update();
  HANetwork.Update(interval);
  
  // Wait a bit before we start over again
  delay(interval);
}
