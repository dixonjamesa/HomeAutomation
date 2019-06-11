// Act as simple WAP to allow user to input details
// so we can then connect to the main WiFi
// then connect MQTT nessages to the pins to control relays.

// note to upload to USB, give access with 
// sudo chmod 666 /dev/ttyUSB0

//Flash settings:
// Board - NodeMCU 1.0 ESP-12E
//

#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include <list>

const uint16_t PixelCount = 150; // make sure to set this to the number of pixels in your strip
const uint8_t PixelPin = D4;  // make sure to set this to the correct pin, ignored for Esp8266

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);
NeoPixelAnimator animations(2);

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
const char *AP_ssid = "rgb101";
const char *AP_password = "password";

int numNetworks=0;
// details for the main Wifi (entered by user)
String WIFI_ssid = "PrettyFly24";
String WIFI_pass = "Dixonia1";
String WIFI_status="Not connected";
bool WIFI_details_ready = true;
bool WIFI_connected = false;
const char* mqtt_server = "192.168.0.99";

const byte ledPin = LED_BUILTIN;

int stateLED = HIGH; // high is off (obviously!)

ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);

void debugout(const char *msg)
{
  Serial.println(msg);
}

String topics_sub[] = {
    "general/striplight/R",
    "general/striplight/G",
    "general/striplight/B",
    };
String topics_pub[] = {
    "general/striplight/R",
    "general/striplight/G",
    "general/striplight/B",
    };
String topics_vals[] = {"-","-","-"}; 
int topics_pins[] = {D7, D2, D1, D3, D6, D5, D4};

void tryConnectMQTT() 
{
  if( WIFI_connected )
  {
    Serial.print("Connection: ");
    Serial.println(client.connected());
    if(!client.connected())
    {
      Serial.print("Attempting MQTT connection...");
      // Attempt to connect
      if (client.connect("ESP8266 Client")) 
      {
        Serial.println("connected");
        // ... and subscribe to all topics
        for(int i=0; i < sizeof(topics_sub)/sizeof(topics_sub[0]) ; i++)
        {
          client.subscribe(topics_sub[i].c_str());
          Serial.print("Sub to : ");
          Serial.println(topics_sub[i]);
          client.loop(); // do this. or >5 subs causes disconnect some time later. Eugh
        }
        Serial.println(client.connected());
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
  for (i=0 ; i<sizeof(topics_sub)/sizeof(topics_sub[0]) ; i++) 
  {
    if( !strcmp(topic, topics_sub[i].c_str() ) )
    {
      payload[length]=0;
      topics_vals[i] = (char *)payload;
      Serial.print("Setting topic ");
      Serial.print(i);
      Serial.print(" to ");
      Serial.print(topics_vals[i]);
      strip.SetPixelColor(payload[0]*PixelCount, HtmlColor(0x3f3f00));
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

  
  if(stateLED == LOW){
    htmlRes += HtmlLedStateLow;
  }else{
    htmlRes += HtmlLedStateHigh;
  }

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

void SecondsTick(const AnimationParam& param)
{
  if( param.state == AnimationState_Started )
  {
  }
  else if (param.state == AnimationState_Completed)
  {
    // done, time to restart this position tracking animation/timer
      animations.RestartAnimation(param.index);
  }
  else
  {
    strip.SetPixelColor(param.progress*PixelCount, HtmlColor(0x3f0000));
  }
}

void setup() 
{
    delay(1000);
    Serial.begin(9600);
    Serial.println("Boom");

    // switches, bitches:
    for(int i=0;i<sizeof(topics_pins)/sizeof(topics_pins[0]); i++)
    {
      pinMode(topics_pins[i],OUTPUT);
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
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, stateLED);

    debugout("Ready");

    gTime = millis();

    cbData *cbd = new cbData(tryConnectMQTT, 0, true, 5000);
    callbackList.push_back(cbd);
    callbackList.push_back(new cbData(flashWifi, 0, true, 2000 ));
    callbackList.push_back(new cbData(tryConnectWIFI, 0, true, 5000));

    strip.Begin();
    animations.StartAnimation(0, 1000, SecondsTick);
    strip.SetPixelColor(10, HtmlColor(0x3f0000));
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
  }
  else
  {
    stateLED = LOW;
    digitalWrite(LED_BUILTIN, stateLED);
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

    animations.UpdateAnimations();
    strip.Show();

}
