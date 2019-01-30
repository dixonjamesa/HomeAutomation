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
#include "HAS_WebServer.h"

#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "HAS_option.h"
#include "HAS_Comms.h"
#include <list>

#define min(a,b) ((a<b)?a:b)
#define max(a,b) ((a<b)?b:a)

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

T_Switch switches[4];
bool outputs[NUM_OUTS];
Rotary rotaries[4];



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
    WiFi.softAP(options.WebNameParsed(), options.AP_pass().c_str());
    SAP_enabled = true;
    IPAddress apip = WiFi.softAPIP();
    Serial.print("visit: ");
    Serial.print(apip);
    Serial.print(" on ");
    Serial.print(options.WebNameParsed());
    Serial.print(" (pass ");
    Serial.print(options.AP_pass().c_str());
    Serial.println(") ");      
  }
}
void StopSoftAP()
{
  if(SAP_enabled)
  {
    WiFi.softAPdisconnect(true);
    SAP_enabled = false;
  }
}

void PublishLWT()
{
  char topic[64];
  sprintf(topic, "%s/%s/LWT", options.MqttTopic().c_str(), options.Prefix3().c_str());
  PSclient.publish(topic, "Online", true); // LWT message
}
void PublishStatus()
{
  char topic[64];
  char tbuf[128];
  sprintf( topic, "%s/%s/STATE", options.MqttTopic().c_str(), options.Prefix3().c_str());
  sprintf( messageBuffer, "{\"Uptime\":\"%d:%d:%d:%d\",", UT_days, UT_hours, UT_minutes, UT_seconds);
  if( options.OutPin(1) != -1 ) { sprintf(tbuf, "\"POWER1\":\"%s\",", outputs[0]?"ON":"OFF"); strcat(messageBuffer, tbuf); }
  if( options.OutPin(2) != -1 ) { sprintf(tbuf, "\"POWER2\":\"%s\",", outputs[1]?"ON":"OFF"); strcat(messageBuffer, tbuf); }
  if( options.OutPin(3) != -1 ) { sprintf(tbuf, "\"POWER3\":\"%s\",", outputs[2]?"ON":"OFF"); strcat(messageBuffer, tbuf); }
  if( options.OutPin(4) != -1 ) { sprintf(tbuf, "\"POWER4\":\"%s\",", outputs[3]?"ON":"OFF"); strcat(messageBuffer, tbuf); }
  if( options.OutPin(5) != -1 ) { sprintf(tbuf, "\"POWER5\":\"%s\",", outputs[4]?"ON":"OFF"); strcat(messageBuffer, tbuf); }
  if( options.OutPin(6) != -1 ) { sprintf(tbuf, "\"POWER6\":\"%s\",", outputs[5]?"ON":"OFF"); strcat(messageBuffer, tbuf); }
  if( options.OutPin(7) != -1 ) { sprintf(tbuf, "\"POWER7\":\"%s\",", outputs[6]?"ON":"OFF"); strcat(messageBuffer, tbuf); }
  if( options.OutPin(8) != -1 ) { sprintf(tbuf, "\"POWER8\":\"%s\",", outputs[7]?"ON":"OFF"); strcat(messageBuffer, tbuf); }
  sprintf(tbuf, "\"Wifi\":{\"AP\":%d,\"SSId\":\"%s\",\"IP\":\"%s\",\"RSSI\":%d,\"APMac\":\"%s\"},\"Mqtt\":%d}", 
                          WiFi.status(), WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), -WiFi.RSSI(), WiFi.BSSIDstr().c_str(), PSclient.state());
  strcat(messageBuffer, tbuf);
  Serial.print(topic);
  Serial.print(" ");
  Serial.println(messageBuffer);
  if( !PSclient.publish(topic, messageBuffer, false)) // STATE message  
  {
    // publish failed
    Serial.println("FAILED");
    // disconnect and try again (the MQtt update callback)...
    PSclient.disconnect();
  }
}

void PublishInitInfo()
{
  // telemetry:
  PublishLWT();
  PublishStatus();
  PSclient.loop();

  // HA auto-discovery:
#if HOMEASSISTANT_DISCOVERY
  {
    for (int i=1; i<=NUM_OUTS; i++)
    {
      String UID = options.UID() + "_LI_" + i;
      String disco = "homeassistant/light/" + UID + "/config";
      if( options.OutPin(i) != -1 )
      {
        String data = "{\"device_class\":\"light\",\"name\":\"";
        data += options.FriendlyNameParsed(i);
        data += "\"";
        data += ",\"cmd_t\":\"~/cmnd/POWER" + String(i) + "\"";
        data += ",\"stat_t\":\"~/tele/STATE\"";
        data += ",\"val_tpl\":\"{{value_json.POWER" + String(1) + "}}\"";
        //data += ",\"stat_t\":\"~/stat/POWER"+ String(i) + "\"";
        data += ",\"pl_off\":\"OFF\",\"pl_on\":\"ON\",\"avty_t\":\"~/tele/LWT\",\"pl_avail\":\"Online\",\"pl_not_avail\":\"Offline\"";
        //data += ",\"uniq_id":\"";
        //data += UID + "\",\"device\":{\"identifiers\":[\"??????\"],\"name\":\"Bedroom Changing\",\"model\":\"ESP8266Generic\",\"sw_version\":\"VERSION\",\"manufacturer\":\"Dixon\"}";
        data += ",\"~\":\""+options.MqttTopic()+"\"";
        data += "}";
        PSclient.publish(disco.c_str(), data.c_str(), true);
      }
      else
      {
        // send empty payload to remove any previously retained message
        PSclient.publish(disco.c_str(), "", true);
      }
      PSclient.loop();
    }
  }
#endif
}

void tryConnectMQTT() 
{
  if( WiFi.status() == WL_CONNECTED )
  //if( WIFI_connected )
  {
    if(!PSclient.connected())
    {
      const char *clname = options.MqttClientParsed();
      Serial.print("Attempting MQTT connection as '");
      Serial.print(clname);
      Serial.println("'...");
      // Attempt to connect
      bool Cstatus;
      // construct the full LWT topic:
      String LWTTop = options.MqttTopic() + "/" + options.Prefix3() + "/LWT";
      if( options.MqttUser() == "")
      {
        Cstatus = PSclient.connect(clname, LWTTop.c_str(), 1, true, "Offline");    
      }
      else
      {
        Cstatus = PSclient.connect(clname, options.MqttUser().c_str(), options.MqttPass().c_str(), LWTTop.c_str(), 1, true, "Offline" );
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
        Serial.println(" try again in 5 seconds");
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

// called to try to connect to wifi
// skips if already connected
void tryConnectWIFI()
{
  String ssid = options.WF_ssid();
  String pass = options.WF_pass();
  if( ssid.length() != 0 && !WIFI_connected /*WiFi.status() != WL_CONNECTED*/ )
  {
    Serial.print("Connecting to WIFI ");
    Serial.print(ssid);
    Serial.print(", pass ");
    Serial.println(pass);
    Serial.print("Hostname: ");
    Serial.println(options.Unit());
    //WiFi.mode(WIFI_STA); - this stops the softAP working
    if( !WiFi.hostname(options.Unit().c_str())) Serial.println("Could not set hostname");
    WiFi.begin(ssid.c_str(), pass.c_str());
    if( !MDNS.begin(options.Unit().c_str()) ){Serial.println("mDNS start error");}
    int tries = 0;

    while( WiFi.status() != WL_CONNECTED )
    {
      digitalWrite(options.StatusLED(), tries%2);
      delay(1000);
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
      {
        WIFI_status = "Connected to " + options.WF_ssid() + " with IP " + WiFi.localIP().toString();
        Serial.println(WIFI_status);
        Serial.print("Connecting to MQTT server '");
        Serial.print(options.MqttHost().c_str());
        Serial.println("'...");
        PSclient.setServer(options.MqttHost().c_str(), options.MqttPort());
        PSclient.setCallback(MQTTcallback);
        WIFI_connected = true;
        // OTA setup:
        setupOTA();
        break;
      }
      case WL_CONNECT_FAILED:
        WIFI_status = "Failed to connect to " + options.WF_ssid();
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
  else
  {
    if( ssid.length()==0 )
    {
      WIFI_status = "Not Ready";
      Serial.println(" WIFI details not ready");
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

  options.Begin();

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

  // switch pins type
  for(int i=0 ; i<NUM_SWITCHES ; i++)
  {
    if( switches[i].pin1 != -1 ) pinMode(switches[i].pin1, INPUT_PULLUP);
    if( switches[i].pin2 != -1 ) pinMode(switches[i].pin2, INPUT_PULLUP);
  }
  // rotary pins type
  for(int i=0 ; i < NUM_ROTS ; i++)
  {    
    p = options.RotPinU(i+1); if( p != -1 ) pinMode(p, INPUT_PULLUP);
    p = options.RotPinD(i+1); if( p != -1 ) pinMode(p, INPUT_PULLUP);
  }
  //output pins type
  for(int i=0; i< NUM_OUTS; i++)
  {
    p = options.OutPin(i+1); if( p != -1 ) { pinMode(p, OUTPUT);digitalWrite(p, !outputs[i]); }
    p = options.OutLED(i+1); if( p != -1 ) pinMode(p, OUTPUT);
  }
  if( options.StatusLED() != -1 ) pinMode(options.StatusLED(), OUTPUT);
  
  // init done
  String u;
  u = options.Unit();
  Serial.print("Start unit name: ");
  Serial.println(u);

#if 0
  if( options.APServer() )
  {
    StartSoftAP();
  }
#endif

  // web server running, This will be accessible over softAP (x.x.4.1) and WiFi
  InitWebServer();
  Serial.println("HTTP server started");
  
  gTime = millis();

  callbackList.push_back(new cbData(tryConnectMQTT, 0, true, 5000));
  callbackList.push_back(new cbData(flashWifi, 0, true, 1000 ));
  callbackList.push_back(new cbData(tryConnectWIFI, 0, true, 11000));

  Serial.println("Ready");
}

void flashWifi()
{
  int state = digitalRead(options.StatusLED());
  int flash = 0;
  if( WiFi.status() != WL_CONNECTED )
  {
    flash = 2;
  }
  if( !PSclient.connected() )
  {
    flash = 3; 
  }

  digitalWrite(options.StatusLED(), HIGH);    
  for(int i = 0 ; i < flash ; i++)
  {
    digitalWrite(options.StatusLED(), LOW);
    delay(100);
    digitalWrite(options.StatusLED(), HIGH);    
  }
    digitalWrite(options.StatusLED(), state);    
}

bool GetOutput(int _id)
{
  return outputs[_id-1];
}

/*
 * Sets output pin state and LED, and
 * publishes <unit>/stat/POWER<n> to MQTT if changed
 */
void SetOutput( int _id, bool _state, bool _toggle )
{
  int pin = -1;
  int led = -1;
  if( _id <= 4 )
  {
    if( _toggle ) 
    {
      outputs[_id-1] = !outputs[_id-1];
      _state = outputs[_id-1];
    }
    else
    {
      outputs[_id-1] = _state;      
    }
  }
  pin = options.OutPin(_id);
  led = options.OutLED(_id);

  // set the LED state:
  if( led != -1 )
  {
    digitalWrite(led, options.InvertLED()?_state:!_state);
  }

  // set the output state, and publish to MQTT if changed:
  if( pin != -1 )
  {
    Serial.print("Setting Output ");
    Serial.print(_id);
    Serial.print(" to ");
    Serial.println(_state);
    digitalWrite(pin, !_state); // direct hardware connection for relability. Note set low to activate

    // publish state message
    String message = options.MqttTopic() + "/" + options.Prefix2() + "/POWER" + _id;
    //Serial.println(message);
    PSclient.publish(message.c_str(), _state?"ON":"OFF", false);
    PublishStatus(); // also do this for the HomeAssistant update
  }
}

String T_Switch::CmndString()
{
  return options.MqttTopic() + "/" + options.Prefix1() + "/switch" + id;
}

void T_Switch::Set( const char *_val, bool _out, bool _tog )
{
  // first publish switch command message:
  String t = CmndString();
  PSclient.publish(t.c_str(), _val, false);
  
  // and additional topic if set:
  if( topic != "" )
  {
    PSclient.publish(topic.c_str(), _val, false);        
  }
  Serial.print(t);
  Serial.print(": ");
  Serial.println(_val);

  // and now make the change to the hardware:
  SetOutput(id, _out, _tog);  
}

void T_Switch::Toggle()
{
  Set("TOGGLE", false, true);
}

void T_Switch::On()
{
  Set("ON", true, false);
}
void T_Switch::Off()
{
  Set("OFF", false, false);
}

/*
 * Update a switch
 * work out which output it relates to (id)
 */
void T_Switch::Update( )
{
  if( type == 1 || type == 2 || type == 3)
  { // pushbutton or on/off
    if( !digitalRead(pin1) )
    {
      if( !latch )
      {
        latch = true;
        if( type == 1 )
        {
          Toggle();
        }
        if( type == 3 )
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
        if( type == 2 )
        {
          Toggle();
        }
        if( type == 3 )
        {
          Off();
        }
      }
    }
  }
  else if(type == 4)
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

  PublishLWT();
  PublishStatus();
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
  if( WiFi.status() == WL_CONNECTED )
  {
    StopSoftAP();
  }
  else
  {
    StartSoftAP();
  }
}

void loop() 
{
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

  ArduinoOTA.handle();

  // Read the rotary switches (high rate)
  bool rDir;
  for(int i=0;i<4;i++)
  {
    rotaries[i].count = 0;
    rotaries[i].latch = false;
  }
  
  for(int j = 0 ; j<25 ; j++)
  {
    for(int i=0;i<4;i++)
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
      delay(1);
    }
  }
  
  char data[10];
  // have the rotaries moved?
  // what about switches...
  for(int i=0;i<4;i++)
  {
    switches[i].Update();
    if( 0 != rotaries[i].count )
    {
      //Serial.println(rotaries[i].count);
      sprintf(data, "%d", rotaries[i].count);
      PSclient.publish((options.MqttTopic()+"/"+options.Prefix1()+"/"+"ROTARY"+(i+1)).c_str(), data, false);
    }
  }
}
