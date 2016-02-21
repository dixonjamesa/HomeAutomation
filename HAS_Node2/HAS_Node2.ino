#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <HACommon.h>
#include <HAHelper.h>
#include <DHT.h>

  static const byte digitCodeMap[] = {
    B00111111, // 0
    B00000110, // 1
    B01011011, // 2
    B01001111, // 3
    B01100110, // 4
    B01101101, // 5
    B01111101, // 6
    B00000111, // 7
    B01111111, // 8
    B01101111, // 9
    B00000000, // BLANK
    B01000000    }; // DASH
// PIN Configuration:

const byte RFCE = 9; // transmitter
const byte RFCSN = 10; // transmitter
const byte SRData = 6;
const byte SRLatch = 7;
const byte SRClock = 8;
const byte Digit1 = A1;
const byte Digit2 = A2;
const byte Digit3 = A3;
const byte Digit4 = A4;

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
      case 1:
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

  pinMode(Digit1, OUTPUT);
  pinMode(Digit2, OUTPUT);
  pinMode(Digit3, OUTPUT);
  pinMode(Digit4, OUTPUT);
  pinMode(SRData, OUTPUT);
  pinMode(SRClock, OUTPUT);
  pinMode(SRLatch, OUTPUT);
  pinMode(LDRPin, INPUT);
  pinMode(motionPin, INPUT);
  pinMode(switchPin, INPUT_PULLUP);

  delay(50);

  HANetwork.InitialiseMessaging();
  Serial.println(F("Initialised"));
}

unsigned long mils;

void debugTime(char *str)
{
  /*
  unsigned long now = millis();
  int td = now-mils;
  mils=now; 
  Serial.print(str);
  Serial.print(": ");
  Serial.print(td);
  Serial.print("; ");*/
}
void loop() 
{
  debugTime("Start");
  // Update network data
  RFNetwork.update();
  debugTime("RFNetwork");
  HANetwork.Update(loopTime);
  debugTime("HANetwork");

  if( !allInitialised && HANetwork.QueueEmpty() ) 
  {
    allInitialised = true;
    strcpy(StatusMessage, "OK.");
  }
  debugTime("A1");

  // read our data pins...
  int raw = analogRead(LDRPin);
  raw -= 300; if( raw<0) raw=0;
  raw = raw/6; if (raw>100) raw=100;
  lightSensor = raw; // scaling through looking at readings.
  motionSensor = digitalRead(motionPin);
  temperature = dht.readTemperature();
  //humidity = dht.readHumidity();
  debugTime("A2");

  if( !digitalRead(switchPin) )
  {
    for(int i=0;i<100;i++)
    {
      float td = temperature;
      td = td/10;
      int d1 = td;
      int d2 = td*10-d1*10;
      int d3 = td*100-d1*100-d2*10;
      int d4=0;
      displayData(digitCodeMap[d1], digitCodeMap[d2]+128,digitCodeMap[d3],digitCodeMap[d4]);
    }
  }
/*
  Serial.print(temperature);
  Serial.print(F("deg, "));
  Serial.print(humidity);
  Serial.print(F("hum, "));
  Serial.print(motionSensor);
  Serial.print(F("MO, "));
  Serial.println(lightSensor);*/
  // Wait a bit before we start over again
  debugTime("End");
  //Serial.println(".");
}

void selectDigit(byte d)
{
  digitalWrite(Digit1, d!=Digit1?HIGH:LOW);
  digitalWrite(Digit2, d!=Digit2?HIGH:LOW);
  digitalWrite(Digit3, d!=Digit3?HIGH:LOW);
  digitalWrite(Digit4, d!=Digit4?HIGH:LOW);
}

void displayData(byte d1, byte d2, byte d3, byte d4)
{
  int dly=2;
  digitalWrite(SRLatch, LOW);
  shiftOut(SRData, SRClock, MSBFIRST, d4);
  digitalWrite(SRLatch, HIGH);
  selectDigit(Digit1);
  delay(dly);
  selectDigit(0);
  digitalWrite(SRLatch, LOW);
  shiftOut(SRData, SRClock, MSBFIRST, d3);
  digitalWrite(SRLatch, HIGH);
  selectDigit(Digit2);
  delay(dly);
  selectDigit(0);
  digitalWrite(SRLatch, LOW);
  shiftOut(SRData, SRClock, MSBFIRST, d2);
  digitalWrite(SRLatch, HIGH);
  selectDigit(Digit3);
  delay(dly);
  selectDigit(0);
  digitalWrite(SRLatch, LOW);
  shiftOut(SRData, SRClock, MSBFIRST, d1);
  digitalWrite(SRLatch, HIGH);
  selectDigit(Digit4);
  delay(dly);
  selectDigit(0);
}

