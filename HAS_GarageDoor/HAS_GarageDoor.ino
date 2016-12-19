#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <HACommon.h>
#include <HAHelper.h>

#define SERIALOUT 1

bool allInitialised = false;

const byte RFCE = 9; // transmitter
const byte RFCSN = 10; // transmitter
const byte SensorRollerDoor = A1;
const byte SensorBackDoor = A2;

const byte switchPin = 2; // output to RC switch
const byte outputLEDRPin = 3;
const byte outputLEDGPin = 5;
const byte outputLEDBPin = 6;

// Constants that identify this node and the node to send data to
const uint16_t this_node = 022;
const uint16_t control_node = 0;

// Switch states
bool switchIsOn=false; // whether the output switch is on
int switchOnTimer = 0; // timer for how long the switch remains on
const int switchOnTime = 600; // how long to leave it on
bool switchDummy = false; // use this for turning the switch off after a delay (using a force send)

byte backDoorSensorState=0; // back door state
byte rollerDoorSensorState=0; // roller door state

// indicator globals
bool indicatorLoopUp = true; // direction we're fading in
int indicatorFadeValue = 0; // current fade value
int indicatorFadeMax = 255; // max fade value

// Radio with CE & CSN pins
RF24 radio(RFCE, RFCSN);
RF24Network RFNetwork(radio);

// Time between checks (in ms)
const unsigned long loopTime = 100;

class MyHANet: public HomeAutoNetwork
{
  public:
  MyHANet(RF24Network *_net):HomeAutoNetwork(_net) {}
  virtual void OnMessage(uint16_t from_node, message_data *message)
  {
    Serial.print(F("Data received from node "));
    Serial.print(from_node);
    // Check value and change the pin...
    Serial.print(F(", Code: "));
    Serial.print(message->code);    
    Serial.print(F(" - "));
    switch( message->code )
    {
      case 202: // reset
        Serial.println(F("Reset"));
        OnResetNeeded();
      break;
      case 201:
        switchIsOn = (bool)message->data[0];
        Serial.print(F("Switch to "));
        Serial.println(switchIsOn);
        digitalWrite(switchPin, switchIsOn);
      break;
      default:
        Serial.println(F(" - unexpected code"));
      break;
    }
  }

  void OnAwake(bool success)
  {
    if( success)
    {
      strcpy(StatusMessage,"OK.");
      ForceSendRegisteredChannel(101);
    }
  }
  // when we receive a MSG_UNKNOWN code (with data payload)
  virtual void OnUnknown(uint16_t from_node, message_data *_message) 
  {
    // let's at least report the problem...
    Serial.print(F("Message returned UNKNOWN: "));
    Serial.println(_message->code);
    if( allInitialised )
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

    sprintf(channel, "n%o/switch", this_node);
    SubscribeChannel( DT_BOOL, 201, channel); // door switch state from elsewhere in the network
    RegisterChannel( DT_BOOL, 104, &switchDummy, 0, channel, _restart); // allow us to turn the switch off again
    
    sprintf(channel, "n%o/bdoor", this_node);
    RegisterChannel( DT_BYTE, 102, &backDoorSensorState, 0, channel, _restart); 
    
    sprintf(channel, "n%o/gdoor", this_node);
    RegisterChannel( DT_BYTE, 103, &rollerDoorSensorState, 0, channel, _restart); 
    
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
  radio.setPALevel(RF24_PA_MAX);
  radio.setRetries(8,11);
  RFNetwork.txTimeout = 137;
  
  HANetwork.Begin(this_node);

  pinMode(outputLEDRPin, OUTPUT);
  pinMode(outputLEDGPin, OUTPUT);
  pinMode(outputLEDBPin, OUTPUT);
  pinMode(switchPin, OUTPUT);
  pinMode(SensorRollerDoor, INPUT);
  pinMode(SensorBackDoor, INPUT);
  digitalWrite(switchPin,false);
  SetIndicator(255,255,255); // white

  delay(50);

  HANetwork.InitialiseMessaging();
  Serial.println(F("Initialised"));
}

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
    else
    {
      SetIndicator(0,255,255); // cyan
    }
  }
  else
  {
    backDoorSensorState = ProcessInputValue(analogRead(SensorBackDoor));
    rollerDoorSensorState = ProcessInputValue(analogRead(SensorRollerDoor));
    if( backDoorSensorState == 2 || rollerDoorSensorState == 2 )
    {
      SetIndicator(255,0,0); // red
      UpdateIndicator(loopTime); // flash faster
      if( backDoorSensorState == 2 )
      {
        sprintf( StatusMessage, "Tamper Back Door %d", analogRead(SensorBackDoor));
      }
      else
      {
        sprintf( StatusMessage, "Tamper Garage %d", analogRead(SensorRollerDoor));
      }
    }
    else if( rollerDoorSensorState == 0 )
    {
      SetIndicator(100,255,0); // orange
      strcpy( StatusMessage,"Garage Door Open");
    }
    else if( backDoorSensorState == 0 )
    {
      SetIndicator(10,120,255); // cyan
      strcpy( StatusMessage,"Back Door Open");
    }
    else
    {
      SetIndicator(0,255,0); // green      
      strcpy( StatusMessage,"OK.");
    }

    if( switchIsOn )
    {
      switchOnTimer += loopTime;
      Serial.println(switchOnTimer);
      indicatorFadeValue = indicatorFadeMax;
      SetIndicator(0,100,255); // blue
      if( switchOnTimer > switchOnTime )
      {      
        HANetwork.ForceSendRegisteredChannel(104);
        switchOnTimer = 0;
        switchIsOn = false;
        digitalWrite(switchPin, false);
        Serial.println("Switch is OFF");
      }
    }
  }
  delay(loopTime);
  UpdateIndicator(loopTime);
}

// update the indicator fading
void UpdateIndicator(int ltime)
{
  if( indicatorLoopUp )
  {
    indicatorFadeValue += 25;
    if( indicatorFadeValue > indicatorFadeMax )
    {
      indicatorFadeValue = indicatorFadeMax;
      indicatorLoopUp = false;
    }
  }
  else
  {
    indicatorFadeValue -= 25;
    if( indicatorFadeValue < 0 )
    {
      indicatorFadeValue = 0;
      indicatorLoopUp = true;
    }
  }
}

// threshold levels...
const int TamperLowThreshold = 100; // below this likely short
// close 550, open 850
const int OpenThreshold = 700;
const int TamperHighThreshold = 1000; // above this likely o/c (wires cut)
// return relevant code given analog reading from sensor
// 0 - open
// 1 - closed
// 2 - tamper
byte ProcessInputValue(int value)
{
  if( value < TamperLowThreshold || value > TamperHighThreshold)
  {
    return 2; // tamper
  }
  if( value < OpenThreshold )
  {
    return 1; // closed
  }
  return 0; // open
}

void SetIndicator(int r, int g, int b)
{
  analogWrite(outputLEDRPin, r*indicatorFadeValue/indicatorFadeMax);
  analogWrite(outputLEDGPin, g*indicatorFadeValue/indicatorFadeMax);
  analogWrite(outputLEDBPin, b*indicatorFadeValue/indicatorFadeMax);
}


