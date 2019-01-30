#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

#include <list>

#define SERIALOUT 1
class cbData
{
  public:
  int timeout; // time before call
  void (*func)(void);  // func to call
  bool repeat;
  int time2;

  cbData(void (*_f)(void), int _t, bool _rep, int _repTime)
  {
    func = _f;
    repeat = _rep;
    timeout = _t;
    time2 = _repTime;
  }

  void step(int _t)
  {
    timeout -=_t;
    if( timeout <= 0 )
    {
      func();
      if( repeat )
      {
        timeout = time2;
      }
    } 
  }
};

cbData * callbackList[4];
//std::list<cbData *> callbackList;

unsigned int gTime;
unsigned int gTimestep;


int numNetworks=0;
// details for the main Wifi (entered by user)
String WIFI_ssid = "PrettyFly24";
String WIFI_pass = "Dixonia1";
String WIFI_status="Not connected";
bool WIFI_details_ready = true;
bool WIFI_connected = false;
const char* mqtt_server = "192.168.0.99";

int stateLED = HIGH; // high is off (obviously!)

WiFiClient espClient;
PubSubClient client(espClient);

void debugout(const char *msg)
{
  Serial.println(msg);
}

const byte SensorRollerDoor = A0;
const byte SensorBackDoor = D6;

const byte switchPin = D4; // output to RC switch
const byte outputLEDRPin = D1;
const byte outputLEDGPin = D2;
const byte outputLEDBPin = D3;

// Switch states
bool switchIsOn=false; // whether the output switch is on
int switchOnTimer = 0; // timer for how long the switch remains on
const int switchOnTime = 600; // how long to leave it on
long lockoutTimer = 0; // timer to avoid door opening as network connections start up

byte backDoorSensorState=-1; // back door state
byte rollerDoorSensorState=-1; // roller door state

// indicator globals
bool indicatorLoopUp = true; // direction we're fading in
int indicatorFadeValue = 0; // current fade value
int indicatorFadeMax = 255; // max fade value


// Time between checks (in ms)
const unsigned long loopTime = 100;

// subscribed topics, and their values.
char *topics_sub[] = {
    "garage2/switch",
    };
String topics_vals[] = {"-","-","-","-","-","-","-"}; 

// topics we publish
char *topics_pub[] = {
    "garage2/bdoor",
    "garage2/gdoor",
    "garage2/status",
    "garage2/switch",
    "garage2/IP",
    "garage2/command",
    };
enum
{
  PUB_BackDoor=0,
  PUB_RollerDoor,
  PUB_Status,
  PUB_Switch,
  PUB_IP,
  PUB_cmnd
};

void tryConnectMQTT() 
{
  if( WIFI_connected )
  {
    if(!client.connected())
    {
      Serial.print("Attempting MQTT connection...");
      // Attempt to connect
      if (client.connect("GarageClient")) 
      {
        Serial.println(F("connected"));
        lockoutTimer = 0;
        // ... and subscribe to all topics
        for(int i=0; i < sizeof(topics_sub)/sizeof(topics_sub[0]) ; i++)
        {
          client.subscribe(topics_sub[i]);
          Serial.print(F("Sub to : "));
          Serial.println(topics_sub[i]);
          client.loop(); // do this. or >5 subs causes disconnect some time later. Eugh
        }
        Serial.println(client.connected());
        client.publish(topics_pub[PUB_Status], "OK", true);
        client.publish(topics_pub[PUB_IP], WiFi.localIP().toString().c_str(), true);
        Serial.print(F("IP address: "));
        Serial.println(WiFi.localIP());
      } 
      else 
      {
        Serial.print(F("failed, rc="));
        Serial.print(client.state());
        Serial.println(F(" try again in 5 seconds"));
      }
    }
  }
}

void MQTTcallback(char* topic, byte* payload, unsigned int length)
{
  int i = 0;
  Serial.print(F("Message arrived ["));
  Serial.print(topic);
  //Serial.print(", ");
  //Serial.print((char *)payload);
  Serial.print("] ");
  for (i=0 ; i<sizeof(topics_sub)/sizeof(topics_sub[0]) ; i++) 
  {
    if( !strcmp(topic, topics_sub[i] ) )
    {
      payload[length]=0;
      topics_vals[i] = (char *)payload; // OK because using String class
      Serial.print("Setting topic ");
      Serial.print(i);
      Serial.print(" to ");
      Serial.print(topics_vals[i]);
      if( !strcmp(topics_vals[i].c_str(),"ON") )
      {
        char tmp[60];
        sprintf(tmp,"Opened %02d:%02d:%02d",gTime/(1000*60*60),(gTime/(1000*60))%60, (gTime/1000)%60);
        client.publish(topics_pub[PUB_cmnd],tmp,true);
        switchIsOn = true;
        if( lockoutTimer > (long)1000*10 )
        { // Cannot automatically open the door for the first seconds...
          digitalWrite(switchPin, true);
        }
        Serial.print("(ON)");
      }
  #if false
      else
      {
        switchIsOn = false;
        if( lockoutTimer > (long)1000*10 )
        { // Cannot automatically open the door for the first seconds...
          digitalWrite(switchPin, false);
        }
        Serial.print("(OFF)");
      }
   #endif
    }
  }
  Serial.println();
}

void tryConnectWIFI()
{
  if( WIFI_details_ready && WiFi.status() != WL_CONNECTED )
  {
    Serial.print("Connecting WIFI ");
    Serial.print(WIFI_ssid);
    Serial.print(WIFI_pass);
    WiFi.begin(WIFI_ssid.c_str(), WIFI_pass.c_str());
    
    int tries = 0;

    while( WiFi.status() != WL_CONNECTED )
    {
      stateLED = 1 - stateLED;
      digitalWrite(LED_BUILTIN, stateLED);
      delay(500);
      Serial.print(".");
      if( ++tries > 20 )
      {
        break; // give up then
      }
    }
    Serial.println("Updating status...");
    switch(WiFi.status())
    {
      case WL_CONNECTED:
        WIFI_status = "Connected to " + WIFI_ssid;
        WIFI_connected = true;
        debugout("LED On");
        client.setServer(mqtt_server, 1883);
        client.setCallback(MQTTcallback);
        stateLED = LOW;
        digitalWrite(LED_BUILTIN, stateLED);
        break;
      case WL_CONNECT_FAILED:
        WIFI_status = "Failed to connect to " + WIFI_ssid;
        break;
      //case WL_CONNECTION_LOST:
      case WL_DISCONNECTED:
        WIFI_status = "Disconnected";
        break;       
      default:
        WIFI_status = "Unknown problem";
        break;
    }
  }
}

void setup(void)
{
  // setup pins
  pinMode(outputLEDRPin, OUTPUT);
  pinMode(outputLEDGPin, OUTPUT);
  pinMode(outputLEDBPin, OUTPUT);
  pinMode(switchPin, OUTPUT);
  pinMode(SensorRollerDoor, INPUT);
  pinMode(SensorBackDoor, INPUT);
  digitalWrite(switchPin,false);

  delay(500);
#if SERIALOUT
  // Set up the Serial Monitor
  Serial.begin(9600);
  Serial.println(F("Start"));
#endif

  SetIndicator(255,255,255); // white

  // setup Wifi...
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, stateLED);

    Serial.println(F("Ready"));

    gTime = millis();

    cbData *cbd = new cbData(tryConnectMQTT, 0, true, 3000);
    callbackList[0] = cbd;
    callbackList[1] = new cbData(flashWifi, 0, true, 2000 );
    callbackList[2] = new cbData(tryConnectWIFI, 0, true, 5000);

}

void flashWifi()
{
  if( !WIFI_connected )
  {
    stateLED = LOW;
    digitalWrite(LED_BUILTIN, stateLED);
    delay(100);
    stateLED = HIGH;
    digitalWrite(LED_BUILTIN, stateLED);    
    SetIndicator(0,255,255); // cyan
    Serial.println(F("X WiFi is gawn"));
  }
  else
  {
    stateLED = HIGH;
    digitalWrite(LED_BUILTIN, stateLED);
    SetIndicator(0,255,0); // green
  }
}

void loop() 
{
  gTimestep = millis() - gTime;
  gTime = millis();

  // loop over any callbacks:
  callbackList[0]->step(gTimestep);
  callbackList[1]->step(gTimestep);
  callbackList[2]->step(gTimestep);

    //server.handleClient();
  if( client.connected() )
  {
    if( client.loop() == false )
    {
      Serial.print(client.state());    
    }
  }
  else
  {
      Serial.print(client.state());      
  }

  if( client.connected() )
  {
    byte t;
    char tst[40];
    
    //t = ProcessInputValue(analogRead(SensorBackDoor));
    t = ProcessInputValue(600); // safe closed value
    if( backDoorSensorState !=  t )
    {
      backDoorSensorState = t;
      if( backDoorSensorState == 2 )
      {
        sprintf(tst, "Tamper Back Door %d", analogRead(SensorBackDoor));
        client.publish(topics_pub[PUB_Status], tst);
      }
      sprintf(tst,"%d",t);
      client.publish(topics_pub[PUB_BackDoor], tst);
      Serial.print("Back door state now ");
      Serial.println(tst);
    }
    t = ProcessInputValue(analogRead(SensorRollerDoor));
    //t = ProcessInputValue(digitalRead(SensorRollerDoor));
    if( rollerDoorSensorState !=t  )
    {
      rollerDoorSensorState = t;
      if( rollerDoorSensorState == 2 )
      {
        sprintf(tst, "Tamper Roller Door %d", analogRead(SensorRollerDoor));
        client.publish(topics_pub[PUB_Status], tst);
      }
      else if( rollerDoorSensorState == 0 )
      {
        client.publish(topics_pub[PUB_Status], "Garage Door Open");
      }
      sprintf(tst,"%d",t);
      client.publish(topics_pub[PUB_RollerDoor], tst);
      Serial.print("Roller door state now ");
      Serial.println(tst);
    }
    if( backDoorSensorState == 2 || rollerDoorSensorState == 2 )
    { // tamper
      SetIndicator(255,0,0); // red
      UpdateIndicator(loopTime); // flash faster
    }
    else if( rollerDoorSensorState == 0 )
    {
      SetIndicator(180,255,0); // orange
    }
    else if( backDoorSensorState == 0 )
    {
      SetIndicator(0,60,255); // cyan
    }
    else
    {
      SetIndicator(0,255,0); // green      
    }

    if( switchIsOn )
    {
      switchOnTimer += loopTime;
      //Serial.println(switchOnTimer);
      indicatorFadeValue = indicatorFadeMax;
      SetIndicator(0,0,255); // blue
      if( switchOnTimer > switchOnTime )
      {
        client.publish(topics_pub[PUB_Switch],"OFF",true);
        switchOnTimer = 0;
        switchIsOn = false;
        digitalWrite(switchPin, false);
        Serial.println("Switch is OFF");
      }
    }
  }

  UpdateIndicator(gTimestep);
  
  // ensure the door doesn't get opened in the first minute:
  lockoutTimer += loopTime;
  delay(loopTime);
}

// update the indicator fading
void UpdateIndicator(int ltime)
{
  if( indicatorLoopUp )
  {
    indicatorFadeValue += ltime*500/1000;
    if( indicatorFadeValue > indicatorFadeMax )
    {
      indicatorFadeValue = indicatorFadeMax;
      indicatorLoopUp = false;
    }
  }
  else
  {
    indicatorFadeValue -= ltime*50/1000;
    if( indicatorFadeValue < 0 )
    {
      indicatorFadeValue = 0;
      indicatorLoopUp = true;
    }
  }
}

// threshold levels...
const int TamperLowThreshold = 100; // below this likely short (wires connected)
const int OpenThreshold = 900;
const int TamperHighThreshold = 1100; // above this likely o/c (wires cut)
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


