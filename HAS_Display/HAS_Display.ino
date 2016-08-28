#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <HACommon.h>
#include <HAHelper.h>
#include <Adafruit_GFX.h> // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>

bool allInitialised = false;

#define MINPRESSURE 100
#define MAXPRESSURE 1000

#define BUTTONSIZE 50
// Assign human-readable names to some common 16-bit color values:
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

// PIN Configuration:
#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET A4
#define SD_CS 10

#define YP A2 // must be an analog pin, use "An" notation!
#define XM A3 // must be an analog pin, use "An" notation!
#define YM 8 // can be a digital pin
#define XP 9 // can be a digital pin

#define TS_MINX 188
#define TS_MINY 186
#define TS_MAXX 958
#define TS_MAXY 913

const byte RFCE = 2; // transmitter
const byte RFCSN = 3; // transmitter

// Constants that identify this node and the node to send data to
const uint16_t this_node = 011;
const uint16_t control_node = 0;

// Switch states
bool latchState=false;
bool toggleState=false;
bool resendsetup=false;
bool dummy=false;
unsigned long touchTime;

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 900);
Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

// Radio with CE & CSN pins
RF24 radio(RFCE, RFCSN);
RF24Network RFNetwork(radio);

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
        SubscribeChannel( DT_BOOL, 2, message->data );
        RegisterChannel( DT_BOOL, 102, &dummy, 1, message->data, false ); // so we can send messages on this code
        allInitialised = true;
        Serial.println("Monitor setup done.");
        strcpy(StatusMessage, "OK: ");
        strcat(StatusMessage, message->data);
      break;
      case 202: // reset
        // all we need to do is re-register the channels...
        Serial.println("Reset");
        InitialiseMessaging(true);
      break;
      case 2:
        toggleState = (bool)message->data[0];
        Serial.print("Data received from node ");
        Serial.print(from_node);
        // Check value and change the pin...
        Serial.print(" {Code: ");
        Serial.print(message->code);
        Serial.print(", Value: ");
        Serial.print(toggleState);
        Serial.println("}");
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
    sprintf(channel, "n%o/monitor", this_node);
    SubscribeChannel( DT_TEXT, 201, channel); // we will be told what channel to do switching for
    sprintf(channel, "n%o/reset", this_node);
    SubscribeChannel( DT_TEXT, 202, channel); // we will be told to reset here
    sprintf(channel, "n%o/sendsetup", this_node);
    RegisterChannel( DT_BYTE, 204, &resendsetup, 0, channel, _restart); // use this to prompt that we still aren't set up yet
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
  //radio.begin();
  delay(5);
  //RFNetwork.begin(120, this_node);
  //radio.setRetries(8,11);
  //RFNetwork.txTimeout = 129;
  
  //HANetwork.Begin(this_node);

  tft.reset();

  uint16_t identifier = tft.readID();
  identifier = 0x9341;
  
  Serial.print(F("LCD driver chip: "));
  Serial.println(identifier, HEX);
  
  tft.begin(identifier);
  
  tft.fillScreen(BLACK);
  
  tft.setRotation(1); // landscape 320*240 , mode 2 = 240*320
  
  tft.fillRect(0, 0, BUTTONSIZE, BUTTONSIZE, RED);
  tft.fillRect(BUTTONSIZE, 0, BUTTONSIZE, BUTTONSIZE, YELLOW);
  tft.fillRect(BUTTONSIZE*2, 0, BUTTONSIZE, BUTTONSIZE, GREEN);
  tft.fillRect(BUTTONSIZE*3, 0, BUTTONSIZE, BUTTONSIZE, CYAN);
  tft.fillRect(BUTTONSIZE*4, 0, BUTTONSIZE, BUTTONSIZE, BLUE);
  
  tft.drawRect(0, 0, BUTTONSIZE, BUTTONSIZE, WHITE);

  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  delay(50);

  HANetwork.InitialiseMessaging();
  Serial.println("Setup complete.");
}

void loop() 
{
  //Latch(total);
  Serial.print("LoopStart");

  // Update network data
  //RFNetwork.update();
  //HANetwork.Update(20);
  /*
  if( !allInitialised )
  {
    toggleState = !toggleState;
    if( HANetwork.QueueEmpty() )
    {
      resendsetup = !resendsetup; // trigger a message
    }
    delay(400);
  }*/


  TSPoint p = ts.getPoint();
Serial.print("X = "); Serial.print(p.x);
Serial.print("\tY = "); Serial.print(p.y);
Serial.print("\tZ = "); Serial.print(p.z);
Serial.print("\n");
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) 
  {
    tft.fillCircle(p.x, p.y, (MAXPRESSURE-p.z)/25, RED);
  }
  Serial.print("Loop");
}

long ThresholdHigh = 200;
long ThresholdLow = 120;
void Latch(long value)
{
  if(latchState == false && value > ThresholdHigh)
  { // Hand has approached switch
    latchState = true;
    touchTime = 0;
  }
  else if( latchState == true && value < ThresholdLow )
  { // Hand has receded from switch
    latchState = false;
    toggleState = !toggleState;
    dummy = toggleState;
    HANetwork.ForceSendRegisteredChannel(&dummy);
    touchTime = 0;
  }
  else if( latchState == true )
  { // Hand is hovering at switch
    touchTime += 20;
    if( touchTime > 1000 )
    {
      if( toggleState == false )
      {
        toggleState = !toggleState;
        dummy = toggleState;
        HANetwork.ForceSendRegisteredChannel(&dummy);
      }
    }
  }
}

