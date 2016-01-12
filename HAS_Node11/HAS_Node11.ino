#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <HACommon.h>
#include <HAHelper.h>
#include <CapacitiveSensor.h>

// PIN Configuration:

const bool senseTx = 3;
const bool senseRx = 4;
const bool senseFb = 8;
const bool RFCE = 9;
const bool RFCSN = 10;
const byte outputPin = 2;

// Constants that identify this node and the node to send data to
const uint16_t this_node = 011;
const uint16_t control_node = 0;

// the touch sensor
CapacitiveSensor CSensor = CapacitiveSensor(senseTx,senseRx);        // 10 megohm resistor between pins 4 & 2, pin 2 is sensor pin, add wire, foil

// Light relay pin
bool lightState;

// Radio with CE & CSN pins
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
      if( message->code == outputPin ) lightState = state; // store the state
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

  pinMode(outputPin, OUTPUT);
  CSensor.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example

  delay(50);

  initialisemessaging();
  Serial.println("Initialised");
}

void initialisemessaging()
{
  HANetwork.SubscribeChannel( DT_BOOL, outputPin, "kitchen/light1");
  HANetwork.RegisterChannel( DT_BOOL, 101, &lightState, 0, "kitchen/light1");
}

void loop() 
{
  
  // Update network data
  RFNetwork.update();
  HANetwork.Update();
  long total1 =  CSensor.capacitiveSensor(30);
  Latch(total1);
  Serial.println(total1);

  // Wait a bit before we start over again
  delay(interval);
}

bool latchState = false;
long ThresholdHigh = 200;
long ThresholdLow = 120;
void Latch(long value)
{
  if(latchState == false && value > ThresholdHigh)
  {
    Toggle();
    latchState = true;
  }
  else if( latchState == true && value < ThresholdLow )
  {
    latchState = false;
  }

  if( value > ThresholdLow)
  {
    digitalWrite(senseFb, HIGH);
  }
  else
  {
    digitalWrite(senseFb, LOW);
  }
}

void Toggle()
{
    lightState = !lightState;
    digitalWrite(outputPin, lightState);
    Serial.print("Toggle ");
    Serial.println(lightState);
}

