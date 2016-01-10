#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <HACommon.h>
#include <HAHelper.h>


byte switchPin = 4;
bool doorState = false;
// LED (switch) PIN
byte led1Pin = 2;
byte led2Pin = 3;

// Radio with CE & CSN connected to pins 9 & 10
RF24 radio(9, 10);
RF24Network RFNetwork(radio);

// Constants that identify this node and the node to send data to
const uint16_t this_node = 1;
const uint16_t control_node = 0;

float gTemperature = 0;
int globalcount = 0;

char gMessage[64] = "";

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
      // Check value and turn the LED on or off
      Serial.print(" {Id: ");
      Serial.print(message->code);
      Serial.print(", Value: ");
      Serial.print(state);
      Serial.println("}");
      if (state) 
      {
        digitalWrite(message->code, HIGH);
      } 
      else 
      {
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

  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  pinMode(switchPin, INPUT_PULLUP);

  delay(50);

  doorState = digitalRead(switchPin);
  initialisemessaging();
  Serial.println("Initialised");
}

void initialisemessaging()
{
  HANetwork.SubscribeChannel( DT_BOOL, led1Pin, "node1/switch1");
  HANetwork.SubscribeChannel( DT_BOOL, led2Pin, "node1/switch2");
  HANetwork.RegisterChannel( DT_BOOL, switchPin, &doorState, 0, "node1/door");
  HANetwork.RegisterChannel( DT_TEXT, 101, gMessage, 0, "node1/name");
  HANetwork.RegisterChannel( DT_FLOAT, 102, &gTemperature, 0.25f, "node1/temperature");
}

void loop() 
{
  
  // Update network data
  RFNetwork.update();

/// RECEIVING MESSAGES
  HANetwork.Update();
  
/// SENDING MESSAGES

  doorState = digitalRead(switchPin);
  strcpy(gMessage, "FirstTest");
  gTemperature = 32.5f;

  // Wait a bit before we start over again
  delay(interval);
}
