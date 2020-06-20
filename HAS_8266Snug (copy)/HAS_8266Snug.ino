// Act as simple WAP to allow user to input details
// so we can then connect to the main WiFi
// then connect MQTT nessages to the pins to control relays.

// note to upload to USB, may need to give access with 
// sudo chmod 666 /dev/ttyUSB0

//Flash settings:
// Use Node-MCU 1.0

// Snug - rotary encoder for kitchen lights
// Actual snug light is on a separate snoff unit

// Hardware:
// 1st switch with LED - Switch1 & LED controlled by us
// 2nd switch - switch to sonoff
// 3rd switch - Switch2
// switch-and-rotary Switch3, Rot1 & Rot2
// Relay - snug light, switch 1 enabled

#include <config.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <list>

#define min(a,b) ((a<b)?a:b)
#define max(a,b) ((a<b)?b:a)

const byte pinRot1 = D2; // rotary1 (connect third terminal to gnd)
const byte pinRot2 = D1; // rotary2
const byte pinSwitch1 = D5;
const byte pinSwitch2 = D4;
const byte pinSwitch3 = D3;
const byte pinLED = D6; // output indicator
const byte pinPower = D7; // relay

const byte statusLEDPin = D6;
int stateLED = HIGH; // high is off (obviously!)

bool switch1Latch = false; // latch for the main switch
bool switch2Latch = false; // latch for the main switch
bool switch3Latch = false; // latch for the main switch

bool snugLight = false;

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

std::list<cbData *> callbackList;

unsigned int gTime;
unsigned int gTimestep;


// details for our AP
const char *AP_ssid = AP_SSID;
const char *AP_password = AP_PASS;
bool APServer = AP_SERVER; // see config.h

int numNetworks=0;
// details for the main Wifi (entered by user)
String WIFI_ssid = STA_SSID;
String WIFI_pass = STA_PASS;
String WIFI_status="Not connected";
bool WIFI_details_ready = WIFI_AUTOCONNECT;
bool WIFI_connected = false;
const char* mqtt_server = MQTT_HOST;


ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);

void debugout(const char *msg)
{
  Serial.println(msg);
}

// topics we subscribe to and their values
char *topics_sub[] = {
    "kitchen/lights/l2", // mood
    "kitchen/lights/l1", //main
    "kitchen/lights/l3", // bar
    "dining/cmnd/POWER", // dining
    "kitchen/lights/l7", // sink
    "kitchen/lights/l5", // pillar
    "kitchen/lights/l4",
    "snug/cmnd/POWER", // snug sonoff switch
    "snug/light", //snug light
    };
enum
{
  SUB_Mood=0,
  SUB_Main,
  SUB_Bar,
  SUB_Dining,
  SUB_Sink,
  SUB_Pillar,
  SUB_T7,
  SUB_SnugSw,
  SUB_SnugLight,
  SUB_Max
};
int topics_vals[] = {0,0,0,0,0,0,0,0,0,0}; 

char *topics_pub[] = {
    "snug/rotary",
    "snug/switch1", 
    "cmnd/snug/POWER", // snug sonoff switch
    "snug/switch3", 
    "snug/switch4", 
    "snug/light",
    "snug/status",
    "snug/IP",
    };

   
enum
{
  PUB_Rotary=0,
  PUB_Switch1,
  PUB_Switch2,
  PUB_Switch3,
  PUB_Switch4,
  PUB_Light,
  PUB_Status,
  PUB_IP,
};


void tryConnectMQTT() 
{
  if( WIFI_connected )
  {
    if(!client.connected())
    {
      Serial.print("Attempting MQTT connection...");
      // Attempt to connect
      if (client.connect("SnugClient")) 
      {
        Serial.println("connected");
        // ... and subscribe to all topics
        for(int i=0; i < SUB_Max ; i++)
        {
          client.subscribe(topics_sub[i]);
          Serial.print("Sub to : ");
          Serial.println(topics_sub[i]);
          client.loop(); // do this. or >5 subs causes disconnect some time later. Eugh
        }
        Serial.println(client.connected());
        client.publish(topics_pub[PUB_Status], "OK", true);
        client.publish(topics_pub[PUB_IP], WiFi.localIP().toString().c_str(), true);
      } 
      else 
      {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
      }
    }
  }
}

// callback when an MQTT message is received
// for all topics we've subscribed to
void MQTTcallback(char* topic, byte* payload, unsigned int length)
{
  int i = 0;
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  // look for the actual topic values, and keep up to date
  for (i=0 ; i<SUB_Max ; i++) 
  {
    if( !strcmp(topic, topics_sub[i] ) )
    {
      payload[length]=0;
      Serial.print("Setting topic ");
      Serial.print(i);
      Serial.print(" to ");
      Serial.print((char *)payload);
      if( !strcmp((char *)payload,"OFF") )
      {
        topics_vals[i] = 0;
        Serial.print("(OFF)");
      }
      else
      {
        topics_vals[i] = 1;
        Serial.print("(ON)");
      }
    }
  }
  Serial.println();
  snugLight = topics_vals[SUB_SnugLight];
}


void handleRoot() 
{
    response();
}

// Scan all available networks
void handleScan()
{
  // scan for all networks, including hidden (2nd param true)
  WiFi.scanNetworks(false, true);
  numNetworks = WiFi.scanComplete();
  response();
}

const String HtmlHtml = "<html><head>"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" /></head>";
const String HtmlHtmlClose = "</html>";
const String HtmlTitle = "<h1>Miniserve Acess Point</h1><br/>\n";
const String HtmlLedStateLow = "<big>LED is now <b>ON</b></big><br/>\n";
const String HtmlLedStateHigh = "<big>LED is now <b>OFF</b></big><br/>\n";
const String HtmlInput = "Password:<br><input type=\"text\" name = \"pass\"></input><input type=\"submit\">";
const String HtmlTail = "<a href=\"Scan\"><button style=\"display: block; width: 100%;\">Scan</button></a><br/>";

void handleLogon()
{
  int ssidID = -1;
  int passID = -1;
  
  Serial.print("Attempting WIFI connection..");
  debugout("WIFI connecting...");

  for(int i=0 ; i < server.args() ; i++)
  {
    if( server.argName(i) == "ssid" )
    {
      ssidID = i;
    }
    if( server.argName(i) == "pass" )
    {
      passID = i;
    }    
  }

  if( passID != -1 && ssidID != -1 )
  {
    WIFI_ssid = server.arg(ssidID);
    WIFI_pass = server.arg(passID);
    WIFI_details_ready = true;
  }
  if( WiFi.status() == WL_CONNECTED )
  {
    // disconnect previous wifi...
    WiFi.disconnect();
  }
  tryConnectWIFI();
  response();
}

// called to try to connect to wifi
// skips if already connected
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
      digitalWrite(statusLEDPin, stateLED);
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
        WIFI_status = "Connected to " + WIFI_ssid + " with IP " + WiFi.localIP().toString();
        WIFI_connected = true;
        client.setServer(mqtt_server, 1883);
        client.setCallback(MQTTcallback);
        //stateLED = LOW;
        //digitalWrite(statusLEDPin, stateLED);
        break;
      case WL_CONNECT_FAILED:
        WIFI_status = "Failed to connect to " + WIFI_ssid;
        break;
      case WL_CONNECTION_LOST:
      case WL_DISCONNECTED:
        WIFI_status = "Disconnected";
        break;       
      default:
        WIFI_status = "Unknown problem";
        break;
    }
  }
}

///
// HTML response
///
void response()
{
  String htmlRes = HtmlHtml + HtmlTitle;

  Serial.println("Serving content");

  htmlRes += HtmlTail;
  htmlRes += "<p>"+WIFI_status+"</p>";

  htmlRes += "<form action=\"Logon\">";

  if( numNetworks > 0 )
  {
    for(int i=0;i<numNetworks ; i++)
    {
      htmlRes += "<input type=\"radio\" name=\"ssid\" value=\"" + WiFi.SSID(i) + "\"";
      if( i==0) htmlRes += " checked ";
      htmlRes += ">" + WiFi.SSID(i) + "<br>";
    }
    htmlRes += HtmlInput;
  }
  else
  {
    htmlRes += "<p>No networks available</p>";
  }

  htmlRes += "</form>";
  
  htmlRes += HtmlHtmlClose;


  server.send(200, "text/html", htmlRes);
}

void setup() 
{
    delay(1000);
    //Serial.begin(9600);
    Serial.println("Boom");

    pinMode(pinRot1, INPUT_PULLUP);
    pinMode(pinRot2, INPUT_PULLUP);
    pinMode(pinSwitch1, INPUT_PULLUP);
    pinMode(pinSwitch2, INPUT_PULLUP);
    pinMode(pinSwitch3, INPUT_PULLUP);
    pinMode(pinLED, OUTPUT);
    pinMode(pinPower, OUTPUT);
    digitalWrite(pinPower, true);
    handleScan();
    // init done
        
    if( APServer )
    {
      WiFi.softAP(AP_ssid, AP_password);

      IPAddress apip = WiFi.softAPIP();
      Serial.print("visit: \n");
      Serial.println(apip);
      server.on("/", handleRoot);
      server.on("/Scan", handleScan);
      server.on("/Logon", handleLogon);
      server.begin();
      Serial.println("HTTP server started");
    }
    
    pinMode(statusLEDPin, OUTPUT);

    debugout("Ready");

    gTime = millis();

    cbData *cbd = new cbData(tryConnectMQTT, 0, true, 5000);
    callbackList.push_back(cbd);
    callbackList.push_back(new cbData(flashWifi, 0, true, 1000 ));
    callbackList.push_back(new cbData(tryConnectWIFI, 0, true, 11000));
}

void flashWifi()
{
  if( !WIFI_connected )
  {
    stateLED = LOW;
    digitalWrite(statusLEDPin, stateLED);
    delay(100);
    stateLED = HIGH;
    digitalWrite(statusLEDPin, stateLED);    
  }
}

bool HandleSwitch(int _pin, bool *_latch, int _topic)
{
  if( !digitalRead(_pin) )
  {
    if( !*_latch )
    {
      *_latch = true;
      client.publish(topics_pub[_topic], "ON", true);
      Serial.println("Topic ON");
      return true;
    }
  }
  else
  {
    if( *_latch )
    {
      *_latch = false;
      client.publish(topics_pub[_topic], "OFF", true);
      Serial.println("Topic OFF");
    }
  }
  return false;
}

void loop() 
{
  gTimestep = millis() - gTime;
  gTime = millis();

  // loop over any callbacks:
  for(std::list<cbData *>::const_iterator iterator = callbackList.begin() ; iterator != callbackList.end() ; ++iterator )
  {
    (*iterator)->step(gTimestep);
  }

  server.handleClient();
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

  // Read the rotary switch (high rate)
  int rCount = 0;
  bool rotaryLatch = false;
  bool rDir;
  
  for(int i = 0;i<50;i++)
  {
    if( digitalRead(pinRot1) == false )
    {
      if( !rotaryLatch )
      {
        rotaryLatch = true;
      }
    }
    else if(rotaryLatch)
    {
      rDir = !digitalRead(pinRot2);
      if( rDir )
      {
        rCount++;
      }
      else
      {
        rCount--;
      }
      rotaryLatch = false;
    }
    delay(1);
  }

  // has the rotary moved?
  if(  0 != rCount )
  {
    Serial.println(rCount);
    char data[10];
    sprintf(data, "%d", rCount);
    client.publish(topics_pub[PUB_Rotary], data, true);
  }

  // what about switches...
  bool switch1 = HandleSwitch(pinSwitch1, &switch1Latch, PUB_Switch1);
  HandleSwitch(pinSwitch2, &switch2Latch, PUB_Switch3);
  HandleSwitch(pinSwitch3, &switch3Latch, PUB_Switch4);
  if( switch1 )
  {
    snugLight = !snugLight;
    char data[10];
    sprintf(data, snugLight?"ON":"OFF");
    client.publish(topics_pub[PUB_Light], data, true);
    Serial.print("Snug light: ");
    Serial.println(snugLight);
  }

  if( topics_vals[SUB_SnugLight] == 1 || snugLight)
  {
    digitalWrite(pinPower, false);  
  }
  else
  {
    digitalWrite(pinPower, true);
  }
  if(/*topics_vals[SUB_Mood] == 0 && topics_vals[SUB_Main] == 0 && */!snugLight)
  {
        digitalWrite(pinLED, true); // light up so can be seen in dark    
  }
  else
  {
        digitalWrite(pinLED, false); // no need to be on    
  }
}

