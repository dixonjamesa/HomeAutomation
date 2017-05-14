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
const uint16_t this_node = 0131;
const uint16_t control_node = 0;

// Switch states
bool switchIsOn=false; // whether the output switch is on
int switchOnTimer = 0; // timer for how long the switch remains on
const int switchOnTime = 600; // how long to leave it on
bool switchDummy = false; // use this for turning the switch off after a delay (using a force send)
long lockoutTimer = 0; // timer to avoid door opening as network connections start up

byte backDoorSensorState=0; // back door state
byte rollerDoorSensorState=0; // roller door state

// indicator globals
bool indicatorLoopUp = true; // direction we're fading in
int indicatorFadeValue = 0; // current fade value
int indicatorFadeMax = 255; // max fade value

// Radio with CE & CSN pins
RF24 RFRadio(RFCE, RFCSN);
RF24Network RFNetwork(RFRadio);

// Time between checks (in ms)
const unsigned long loopTime = 100;

class MyHANet: public HomeAutoNetwork
{
  public:
  MyHANet():HomeAutoNetwork(&RFRadio, &RFNetwork) {}
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
      case 201:
        switchIsOn = (bool)message->data[0];
        Serial.print(F("Switch to "));
        Serial.println(switchIsOn);
        if( lockoutTimer > (long)1000*60 )
        { // Cannot automatically open the door for the first minute...
          digitalWrite(switchPin, switchIsOn);
        }
      break;
      default:
        Serial.println(F(" - unexpected code"));
      break;
    }
  }

  virtual void OnAwake(bool _ack)
  {
    // good practice to send subscriptions again just in case there's been a problem
    if( !_ack )
    {
      strcpy(StatusMessage,"OK.");
      ForceSendRegisteredChannel(101);
      SetStatus((char *)"Initialising");
    }
    InitialiseMessaging(_ack);
  }
  
  // when we receive a MSG_UNKNOWN code (with data payload)
  virtual void OnUnknown(uint16_t from_node, message_data *_message) 
  {
    // let's at least report the problem...
    Serial.print(F("Message returned UNKNOWN: "));
    Serial.println(_message->code);
    sprintf(StatusMessage , "Received unknown code %d", _message->code);
    if( allInitialised )
    {
      // assume it's because of lost signal, so just reset ourselves...
      InitialiseMessaging(true);
    }
  }

  // Controller has told us to reset
  virtual void OnResetNeeded()
  {
   #if DOSERIAL
    // all we need to do is re-register the channels...
    Serial.println(F("RESET"));
   #endif
    strcpy(StatusMessage, "Resetting");
    SetStatus((char *)"Resetting");
    if(allInitialised)
    {
      InitialiseMessaging(true);
    }
  }
    // Controller has told us to resend
  virtual void OnResendNeeded()
  {
   #if DOSERIAL
    // all we need to do is re-register the channels...
    Serial.println(F("RESEND"));
   #endif
    strcpy(StatusMessage, "Resending");
    SetStatus((char *)"Resending");
    if(allInitialised)
    {
      InitialiseMessaging(true);
    }
  }

  // set up all the channels we care about...
  // _restart = true if this isn't the first intialise...
  void InitialiseMessaging(bool _restart=false)
  {
    RegisterChannel( DT_TEXT, 101, StatusMessage, "garage/status", _restart); // common status reporting method
    strcpy(StatusMessage, "Initialising...");
    
    SubscribeChannel( DT_BOOL, 201, "garage/switch"); // door switch state from elsewhere in the network
    RegisterChannel( DT_BOOL, 104, &switchDummy, 0, "garage/switch", _restart); // allow us to turn the switch off again
    
    RegisterChannel( DT_BYTE, 102, &backDoorSensorState, 0, "garage/bdoor", _restart); 
    
    RegisterChannel( DT_BYTE, 103, &rollerDoorSensorState, 0, "garage/gdoor", _restart); 
    
    allInitialised = false;
  }

} HANetwork;//&Radio, &RFNetwork);


void setup(void)
{
#if SERIALOUT
  // Set up the Serial Monitor
  Serial.begin(9600);
  Serial.println("Start");
#endif
  // Initialize all radio related modules
  SPI.begin();
  delay(5);
  HANetwork.Begin(this_node);  

  pinMode(outputLEDRPin, OUTPUT);
  pinMode(outputLEDGPin, OUTPUT);
  pinMode(outputLEDBPin, OUTPUT);
  pinMode(switchPin, OUTPUT);
  pinMode(SensorRollerDoor, INPUT);
  pinMode(SensorBackDoor, INPUT);
  digitalWrite(switchPin,false);
  SetIndicator(255,255,255); // white
}

void loop() 
{
  // Update network data
  HANetwork.Update(loopTime);

  if( !allInitialised )
  {
    if( HANetwork.QueueEmpty() ) 
    {
      allInitialised = true;
      strcpy(StatusMessage, "OK.");
      HANetwork.SetStatus((char *)"OK.");
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
    { // tamper
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
      SetIndicator(180,255,0); // red/orange
      strcpy( StatusMessage,"Garage Door Open");
    }
    else if( backDoorSensorState == 0 )
    {
      SetIndicator(0,60,255); // cyan
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
      SetIndicator(0,0,255); // blue
      if( switchOnTimer > switchOnTime )
      {
        switchDummy = false;
        HANetwork.ForceSendRegisteredChannel(104);
        switchOnTimer = 0;
        switchIsOn = false;
        digitalWrite(switchPin, false);
        Serial.println("Switch is OFF");
      }
    }
  }

  // ensure the door doesn't get opened in the first minute:
  lockoutTimer += loopTime;
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


