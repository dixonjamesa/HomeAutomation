#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <HACommon.h>
#include <HAHelper.h>
#include <CapacitiveSensor.h>

// PIN Configuration:

const byte RFCE = 9; // transmitter
const byte RFCSN = 10; // transmitter
const byte outputRollerDoorStatusPin = 2;
const byte outputGarageDoorStatusPin = 3;
const byte outputSwitchPin = 4;
const byte inputRollerDoorPin = 5;
const byte inputGarageDoorPin = 6;

// Constants that identify this node and the node to send data to
const uint16_t this_node = 021; // second child node of node 1 (kitchen light hub)
const uint16_t control_node = 0;

// Switch states
bool rollerDoorState=false;
bool garageDoorState=false;
bool switchState=false;
bool resendsetup=false;

// Radio with CE & CSN pins
RF24 radio(RFCE, RFCSN);
RF24Network RFNetwork(radio);

// Time between checks (in ms)
const unsigned long loopTime = 200;

class MyHANet: public HomeAutoNetwork
{
  public:
  MyHANet(RF24Network *_net):HomeAutoNetwork(_net) {}
  virtual void OnMessage(uint16_t from_node, message_data *message)
  {
    Serial.print("Received message ");
    Serial.println(message->code);
    switch( message->code )
    {
      case 202: // reset
        // all we need to do is re-register the channels...
        Serial.println("Reset");
        InitialiseMessaging(true);
      break;
      case outputGarageDoorStatusPin:
      case outputRollerDoorStatusPin:
      case outputSwitchPin:
        digitalWrite(message->code, (bool)message->data[0]);
      break;
      default:
        Serial.print("Received unexpected code ");
        Serial.println(message->code);
      break;
    }
  }
  
  // when we receive a MSG_UNKNOWN code (with data payload)
  virtual void OnUnknown(uint16_t from_node, message_data *_message) 
  {
    // let's at least report the problem...
    Serial.print("Message returned UNKNOWN: ");
    Serial.println(_message->code);
  }

  // Controller has told us to reset
  virtual void OnResetNeeded()
  {
    // all we need to do is re-register the channels...
    Serial.println("RESET");
    InitialiseMessaging(true);
  }
  
  void InitialiseMessaging(bool _restart=false)
  {
    char channel[64];
    strcpy(StatusMessage, "Waiting for monitor...");
    sprintf(channel, "n%o/status", this_node);
    RegisterChannel( DT_TEXT, 101, StatusMessage, channel, _restart); // common status reporting method
    sprintf(channel, "n%o/gdoor", this_node);
    RegisterChannel( DT_BOOL, outputRollerDoorStatusPin, &rollerDoorState, 0, channel, _restart); // roller door state
    sprintf(channel, "n%o/bdoor", this_node);
    RegisterChannel( DT_BOOL, outputGarageDoorStatusPin, &garageDoorState, 0, channel, _restart); // back door state
    sprintf(channel, "n%o/switch", this_node);
    SubscribeChannel( DT_BOOL, outputSwitchPin, channel); // switch
    sprintf(channel, "n%o/reset", this_node);
    SubscribeChannel( DT_TEXT, 202, channel); // we will be told to reset here
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
  RFNetwork.begin(120, this_node);
  radio.setRetries(8,11);
  RFNetwork.txTimeout = 151;
  
  HANetwork.Begin(this_node);

  pinMode(outputRollerDoorStatusPin, OUTPUT); // LED status
  pinMode(outputGarageDoorStatusPin, OUTPUT); // LED status
  pinMode(outputSwitchPin, OUTPUT); // Door trigger
  pinMode(inputGarageDoorPin, INPUT); // Door status
  pinMode(inputRollerDoorPin, INPUT); // Door status
  pinMode(13, OUTPUT);


  delay(50);

  HANetwork.InitialiseMessaging();
  Serial.println("Setup complete.");
}

void loop() 
{
  // Update network data
  RFNetwork.update();
  HANetwork.Update(loopTime);

  garageDoorState = digitalRead(inputGarageDoorPin);
  rollerDoorState = digitalRead(inputRollerDoorPin);
  // Wait a bit before we start over again
  delay(loopTime);
}

