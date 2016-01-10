#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <HACommon.h>
#include <HAHelper.h>


// Light relay pin
byte outputPin = 2;

// Radio with CE & CSN connected to pins 9 & 10
RF24 radio(9, 10);
RF24Network RFNetwork(radio);

// Constants that identify this node and the node to send data to
const uint16_t this_node = 1;
const uint16_t control_node = 0;

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
  HANetwork.Begin();

  pinMode(outputPin, OUTPUT);

  delay(50);

  initialisemessaging();
  Serial.println("Initialised");
}

void initialisemessaging()
{
  HANetwork.SubscribeChannel( DT_BOOL, outputPin, "kitchen/light1");
}

void loop() 
{
  
  // Update network data
  RFNetwork.update();
  HANetwork.Update();
  
  // Wait a bit before we start over again
  delay(interval);
}
