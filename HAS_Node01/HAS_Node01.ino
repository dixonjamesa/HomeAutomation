#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <HACommon.h>
#include <HAHelper.h>


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
const unsigned long interval = 500;

class MyHANet: public HomeAutoNetwork
{
  public:
  MyHANet(RF24Network *_net):HomeAutoNetwork(_net) {}
  virtual void OnMessage(uint16_t from_node, message_data *message)
  {
      bool state = (bool)message->data[0];
      Serial.print("Data received from node ");
      Serial.print(from_node);
      // Check value and change the pin...
      Serial.print(" {Id: ");
      Serial.print(message->code);
      Serial.print(", Value: ");
      Serial.print(state);
      Serial.println("}");
      if (!state) // relay is closed on 0, open on 1 (ie please invert message)
      {
        Serial.print("Sending HIGH to pin ");
        Serial.println(message->code);
        digitalWrite(message->code, HIGH);
      } 
      else 
      {
        Serial.print("Sending Low to pin ");
        Serial.println(message->code);
        digitalWrite(message->code, LOW);
      }
  }
  virtual void OnUnknown(uint16_t from_node, message_data *_message) 
  {
    // let's at least report the problem...
    Serial.print("Message returned UNKNOWN: ");
    Serial.println(_message->code);
  }
  virtual void OnResetNeeded()
  {
    // all we need to do is re-register the channels...
    Serial.println("RESET");
    InitialiseMessaging();
  }
  void InitialiseMessaging()
  {
    SubscribeChannel( DT_BOOL, outputPin, "home/kitchen/light1");
  }

} HANetwork(&RFNetwork);


void setup(void)
{
  // Set up the Serial Monitor
  Serial.begin(9600);
  Serial.println("Start");

  // Initialize all radio related modules
  SPI.begin();
  radio.begin();
  delay(5);
  RFNetwork.begin(90, this_node);
  HANetwork.Begin(this_node);

  pinMode(outputPin, OUTPUT);

  delay(50);

  HANetwork.InitialiseMessaging();
  Serial.println("Initialised");
}

void loop() 
{
  
  // Update network data
  RFNetwork.update();
  HANetwork.Update();
  
  // Wait a bit before we start over again
  delay(interval);
}
