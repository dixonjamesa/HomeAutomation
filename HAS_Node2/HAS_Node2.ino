#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <HACommon.h>
#include <HAHelper.h>
#include <DHT.h>

// PIN Configuration:

const byte RFCE = 9; // transmitter
const byte RFCSN = 10; // transmitter
const byte outputPin1 = 6;
const byte outputPin2 = 7;
const byte LDRPin = A7;
const byte motionPin = 2;
const byte switchPin = 5;
const byte tempPin = 3;

// Constants that identify this node and the node to send data to
const uint16_t this_node = 021;
const uint16_t control_node = 0;

#define DHTTYPE 22
DHT dht(tempPin, DHTTYPE);

byte lightSensor;
bool motionSensor;
float temperature;
float humidity;

bool allInitialised = false;

// Switch states
bool latchState=false;
bool toggleState=false;
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
    switch( message->code )
    {
      case 202: // reset
        Serial.println(F("Reset"));
      break;
      case outputPin1:
      case outputPin2:
        toggleState = (bool)message->data[0];
        Serial.print(F("Data received from node "));
        Serial.print(from_node);
        // Check value and change the pin...
        Serial.print(F(" {Code: "));
        Serial.print(message->code);
        Serial.print(F(", Value: "));
        Serial.print(toggleState);
        Serial.println(F("}"));
        digitalWrite(message->code, toggleState);
      break;
      default:
        Serial.print(F("Received unexpected code "));
        Serial.println(message->code);
      break;
    }
  }
  
  // when we receive a MSG_UNKNOWN code (with data payload)
  virtual void OnUnknown(uint16_t from_node, message_data *_message) 
  {
    // let's at least report the problem...
    Serial.print(F("Message returned UNKNOWN: "));
    Serial.println(_message->code);
    if( allInitialised)
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

    sprintf(channel, "n%o/brightness", this_node);
    RegisterChannel( DT_BYTE, 102, &lightSensor, 5, channel, _restart); 
    sprintf(channel, "n%o/motion", this_node);
    RegisterChannel( DT_BOOL, 103, &motionSensor, 0, channel, _restart); 
    sprintf(channel, "n%o/temp", this_node);
    RegisterChannel( DT_FLOAT, 104, &temperature, 0.1f, channel, _restart);
    sprintf(channel, "n%o/humid", this_node);
    RegisterChannel( DT_FLOAT, 105, &humidity, 1, channel, _restart);
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
  RFNetwork.begin(120, this_node);
  radio.setRetries(8,11);
  RFNetwork.txTimeout = 537;
  
  HANetwork.Begin(this_node);

  pinMode(outputPin1, OUTPUT);
  pinMode(outputPin2, OUTPUT);
  pinMode(LDRPin, INPUT);
  pinMode(motionPin, INPUT);
  pinMode(switchPin, INPUT_PULLUP);

  delay(50);

  HANetwork.InitialiseMessaging();
  Serial.println(F("Initialised"));
}

void loop() 
{
  // Update network data
  RFNetwork.update();
  HANetwork.Update(loopTime);

  if( !allInitialised && HANetwork.QueueEmpty() ) 
  {
    allInitialised = true;
    strcpy(StatusMessage, "OK.");
  }

  // read our data pins...
  int raw = analogRead(LDRPin);
  raw -= 300; if( raw<0) raw=0;
  raw = raw/6; if (raw>100) raw=100;
  lightSensor = raw; // scaling through looking at readings.
  motionSensor = digitalRead(motionPin);
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  digitalWrite(outputPin1, motionSensor);
/*
  Serial.print(temperature);
  Serial.print(F("deg, "));
  Serial.print(humidity);
  Serial.print(F("hum, "));
  Serial.print(motionSensor);
  Serial.print(F("MO, "));
  Serial.println(lightSensor);*/
  // Wait a bit before we start over again
  delay(loopTime);
}

