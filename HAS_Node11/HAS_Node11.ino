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

//initialisation state
bool allInitialised=false;

// Switch states
bool latchState=false;
bool toggleState=false;
bool resendsetup=false;
bool dummy=false; // send toggle message whenever this changes
unsigned long touchTime;
unsigned long lightOffDelay = 0;

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
    switch( message->code )
    {
      case 201: // monitor channel:
        // attach our output pin to this channel...
        Serial.print("Been told to monitor channel ");
        Serial.println(message->data);
        SubscribeChannel( DT_BOOL, outputPin, message->data );
        RegisterChannel( DT_TOGGLE, 102, &dummy, 0, message->data, false ); // so we can send messages on this code
        //RegisterChannel( DT_BOOL, 102, &dummy, 1, message->data, false ); // so we can send messages on this code
        allInitialised = true;
      break;
      case 202: // reset
        // all we need to do is re-register the channels...
        Serial.println("Reset");
        InitialiseMessaging(true);
      break;
      case outputPin:
        toggleState = (bool)message->data[0];
        Serial.print("Data received from node ");
        Serial.print(from_node);
        // Check value and change the pin...
        Serial.print(" {Code: ");
        Serial.print(message->code);
        Serial.print(", Value: ");
        Serial.print(toggleState);
        Serial.println("}");
        digitalWrite(outputPin, toggleState);
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
    sprintf(channel, "home/node%o/monitor", this_node);
    SubscribeChannel( DT_TEXT, 201, channel); // we will be told what channel to do switching for
    sprintf(channel, "home/node%o/reset", this_node);
    SubscribeChannel( DT_TEXT, 202, channel); // we will be told to reset here
    sprintf(channel, "home/node%o/sendsetup", this_node);
    RegisterChannel( DT_BYTE, 200, &resendsetup, 0, channel, _restart); // use this to prompt that we still aren't set up yet
    sprintf(channel, "home/node%o/status", this_node);
    RegisterChannel( DT_TEXT, 101, StatusMessage, channel, _restart); // common status reporting method
    allInitialised = false;
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
  radio.setRetries(8,11);
  RFNetwork.txTimeout = 529;
  
  HANetwork.Begin(this_node);

  pinMode(outputPin, OUTPUT);
  pinMode(senseFb, OUTPUT);
  CSensor.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example

  delay(50);

  HANetwork.InitialiseMessaging();
  Serial.println("Initialised");
}

void loop() 
{
  long total;  
  // Update network data
  RFNetwork.update();
  HANetwork.Update(loopTime);
  
  total =  CSensor.capacitiveSensor(30);
  Latch(total);
  //Serial.println(total);
  // Wait a bit before we start over again
  delay(loopTime);

  if( !allInitialised )
  {
    delay(400);
    toggleState = !toggleState;
    digitalWrite(outputPin, toggleState);
    resendsetup = !resendsetup; // trigger a message
  }

  if( lightOffDelay > 0 )
  {
      // flash to show we're waiting to turn the light off
      digitalWrite(senseFb, !bitRead(PORTD,senseFb));
      if( lightOffDelay < loopTime )
      {
        lightOffDelay = 0;
        
      }
      else
      {
        lightOffDelay -= loopTime;
      }
  }
}

long ThresholdHigh = 200;
long ThresholdLow = 120;
void Latch(long value)
{
  if(latchState == false && value > ThresholdHigh)
  {
    latchState = true;
    touchTime = 0;
    lightOffDelay = 0; // cancel an off delay
    digitalWrite(senseFb, latchState);
  }
  else if( latchState == true && value < ThresholdLow )
  {
    latchState = false;
    if( lightOffDelay == 0 )
    {
      // send a toggle message
      toggleState = !toggleState;
      digitalWrite(outputPin, toggleState); // do this here for maximum responsiveness
      dummy = !dummy;// trigger send of the togglemessage
      //message_data msg;
      //msg.code = 102;
      //msg.type = DT_BOOL;
      //memcpy(msg.data, &toggleState, 1);
      //HANetwork.SendMessage(&msg, 1);
    }
    touchTime = 0;
    digitalWrite(senseFb, latchState);
  }
  else if( latchState == true )
  {
    touchTime += loopTime;
    if( touchTime > 1000 )
    {
      lightOffDelay = 3000;
    }
  }
}

