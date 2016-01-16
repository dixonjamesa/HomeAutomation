#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <HACommon.h>
#include <HAHelper.h>
#include <CapacitiveSensor.h>

// PIN Configuration:

const byte senseTx = 3;
const byte senseRx = 4;
const byte senseFb = 8;
const byte RFCE = 9; // transmitter
const byte RFCSN = 10; // transmitter
const byte outputPin = 2;

// Constants that identify this node and the node to send data to
const uint16_t this_node = 011;
const uint16_t control_node = 0;

// the touch sensor
CapacitiveSensor CSensor = CapacitiveSensor(senseTx,senseRx);        // 10 megohm resistor between pins 4 & 2, pin 2 is sensor pin, add wire, foil

// Switch states
bool latchState=false;
bool toggleState=false;

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
    switch( message->code )
    {
      case 201: // monitor channel:
        // attach our output pin to this channel...
        Serial.print("Been told to monitor channel ");
        Serial.println(message->data);
        SubscribeChannel( DT_BOOL, outputPin, message->data );
      break;
      case 202: // reset
        // all we need to do is re-register the channels...
        Serial.println("Reset");
        InitialiseMessaging(true);
      break;
      default:
        bool state = (bool)message->data[0];
        Serial.print("Data received from node ");
        Serial.print(from_node);
        // Check value and change the pin...
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
      break;
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
    InitialiseMessaging(true);
  }
  void InitialiseMessaging(bool _restart)
  {
    char channel[64];
    sprintf(channel, "node%d/monitor", this_node);
    SubscribeChannel( DT_TEXT, 201, channel); // we will be told what channel to show the status of using our output (LED)
    sprintf(channel, "node%d/reset", this_node);
    SubscribeChannel( DT_BOOL, 202, channel); // we will be specifically told to reset on this channel
    sprintf(channel, "node%d/switch", this_node);
    RegisterChannel( DT_BOOL, 101, &latchState, 0, channel, _restart); // we send ON/OFF messages according to the capacitive switch on this channel
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
  CSensor.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example

  delay(50);

  HANetwork.InitialiseMessaging(false);
  Serial.println("Initialised");
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

long ThresholdHigh = 200;
long ThresholdLow = 120;
void Latch(long value)
{
  if(latchState == false && value > ThresholdHigh)
  {
    toggleState = !toggleState;
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

