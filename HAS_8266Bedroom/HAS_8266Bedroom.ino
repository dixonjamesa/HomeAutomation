// Act as simple WAP to allow user to input details
// so we can then connect to the main WiFi
// then connect MQTT nessages to the pins to control relays.

// note to upload to USB, give access with
// sudo chmod 666 /dev/ttyUSB0

//Flash settings:
//

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
#include <list>


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
const char *AP_ssid = "kitchen101";
const char *AP_password = "password";

int numNetworks=0;
// details for the main Wifi (entered by user)
String WIFI_ssid = "PrettyFly24";
String WIFI_pass = "Dixonia1";
String WIFI_status="Not connected";
bool WIFI_details_ready = true;
bool WIFI_connected = false;
const char* mqtt_server = "192.168.0.99";


const byte statusLEDPin = D6;
int stateLED = HIGH; // high is off (obviously!)

ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);

void debugout(const char *msg)
{
  Serial.println(msg);
}

// topics we subscribe to and their values
char *topics_sub[] = {
    "bedroom/stat/POWER",
    "bedroom2/stat/POWER1",
    "bedroom2/stat/POWER2",
    //"kitchen/lights/l1",
    //"kitchen/lights/l2",
    //"kitchen/lights/l3",
    "bedswitch/topics",
    "bedswitch/topic1",
    "bedswitch/topic2",
    "bedswitch/topic3",
    };
enum
{
  SUB_T1V=0,
  SUB_T2V,
  SUB_T3V,
  SUB_Topics,
  SUB_T1,
  SUB_T2,
  SUB_T3
};
int topics_vals[] = {0,0,0};

char *topics_pub[] = {
    "bedroom/cmnd/POWER",
    "bedroom2/cmnd/POWER1",
    "bedroom2/cmnd/POWER2",
    "bedswitch/status",
    "bedswitch/IP",
    "bedswitch/stat/topics",
    };
enum
{
  PUB_T1,
  PUB_T2,
  PUB_T3,
  PUB_Status=0,
  PUB_IP,
  PUB_Topics,
};

int topics_pinsOn[] = {D6, D2, D4};
int topics_pinsOff[] = {D1, D3, D5};
int topics_LEDs[] = {D7, D8, 3};

void tryConnectMQTT()
{
  if( WIFI_connected )
  {
    if(!client.connected())
    {
      Serial.print("Attempting MQTT connection...");
      // Attempt to connect
      if (client.connect("BedSwitchClient"))
      {
        Serial.println("connected");
        // ... and subscribe to all topics
        for(int i=0; i < sizeof(topics_sub)/sizeof(topics_sub[0]) ; i++)
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

void MQTTcallback(char* topic, byte* payload, unsigned int length)
{
  int i = 0;
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  // look for the actual topic values, and set LEDs accordingly
  for (i=SUB_T1V ; i<=SUB_T3V ; i++)
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
        digitalWrite(topics_LEDs[i], false);
        Serial.print("(OFF)");
      }
      else
      {
        topics_vals[i] = 1;
        digitalWrite(topics_LEDs[i], true);
        Serial.print("(ON)");
      }
    }
  }
  // list topics?
  if( !strcmp(topic, topics_sub[SUB_Topics]))
  {
    char tps[256];
    Serial.print("Received topic request: ");
    Serial.println(topic);
    // return a list of the topics
    sprintf(tps, "{Topic1: %s, Topic2: %s, Topic3: %s}", topics_sub[SUB_T1V], topics_sub[SUB_T2V], topics_sub[SUB_T3V]);
    client.publish(topics_pub[PUB_Topics], tps);
  }
  // change a topic?
  for (i=SUB_T1 ; i<=SUB_T3 ; i++)
  {
    if( !strcmp(topic, topics_sub[i] ) )
    {
      int j = i - SUB_T1 + SUB_T1V;
      payload[length]=0;
      strncpy(topics_sub[j], (char *)payload, length);
      topics_sub[j][length]=0; // terminate string
      Serial.print("Changed topic to ");
      Serial.println((char *)payload);
    }
  }

  Serial.println();
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
  tryConnectWIFI();
  response();
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
        WIFI_status = "Connected to " + WIFI_ssid;
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

  /*
  if(stateLED == LOW){
    htmlRes += HtmlLedStateLow;
  }else{
    htmlRes += HtmlLedStateHigh;
  }*/

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
    Serial.begin(9600);
    Serial.println("Boom");

    // switches, bitches:
    for(int i=0;i<sizeof(topics_pinsOn)/sizeof(topics_pinsOn[0]); i++)
    {
      pinMode(topics_pinsOn[i],INPUT_PULLUP);
      pinMode(topics_pinsOff[i],INPUT_PULLUP);
      pinMode(topics_LEDs[i],OUTPUT);
    }

    handleScan();
    // init done


    WiFi.softAP(AP_ssid, AP_password);

    IPAddress apip = WiFi.softAPIP();
    Serial.print("visit: \n");
    Serial.println(apip);
    server.on("/", handleRoot);
    server.on("/Scan", handleScan);
    server.on("/Logon", handleLogon);
    server.begin();
    Serial.println("HTTP server started");
    if(statusLEDPin != -1 ) pinMode(statusLEDPin, OUTPUT);
    //digitalWrite(statusLEDPin, stateLED);

    debugout("Ready");

    gTime = millis();

    cbData *cbd = new cbData(tryConnectMQTT, 0, true, 5000);
    callbackList.push_back(cbd);
    callbackList.push_back(new cbData(flashWifi, 0, true, 2000 ));
    callbackList.push_back(new cbData(tryConnectWIFI, 0, true, 11000));
}

void flashWifi()
{
  if( !WIFI_connected )
  {
    //stateLED = LOW;
    //digitalWrite(statusLEDPin, stateLED);
    delay(100);
    //stateLED = HIGH;
    //digitalWrite(statusLEDPin, stateLED);
  }
  else
  {
    //stateLED = LOW;
    //digitalWrite(statusLEDPin, stateLED);
  }
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

  for(int i=0;i<sizeof(topics_pinsOn)/sizeof(topics_pinsOn[0]); i++)
  {
    bool valOff = digitalRead(topics_pinsOff[i]);
    bool valOn = digitalRead(topics_pinsOn[i]);
    int newval = topics_vals[i];
    if(!valOff) newval = 0;
    if(!valOn) newval = 1;
    /*
    Serial.print("Topic ");
    Serial.print(i);
    Serial.print(" - val ");
    Serial.print(topics_vals[i]);
    Serial.print(" - off ");
    Serial.print(valOff);
    Serial.print(" - on ");
    Serial.print(valOn);
    Serial.println(".");
    */
    if( newval != topics_vals[i] )
    {
      // change the value by publishing the MQTT message, which we'll pick up on receipt to set the LEDs
      if( newval == 0 )
      {
        Serial.print(F("Turn off "));
        Serial.println(topics_pub[i]);
        client.publish(topics_pub[i], "OFF", true);
      }
      else
      {
        Serial.print(F("Turn on "));
        Serial.println(topics_pub[i]);
        client.publish(topics_pub[i], "ON", true);
      }
    }
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

  delay(200);
}
