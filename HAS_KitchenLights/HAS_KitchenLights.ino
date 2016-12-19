#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <HACommon.h>
#include <HAHelper.h>

#define SERIALOUT 1

// PIN Configuration:
byte outputPin2 = 2;
byte outputPin3 = 3;
byte outputPin4 = 4;
byte outputPin5 = 5;
byte outputPin6 = 6;
byte outputPin7 = 7;
byte outputPin8 = 8;
const byte RFCE = 9; // transmitter
const byte RFCSN = 10; // transmitter

// Constants that identify this node and the node to send data to
const uint16_t this_node = 031;
const uint16_t control_node = 0;

bool allInitialised = false;

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
    if( allInitialised)
    {
      // assume it's because of lost signal, so just reset ourselves...
      InitialiseMessaging(true);
    }
  }
  // set up all the channels
  void InitialiseMessaging(bool _restart=false)
  {
    SubscribeChannel( DT_BOOL, outputPin2, "kitchen/lights/all");
    SubscribeChannel( DT_BOOL, outputPin3, "kitchen/lights/l2");
    SubscribeChannel( DT_BOOL, outputPin4, "kitchen/lights/l3");
    SubscribeChannel( DT_BOOL, outputPin5, "kitchen/lights/l4");
    SubscribeChannel( DT_BOOL, outputPin6, "kitchen/lights/l5");
    SubscribeChannel( DT_BOOL, outputPin7, "kitchen/lights/l6");
    SubscribeChannel( DT_BOOL, outputPin8, "kitchen/lights/l7");
    RegisterChannel( DT_TEXT, 101, StatusMessage, "kitchen/lights/status", _restart); // common status reporting method
   #if SERIALOUT
    Serial.println("Initialised");
   #endif
    strcpy(StatusMessage,"Initialising...");
    allInitialised = false;
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
  RFNetwork.txTimeout = 353;
  HANetwork.Begin(this_node);

  pinMode(outputPin2, OUTPUT);
  pinMode(outputPin3, OUTPUT);
  pinMode(outputPin4, OUTPUT);
  pinMode(outputPin5, OUTPUT);
  pinMode(outputPin6, OUTPUT);
  pinMode(outputPin7, OUTPUT);
  pinMode(outputPin8, OUTPUT);

  delay(50);

  HANetwork.InitialiseMessaging();
}

void loop() 
{ 
  // Update network data
  RFNetwork.update();
  HANetwork.Update(interval);
  if( !allInitialised )
  {
    if( HANetwork.QueueEmpty() ) 
    {
      allInitialised = true;
      strcpy(StatusMessage, "OK.");
    }
  }

  // Wait a bit before we start over again
  delay(interval);
}
