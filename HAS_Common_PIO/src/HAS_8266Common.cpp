//
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

//#include "config.h"
#include "HAS_Main.h"
#include "HAS_Pixels.h"
#include "HAS_WebServer.h"
#include "HAS_Animation.h"

#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "HAS_option.h"
#include "HAS_Comms.h"
#include <list>

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)<(b))?(b):(a))


// forward declarations:
void setupOTA();
void flashWifi();

// uptime:
unsigned int UT_days = 0;
unsigned int UT_hours = 0;
unsigned int UT_minutes = 0;
unsigned int UT_seconds = 0;

struct Rotary
{
  int count;
  bool latch;
  int pinU;
  int pinD;
};

T_Switch switches[NUM_SWITCHES];
bool outputs[NUM_OUTS];
Rotary rotaries[NUM_ROTS];
int resetTimer;
int gMode = 0;

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

bool WIFI_connected = false;
bool SAP_enabled = false;
String WIFI_status="Not connected";

WiFiClient espClient;
PubSubClient PSclient(espClient);
Option options;

void StartSoftAP()
{
  if( !SAP_enabled)
  {
    WiFi.mode(WIFI_AP_STA);
    SAP_enabled = WiFi.softAP(options.WebNameParsed(), options.AP_pass());
    if( SAP_enabled )
    {
      IPAddress apip = WiFi.softAPIP();
      Serial.print("visit: ");
      Serial.print(apip);
      Serial.print(" on ");
      Serial.print(options.WebNameParsed());
      Serial.print(" (pass ");
      Serial.print(options.AP_pass());
      Serial.println(") ");
    }
    else
    {
      Serial.println("Failed to start SoftAP");
      ESP.restart();
    }
  }
}
void StopSoftAP()
{
  if(SAP_enabled)
  {
    WiFi.softAPdisconnect(true);
    SAP_enabled = false;
    Serial.println("Stopped SoftAP");
  }
}

void PublishLWT()
{
  char topic[64];
  sprintf(topic, "%s/%s/LWT", options.MqttTopic(), options.Prefix3());
  PSclient.publish(topic, "Online", true); // LWT message
}

void PublishStatus()
{
  char topic[64];
  char tbuf[128];
  int r,g,b;

  GetRGB(r,g,b);

  //Serial.println("Publishing status");
  sprintf( topic, "%s/%s/STATE", options.MqttTopic(), options.Prefix3());
  sprintf( messageBuffer, "{\"Uptime\":\"%d:%d:%d:%d\",", UT_days, UT_hours, UT_minutes, UT_seconds);
  if( options.OutType(1) != OUTTYPE_NONE ) { sprintf(tbuf, "\"POWER1\":\"%s\",", outputs[0]?"ON":"OFF"); strcat(messageBuffer, tbuf); if( options.OutType(1) == OUTTYPE_RGB ) { sprintf(tbuf, "\"RGB\":\"%d,%d,%d\",", r, g, b); strcat(messageBuffer, tbuf);sprintf(tbuf, "\"BRIGHT\":\"%d\",", GetBrightness()); strcat(messageBuffer, tbuf);sprintf(tbuf, "\"LENGTH\":\"%d/%d\",", GetStripLength(), options.RGBCount()); strcat(messageBuffer, tbuf); }}
  if( options.OutType(2) != OUTTYPE_NONE ) { sprintf(tbuf, "\"POWER2\":\"%s\",", outputs[1]?"ON":"OFF"); strcat(messageBuffer, tbuf); }
  if( options.OutType(3) != OUTTYPE_NONE ) { sprintf(tbuf, "\"POWER3\":\"%s\",", outputs[2]?"ON":"OFF"); strcat(messageBuffer, tbuf); }
  if( options.OutType(4) != OUTTYPE_NONE ) { sprintf(tbuf, "\"POWER4\":\"%s\",", outputs[3]?"ON":"OFF"); strcat(messageBuffer, tbuf); }
  if( options.OutType(5) != OUTTYPE_NONE ) { sprintf(tbuf, "\"POWER5\":\"%s\",", outputs[4]?"ON":"OFF"); strcat(messageBuffer, tbuf); }
  if( options.OutType(6) != OUTTYPE_NONE ) { sprintf(tbuf, "\"POWER6\":\"%s\",", outputs[5]?"ON":"OFF"); strcat(messageBuffer, tbuf); }
  if( options.OutType(7) != OUTTYPE_NONE ) { sprintf(tbuf, "\"POWER7\":\"%s\",", outputs[6]?"ON":"OFF"); strcat(messageBuffer, tbuf); }
  if( options.OutType(8) != OUTTYPE_NONE ) { sprintf(tbuf, "\"POWER8\":\"%s\",", outputs[7]?"ON":"OFF"); strcat(messageBuffer, tbuf); }

sprintf(tbuf, "\"Wifi\":{\"AP\":%d,\"SSId\":\"%s\",\"IP\":\"%s\",\"RSSI\":%d,\"APMac\":\"%s\"},\"Mqtt\":%d}",
                          WiFi.status(), WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), -WiFi.RSSI(), WiFi.BSSIDstr().c_str(), PSclient.state());
  strcat(messageBuffer, tbuf);
  Serial.print(topic);
  Serial.print(" ");
  Serial.print(messageBuffer);
  if( !PSclient.publish(topic, messageBuffer, false)) // STATE message
  {
    // publish failed
    Serial.println("FAILED");
    // disconnect and try again (the MQtt update callback)...
    PSclient.disconnect();
  }
  else
  {
    Serial.println("... OK");
  }
}

void PublishInitInfo()
{
  // telemetry:
  PublishLWT();
  PublishStatus();

  // HA auto-discovery:
#if HOMEASSISTANT_DISCOVERY
  setupAutoDisco();
#endif
}

void ShowPSState()
{
  //if(!PSclient.connected())
  {
    Serial.print("MQTT state: ");
    Serial.print(PSclient.state());
    Serial.print(" WIFI status=");
    Serial.println(WiFi.status());
  }
}

/*
 * Regular callback to (re)connect MQTT
 */
void tryConnectMQTT()
{
  if( WIFI_connected )
  {
    ShowPSState();
    if(!PSclient.connected())
    {
      const char *clname = options.MqttClientParsed();
      Serial.print("Attempting MQTT connection as '");
      Serial.print(clname);
      Serial.println("'...");
      // Attempt to connect
      bool Cstatus;
      PSclient.setServer(options.MqttHost(), options.MqttPort());
      PSclient.setCallback(MQTTcallback);
      // construct the full LWT topic:
      String LWTTop = String(options.MqttTopic()) + "/" + options.Prefix3() + "/LWT";
      if( *options.MqttUser() == 0 )
      {
        Cstatus = PSclient.connect(clname, LWTTop.c_str(), 1, true, "Offline");
      }
      else
      {
        Cstatus = PSclient.connect(clname, options.MqttUser(), options.MqttPass(), LWTTop.c_str(), 1, true, "Offline" );
      }
      if( Cstatus )
      {
        Serial.print("connected...");
        Serial.println(PSclient.connected());
        // ... and subscribe to all topics
        setupMQTTMessages();
        PublishInitInfo();
      }
      else
      {
        Serial.print("failed, rc=");
        Serial.print(PSclient.state());
        Serial.print(" WIFI status=");
        Serial.print(WiFi.status());
        // states can be found here: https://pubsubclient.knolleary.net/api.html#state
        Serial.println(" try again in 10 seconds");
        PSclient.disconnect();
      }
    }
  }
}

/*
 * Disconnect the wifi, and update flag
 */
void disconnectWIFI()
{
  WIFI_connected = false;
  WiFi.disconnect();
}

int WIFI_fail = 0;

void testWIFI()
{
  WIFI_fail += 2; // called every 2 seconds
  switch(WiFi.status())
  {
    case WL_CONNECTED:
    {
      if( !WIFI_connected )
      {
        WIFI_status = String("Connected to ") + options.WF_ssid() + " with IP " + WiFi.localIP().toString();
        Serial.println(WIFI_status);
        WIFI_connected = true;
        WIFI_fail = 0;
        if( !MDNS.begin(options.Unit()) ){Serial.println("mDNS start error");}
        // OTA setup:
        setupOTA();
        StopSoftAP(); // ensure softAP not running
      }
      else
      {
        WIFI_fail = 0;
      }
      break;
    }
    case WL_NO_SSID_AVAIL:
      WIFI_status = String("Cannot reach ") + options.WF_ssid();
      Serial.println(WIFI_status);
      WIFI_connected = false;
      break;
    case WL_CONNECT_FAILED:
      WIFI_status = String("Invalid password for ") + options.WF_ssid();
      Serial.println(WIFI_status);
      WIFI_connected = false;
      break;
    case WL_CONNECTION_LOST:
      WIFI_status = "Connection lost";
      Serial.println(WIFI_status);
      WIFI_connected = false;
      break;
    case WL_DISCONNECTED:
      WIFI_status = "Disconnected - not in station mode";
      Serial.println(WIFI_status);
      WIFI_connected = false;
      break;
    default:
      WIFI_status = "Unknown problem";
      Serial.println(WIFI_status);
      WIFI_connected = false;
      break;
  }
  flashWifi();
  if( !WIFI_connected)
  {
    Serial.println("");
    Serial.print("WiFi Status: ");
    Serial.println(WiFi.status());
    WiFi.printDiag(Serial);
    if( WIFI_fail > 30 )
    {
      // wifi not able to connect for > 30 seconds
      // switch over to softAP
      StartSoftAP();
    }
  }
}

// called to try to connect to wifi
// skips if already connected
// might be worth looking at https://github.com/espressif/arduino-esp32/issues/512
void tryConnectWIFI()
{
  const char *ssid = options.WF_ssid();
  const char *pass = options.WF_pass();
  if( *ssid != 0 && !WIFI_connected )
  {
    Serial.print("Connecting to WIFI ");
    Serial.print(ssid);
    Serial.print(", pass ");
    Serial.println(pass);
    Serial.print("Hostname: ");
    Serial.println(options.Unit());
    if( !WiFi.hostname(options.Unit())) Serial.println("Could not set hostname");
    WiFi.begin(ssid, pass);

    Serial.println("Updating WIFI status...");
  }
  else
  {
    if( *ssid==0 )
    {
      WIFI_status = "Not Set";
      Serial.println(" WIFI details not set");
    }
  }
}

// kick off OTA listener
void setupOTA()
{
  ArduinoOTA.setHostname(options.WebNameParsed());
  // ArduinoOTA.setPassword("admin");

  ArduinoOTA.onStart([]()
  {
    String type;
    // stop the wifi server...
    StopSoftAP();
    Serial.end();
    if (ArduinoOTA.getCommand() == U_FLASH)
    {
      type = "sketch";
    }
    else
    { // U_SPIFFS
      type = "filesystem";
    }
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]()
  {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
  {
    int prg = (progress / (total / 100));
    digitalWrite(options.StatusLED(), prg%2);

    Serial.printf("Progress: %u%%\r", prg);
  });
  ArduinoOTA.onError([](ota_error_t error)
  {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  Serial.println("OTA started: ");
  ArduinoOTA.begin();
}

// standard sketch setup() function
void setup()
{
  int p;

  delay(1000);
  Serial.begin(115200);
  Serial.println("Boom");
  resetTimer = 0;

  options.Begin();

  //Serial.setDebugOutput(true);

  for(int i=0 ; i < NUM_SWITCHES ; i++)
  {
    switches[i].Setup(i+1, options.SwPin1(i+1), options.SwPin2(i+1), options.SwType(i+1), options.SwTopic(i+1));
  }

  // Setup the rotary info
  for(int i=0 ; i < NUM_ROTS ; i++)
  {
    rotaries[i].pinU = options.RotPinU(i+1);
    rotaries[i].pinD = options.RotPinD(i+1);
  }

  // rotary pins type
  for(int i=0 ; i < NUM_ROTS ; i++)
  {
    p = options.RotPinU(i+1); if( p != -1 ) { pinMode(p, INPUT_PULLUP); Serial.print("Pullup rot pinU: ");Serial.println(p); }
    p = options.RotPinD(i+1); if( p != -1 ) { pinMode(p, INPUT_PULLUP); Serial.print("Pullup rot pinD: ");Serial.println(p); }
  }
  //output pins type
  for(int i=0; i< NUM_OUTS; i++)
  {
    p = options.OutPin(i+1); if( p != -1 ) { pinMode(p, OUTPUT);digitalWrite(p, !outputs[i]); }
    p = options.OutLED(i+1); if( p != -1 ) pinMode(p, OUTPUT);
  }
  if( options.StatusLED() != -1 ) pinMode(options.StatusLED(), OUTPUT);
  if( options.ResetPin() != -1 ) pinMode(options.ResetPin(), INPUT_PULLUP);

  // init done
  String u;
  u = options.Unit();
  Serial.print("Start unit name: ");
  Serial.println(u);

  InitPixels();
  SetStripLength(options.RGBCount());
  Serial.print("RGB Count is ");
  Serial.println(GetStripLength());

  SetBrightness(128);
  // web server running, This will be accessible over softAP (x.x.4.1) and WiFi
  InitWebServer();
  Serial.println("HTTP server started");

  gTime = millis();

  callbackList.push_back(new cbData(tryConnectMQTT, 0, true, 10000));
  callbackList.push_back(new cbData(testWIFI, 0, true, 2000 ));

  // auto-reconnect is handled by the WiFi class...
  //callbackList.push_back(new cbData(tryConnectWIFI, 0, true, 30000));
  WiFi.mode(WIFI_STA); // - this stops the softAP working initially
  tryConnectWIFI();
  options.Save();
  Serial.println("Ready");
}

void flashWifi()
{
  int state = digitalRead(options.StatusLED());
  int flash = 0;
  if( WiFi.status() != WL_CONNECTED )
  {
    if( WIFI_fail < 30 )
    {
      flash = 1;
    }
    else
    {
      flash = 3;
    }
  }
  else if( !PSclient.connected() )
  {
    flash = 2;
  }
  if( flash > 0 )
  {
    // start off
    digitalWrite(options.StatusLED(), LOW);
    delay(80);

    for(int i = 0 ; i < flash ; i++)
    {
      digitalWrite(options.StatusLED(), HIGH);
      delay(80);
      digitalWrite(options.StatusLED(), LOW);
      delay(140);
    }
    digitalWrite(options.StatusLED(), state);
  }
}


bool GetOutput(int _id)
{
  if( _id > 0 && _id <= NUM_OUTS )
  {
    return outputs[_id-1];
  }
  return -1;
}

/*
 * Sets output pin state and LED, and
 * publishes <unit>/stat/POWER<n> to MQTT if changed
 */
void SetOutput( int _id, bool _state, bool _toggle )
{
  if( _id > 0 && _id <= NUM_OUTS )
  {
    int pin = -1;
    int led = -1;
    if( _toggle )
    {
      outputs[_id-1] = !outputs[_id-1];
      _state = outputs[_id-1];
    }
    else
    {
      outputs[_id-1] = _state;
    }
    pin = options.OutPin(_id);
    led = options.OutLED(_id);

    // set the LED state:
    if( led != -1 )
    {
      Serial.print("Setting LED ");
      Serial.print(led);
      Serial.print(" to ");
      Serial.println(options.InvertLED()?!_state:_state);
      digitalWrite(led, options.InvertLED()?!_state:_state);
    }

    // set the output state, and publish to MQTT if changed:
    if( pin != -1 )
    {
      Serial.print("Setting Output ");
      Serial.print(_id);
      Serial.print(" to ");
      Serial.println(_state);
      if( options.OutType(_id) == OUTTYPE_ONOFF )
      {
        digitalWrite(pin, !_state); // direct hardware connection for relability. Note set low to activate
        Serial.print("Setting Pin ");
        Serial.print(pin);
        Serial.print(" to ");
        Serial.println(!_state);
      }
      else
      {
        if( _id==1 && _state )
        {
          int r,g,b;
          GetRGB(r,g,b);
          // if it's off due to brightness or RGB values, then set to mid values:
          if( GetBrightness() == 0 ) SetBrightness(128);
          if( r == 0 && g == 0 && b == 0 )
          {
            SetRGB(128, 128, 128);
          }
        }
        else
        {
          // rgb values are updated in teh main loop()
        }
      }
      // publish state message
      String message = String(options.MqttTopic()) + "/" + options.Prefix2() + "/POWER" + _id;
      //Serial.println(message);
      PSclient.publish(message.c_str(), _state?"ON":"OFF", true); // retained power state
      PublishStatus(); // also do this for the HomeAssistant update
    }
  }
}

int GetMode() { return gMode; }
int ChangeMode(int _m)
{
  // -1, just increment
  if( _m == -1 ) gMode = (gMode+1)%MODE_MAX;
  // otherwise set to the specific mode
  else { gMode = _m%MODE_MAX; }
}

void ChangeValue(int _amount)
{
  switch(gMode)
  {
    case MODE_BRIGHTNESS:
      SetBrightness(max(0, min(255, GetBrightness() + _amount*10)));
      break;
    case MODE_HUE:
    {
      ChangeHue(_amount*5);
    }
      break;
    case MODE_SATURATION:
    {
      ChangeSaturation(_amount*5);
    }
      break;
    case MODE_LENGTH:
      SetStripLength(GetStripLength() + _amount);
      break;
  }
}


void T_Switch::Setup(int _id, byte _p1,byte _p2,byte _t,const char *_top)
{
  id=_id;pin1=_p1;pin2=_p2;type=_t;topic=_top;

  if( pin1 != 255 ) {pinMode(pin1, INPUT_PULLUP);Serial.print("Pullup input pin: ");Serial.println(pin1);}
  if( pin2 != 255 ) {pinMode(pin2, INPUT_PULLUP);Serial.print("Pullup input pin: ");Serial.println(pin2);}
}

String T_Switch::CmndString()
{
  return String(options.MqttTopic()) + "/" + options.Prefix1() + "/switch" + id;
}

void T_Switch::Set( const char *_val, bool _out, bool _tog )
{
  // first publish switch command message:
  String t = CmndString();
  PSclient.publish(t.c_str(), _val, false);

  // and additional topic if set:
  if( *topic != 0 )
  {
    PSclient.publish(topic, _val, false);
  }
  Serial.print(t);
  Serial.print(": ");
  Serial.println(_val);

  // and now make the change to the hardware:
  SetOutput(id, _out, _tog);
}

void T_Switch::Toggle()
{
  Serial.println("Toggle switch");
  Set("TOGGLE", false, true);
}

void T_Switch::On()
{
  Serial.println("On switch");
  Set("ON", true, false);
}
void T_Switch::Off()
{
  Serial.println("Off switch");
  Set("OFF", false, false);
}

/*
 * Update a switch
 * work out which output it relates to (id)
 */
void T_Switch::Update( )
{
  if( pin1 == -1 ) return;
  if( type == SWTYPE_PRESS || type == SWTYPE_RELEASE || type == SWTYPE_PUSHBUTTON)
  { // pushbutton or on/off
    if( !digitalRead(pin1) )
    {
      if( !latch )
      {
        latch = true;
        if( type == SWTYPE_PRESS )
        {
          Toggle();
        }
        if( type == SWTYPE_PUSHBUTTON )
        {
          On();
        }
      }
    }
    else
    {
      if( latch )
      {
        latch = false;
        if( type == SWTYPE_RELEASE )
        {
          Toggle();
        }
        if( type == SWTYPE_PUSHBUTTON )
        {
          Off();
        }
      }
    }
  }
  else if(type == SWTYPE_DUAL)
  {
    if( !digitalRead(pin1) )
    {
      if( !latch )
      {
        latch = true;
        On();
      }
    }
    else
    {
      if( latch )
      {
        latch = false;
      }
    }
    if( !digitalRead(pin2) )
    {
      if( !latch2 )
      {
        latch2 = true;
        Off();
      }
    }
    else
    {
      if( latch2 )
      {
        latch2 = false;
      }
    }
  }
}

int regularUpdate = 0;
/*
 * called once a minute
 */
void loopMinutes()
{
  // sort out uptime:
  if( ++UT_minutes >= 60 )
  {
    UT_minutes = 0;
    if( ++UT_hours >= 24 )
    {
      UT_hours = 0;
      UT_days++;
    }
  }

  if( ++regularUpdate >= 5 )
  {
    regularUpdate = 0;
    PublishLWT();
    PublishStatus();
  }
}

/*
 * Called once a second
 */
void loopSeconds()
{
  if( ++UT_seconds >= 60 )
  {
    UT_seconds = 0;
    loopMinutes();
  }
}

void loop()
{
  //Serial.print("Loop...");
  if( (gTime/1000) < (((int)millis())/1000) )
  { // crossed a whole second bounday
    loopSeconds();
  }
  gTimestep = millis()-gTime;
  gTime = millis();

  for(std::list<cbData *>::const_iterator iterator = callbackList.begin() ; iterator != callbackList.end() ; ++iterator )
  {
    (*iterator)->step(gTimestep);
  }
  UpdateWebServer();
  if( PSclient.connected() )
  {
    if( PSclient.loop() == false )
    {
      Serial.print("MQTT Client error: ");
      Serial.println(PSclient.state());
    }
  }
  else
  {
      //Serial.print("MQTT Client not connected: ");
      //Serial.println(client.state());
  }

  //Serial.print("A.");
  ArduinoOTA.handle();

  // Read the rotary switches (high rate)
  bool rDir;
  for(int i=0;i<NUM_ROTS;i++)
  {
    rotaries[i].count = 0;
    rotaries[i].latch = false;
  }
  //Serial.print("B");
  for(int j = 0 ; j<25 ; j++)
  {
    for(int i=0;i<NUM_ROTS;i++)
    {
      if( digitalRead(rotaries[i].pinU) == false )
        {
        if( !rotaries[i].latch )
        {
          rotaries[i].latch = true;
        }
      }
      else if(rotaries[i].latch)
      {
        rDir = !digitalRead(rotaries[i].pinD);
        if( rDir )
        {
          rotaries[i].count++;
        }
        else
        {
          rotaries[i].count--;
        }
        rotaries[i].latch = false;
      }
    }
    //Serial.print(".");

    UpdateAnimation(outputs[0], options.AnimSpeed());
    //Serial.print("_");
    UpdatePixels();
    delay(1);
  }
  //Serial.print("C.");

  char data[10];
  // have the rotaries moved?
  for(int i=0;i<NUM_ROTS;i++)
  {
    if( 0 != rotaries[i].count )
    {
      char topic[128];
      //Serial.println(rotaries[i].count);
      sprintf(data, "%d", rotaries[i].count);
      sprintf(topic, "%s/%s/ROTARY%d", options.MqttTopic(), options.Prefix1(), (i+1));
      PSclient.publish(topic, data, false);
      if( *options.RotTopic(i+1) != 0 )
      {
        // also publish additional topic:
        PSclient.publish(options.RotTopic(i+1), data, false);
      }
    }
  }
  //Serial.print("D.");
  // what about switches...
  for(int i=0;i<NUM_SWITCHES;i++)
  {
    switches[i].Update();
  }
  //Serial.print("E.");

  if( options.ResetPin() != -1 )
  {
    bool pressed = !digitalRead(options.ResetPin());
    if( pressed )
    {
      resetTimer += gTimestep;
      if( resetTimer > 5000 )
      {
        ESP.restart();
      }
    }
    else
    {
      resetTimer = 0;
    }
  }
  //Serial.println("X.");
}
