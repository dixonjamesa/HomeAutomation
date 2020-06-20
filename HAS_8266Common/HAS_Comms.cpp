/*
 * HAS_Comms.cpp
 *
 * (C) 2019 James Dixon
 *
 * MQTT Pub/Sub functionality
 * Handles all the MQTT comms to the network
 *
 */
#include <Arduino.h>
#include "HAS_Comms.h"
#include "HAS_option.h"
#include "HAS_Update.h"
#include "HAS_Main.h"
#include "HAS_Animation.h"
#include <ESP8266WiFi.h>


char messageBuffer[MBUFSIZE];

void subscribeTo(const char *_topic)
{
  PSclient.subscribe(_topic);
  Serial.print("Sub to : ");
  Serial.println(_topic);
  PSclient.loop(); // do this. or too many items causes disconnect some time later. Eugh
}

/*
 * MQTT interface
 */
void setupMQTTMessages()
{
  char tb[128];
  sprintf(tb, "%s/#", options.MqttTopic());
  subscribeTo( tb );
  sprintf(tb, "%s/#", options.MqttGroupTopic());
  subscribeTo( tb );
  for( int i=1 ; i <= NUM_OUTS ; i++ )
  {
    if( *options.LEDTopic(i) != 0 )
    {
      subscribeTo(  options.LEDTopic(i) );
    }
  }
}

// Do requisite action for state message payload
void HandleONOFFTOGGLE( char* payload, int i /* 1-n */ )
{
  Serial.print("HandleONOFF: ");
  Serial.println(payload);
  if( !strncmp(payload, "TOGGLE", 6) ) {
    switches[i-1].Toggle();
  }
  else if( !strncmp(payload, "ON", 2) ) {
    switches[i-1].On();
  }
  else if( !strncmp(payload, "OFF", 3) ) {
    switches[i-1].Off();
  }
}

// callback when an MQTT message is received
// for all topics we've subscribed to
void MQTTcallback(char* _topic, byte* _payload, unsigned int _length)
{
  int i = 0;
  bool match = false;
  // take a copy of the topic (don't want to be fiddling inside PubSubClient's buffer really)
  char topic[127];
  strcpy(topic, _topic);
  char  *messtype = topic;
  char *originaltopic = _topic;
  // get a copy of the payloa and zero-terminate
  char payload[256];
  strncpy(payload, (char *)_payload, min((int)_length, 255));
  payload[min((int)_length, 255)] = 0;

  Serial.print("MQTT in: '");
  Serial.print(messtype);
  Serial.print(" ");
  Serial.print(payload);
  Serial.println("'");

  // uppercase so it's all case insensitive
  for (i = 0; i < strlen(messtype); i++)
  {
      messtype[i] = toupper(messtype[i]);
  }
#if 0
  // 0.5.2 onwards - keep case of payload for things like topics
  for (i = 0; i < _length; i++)
  {
      payload[i] = toupper(payload[i]);
  }
#endif

  // first check for a match against any of the LEDtopics...
  // in this instance, just update the LED to match the topic status, and return out
  for( int i=1 ; i <= NUM_OUTS ; i++ )
  {
    if( *options.LEDTopic(i) != 0 )
    {
      char fnp[64];
      strcpy(fnp, options.LEDTopic(i));
      // uppercase so it's all case insensitive
      for (int j = 0; j < strlen(fnp); j++)
      {
          fnp[j] = toupper(fnp[j]);
      }
      // does it match?
      if( !strcmp(fnp, messtype) )
      {
        match = true; // flag that we've matched
        Serial.print("Matched ");
        Serial.print(fnp);
        Serial.print(" for LEDTopic ");
        Serial.print(i);
        Serial.print(", payload: ");
        Serial.println(payload);
        if( _length > 0 )
        {
          if( *options.LEDOn(i)!= 0 && !strcmp(payload, options.LEDOn(i)) )
          {
            SetLED(i, true);
          }
          else if( *options.LEDFlash(i)!= 0 && !strcmp(payload, options.LEDFlash(i)) )
          {
            SetLED(i, true, true);
          }
          else
          {
            SetLED(i, false);
          }
        }
        // nothing more to do
        return;
      }
    }
  }

  char contents[128];
  if( !match )
  {
    // so now find first '/'
    while( *messtype != 0 && *messtype != '/' ) {messtype++;originaltopic++;}
    if( *messtype == 0 ) return;
    messtype++;originaltopic++;

    // only interested in commands, so
    // see if it's a "cmnd" message:
    if( strncmp (messtype, "CMND", 4 ) )
    {
      Serial.println("Not a CMND, so no action taken.");
      return;
    }
    // advance to the good bit
    messtype += 5; originaltopic += 5;

    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.println();

  }
  // if we've not matched a command yet, keep trying:
  if( !match )
  {
    if( !strncmp(messtype, "STATE", 5) && strlen(messtype) == 5 ||
        !strncmp(messtype, "STATUS", 6) && strlen(messtype) == 6) // STATE / STATUS
    {
      PublishStatus();
      return;
    }
    else
    if( !strncmp(messtype, "SWITCH", 6) && strlen(messtype) == 7 ) // SWITCHn
    {
      //is a switch command
      Serial.print("Switch: ");
      Serial.print(messtype);
      Serial.println(" (no action taken).");
      // Don't do anything with this - the switch is ours!!
      // So it makes no sense for another device to try to change the actual switch states
      // Plus this would double trigger the switch, by responding to our own cmnd/switch message
      // so, just carry on...
      return;
    }
    else
    if( !strncmp(messtype, "POWER", 5) && strlen(messtype) == 6 ) // POWERn
    {
      //is a power command
        Serial.print("Power: ");
        Serial.print(messtype);
        int id = atoi(messtype+5);
        Serial.print(" to ");
        Serial.println(payload);
        if( _length > 0 )
        {
          HandleONOFFTOGGLE( payload, id );
        }
        sprintf(contents, "{\"%s\":\"%s\"}", messtype, GetOutput(id)?"ON":"OFF");
    }
    else
    if( !strncmp(messtype, "ROTARY", 6) && strlen(messtype) == 7) // ROTARYn
    {
      // rotary command
      Serial.print("Rotary: ");
      Serial.print(messtype);
      Serial.println(" (no action taken).");
      // Don't do anything with this - the rotary is ours!!
      // just carry on...
      return;
    }
    else
    if( !strncmp(messtype, "UPDATE", 6) && strlen(messtype) == 6) // UPDATE
    {
      // rotary command
      Serial.print("Update: ");
      if( payload != "" && payload != NULL )
      {
          UpdateURL = payload;
      }
      else
      {
        UpdateURL = options.UpdateServer();
      }
      Serial.println(UpdateURL);
      UpdateNow = true;
      return;
    }
    else
    if( !strncmp(messtype, "UPDATESERVER", 12) && strlen(messtype) == 12) // UPDATESERVER
    {
      // rotary command
      Serial.print("UpdateServer:  ");
      Serial.println(payload);
      options.UpdateServer( payload);
      return;
    }

    else if( !strncmp(messtype, "MODE", 4) && strlen(messtype) == 4) // MODE
    {
      // mode command
      Serial.print("Mode: ");
      Serial.println(payload);
      if( _length > 0 )
      {
        ChangeMode(atoi(payload));
      }
      sprintf(contents, "{\"MODE\":\"%d\"}", GetMode());
    }
    else if( !strncmp(messtype, "CHANGEMODE", 10) && strlen(messtype) == 10) // CHANGEMODE
    {
      char mstr[15];
      // mode command
      Serial.print("Mode: ");
      Serial.println(payload);
      ChangeMode();
      switch(GetMode())
      {
        case MODE_BRIGHTNESS:
          sprintf(mstr, "Brightness" );
          break;
        case MODE_HUE:
          sprintf(mstr, "Hue" );
          break;
        case MODE_SATURATION:
          sprintf(mstr, "Saturation" );
          break;
        case MODE_LENGTH:
          sprintf(mstr, "Length" );
          break;
        default:
          sprintf(mstr, "Undefined" );
          break;
      }
      sprintf(contents, "{\"MODE\":\"%d (%s)\"}", GetMode(), mstr);
    }
    else if( !strncmp(messtype, "CHANGE", 6) && strlen(messtype) == 6) // CHANGE
    {
      // change command
      Serial.print("Change: ");
      Serial.println(payload);
      if( _length > 0 )
      {
        int r,g,b;
        ChangeValue(atoi(payload));
        // TODO: Return the value of the thing wot changed
        switch( GetMode( ) )
        {
          case MODE_BRIGHTNESS:
            sprintf(contents, "{\"BRIGHT\":\"%d\"}", GetBrightness());
            break;
          case MODE_HUE:
            GetRGB(r,g,b);
            sprintf(contents, "{\"RGB\":\"%d,%d,%d\"}",r,g,b);
            break;
          case MODE_SATURATION:
            GetRGB(r,g,b);
            sprintf(contents, "{\"RGB\":\"%d,%d,%d\"}",r,g,b);
            break;
          case MODE_LENGTH:
            sprintf(contents, "{\"STRIPLEN\":\"%d\"}", GetStripLength());
            break;
          default:
            sprintf(contents, "{\"???\":\"???\"}" );
            break;
        }
      }
    }
    else if( !strncmp(messtype, "REBOOT", 6) && strlen(messtype) == 6)
    {
      // REBOOT command
      Serial.print("Reboot");
      options.Save();
      ESP.restart();
    }
    else if( !strncmp(messtype, "VERSION", 7) && strlen(messtype) == 7)
    {
      // VERSION command
      Serial.print("Version: ");
      Serial.println(payload);
      sprintf(contents, "{\"VERSION\":\"%s\"}", options.Version());
      char message[128];
      sprintf(message, "%s/%s/VERSION", options.MqttTopic(), options.Prefix3());
      //Serial.print("Going to publish ");
      //Serial.println(message);
      //Serial.println(contents);
      PSclient.publish(message, options.Version(), false);
      return;
    }
    else if( !strncmp(messtype, "IP", 2) && strlen(messtype) == 2)
    {
      // IP command
      Serial.print("IP: ");
      Serial.println(payload);
      sprintf(contents, "{\"IP\":\"%s\}}", WiFi.localIP().toString().c_str());
      char message[128];
      sprintf(message, "%s/%s/IP", options.MqttTopic(), options.Prefix3());
      //Serial.print("Going to publish ");
      //Serial.println(message);
      //Serial.println(contents);
      PSclient.publish(message, WiFi.localIP().toString().c_str(), false);
      return;
    }
    else if( !strncmp(messtype, "HUE", 3) && strlen(messtype) == 3) // HUE
    {
      // hue command
      Serial.print("Hue: ");
      Serial.println(payload);
      if( _length > 0 )
      {
        SetHue(atoi(payload));
      }
      sprintf(contents, "{\"HUE\":\"%d\"}", GetHue());
    }
    else if( !strncmp(messtype, "HUEDEL", 6) && strlen(messtype) == 6) // HUEDEL
    {
      // huedel command to change delta of hue
      Serial.println("Hue: ");
      if( _length > 0 )
      {
        SetHue(GetHue() + atoi(payload));
      }
      sprintf(contents, "{\"HUE\":\"%d\"}", GetHue());
    }
    else if( !strncmp(messtype, "BRIGHT", 6) && strlen(messtype) == 6) // BRIGHT
    {
      // bright command
      Serial.print("Bright: ");
      Serial.println(payload);
      if( _length > 0 )
      {
        SetBrightness(atoi(payload));
      }
      sprintf(contents, "{\"BRIGHT\":\"%d\"}", GetBrightness());
    }
    else if( !strncmp(messtype, "BRIGHTDEL", 9) && strlen(messtype) == 9) // BRIGHTDEL
    {
      // brightdel command to change delta of brightness
      Serial.println("Bright: ");
      if( _length > 0 )
      {
        SetBrightness(GetBrightness() + atoi(payload));
      }
      sprintf(contents, "{\"BRIGHT\":\"%d\"}", GetBrightness());
    }
    else if( !strncmp(messtype, "SATURATION", 10) && strlen(messtype) == 10) // SATURATION
    {
      // saturation command
      Serial.println("Saturation: ");
      if( _length > 0 )
      {
        SetSaturation(atoi(payload));
      }
      sprintf(contents, "{\"SATURATION\":\"%d\"}", GetSaturation());
    }
    else if( !strncmp(messtype, "SATDEL", 6) && strlen(messtype) == 6) // SATDEL
    {
      // satdel command to change delta of saturation
      Serial.println("Saturation: ");
      if( _length > 0 )
      {
        SetSaturation(GetSaturation() + atoi(payload));
      }
      sprintf(contents, "{\"SATURATION\":\"%d\"}", GetSaturation());
    }
    else if( !strncmp(messtype, "STRIPLEN", 8) && strlen(messtype) == 8) // STRIPLEN
    {
      // striplen command
      Serial.println("Striplen: ");
      if( _length != 0 )
      {
        SetStripLength(atoi(payload));
      }
      sprintf(contents, "{\"STRIPLEN\":\"%d\"}", GetStripLength());
    }
    else if( !strncmp(messtype, "STRIPLENDEL", 11) && strlen(messtype) == 11) // STRIPLENDEL
    {
      // striplendel command to change delta on length...
      Serial.println("Striplen: ");
      if( _length != 0 )
      {
        SetStripLength(GetStripLength() + atoi(payload));
      }
      sprintf(contents, "{\"STRIPLEN\":\"%d\"}", GetStripLength());
    }
    #if 0 // now an option in itself
    else if( !strncmp(messtype, "ANIMATION", 9) && strlen(messtype) == 9) // ANIMATION
    {
      if( _length != 0 )
      {
        SetAnimation(atoi(payload));
      }
      sprintf(contents, "{\"ANIMATION\":\"%d\"}", GetAnimation());
    }
    #endif
    else if( !strncmp(messtype, "RGB", 3) && strlen(messtype) == 3) // RGB
    {
        int r,g,b,ptr1 = 0, ptr2 = 0;
        char *rgbdata = payload;
      //is a RGB command
        Serial.println("RGB: ");
        if( _length > 0 )
        {
          while( rgbdata[ptr2] != ',' && ptr2 < _length ) ptr2++;
          rgbdata[ptr2++] = 0;
          ptr1 = ptr2;
          r = atoi(rgbdata);
          g = atoi(rgbdata+ptr1);
          while( rgbdata[ptr2] != ',' && ptr2 < _length ) ptr2++;
          rgbdata[ptr2++] = 0;
          ptr1 = ptr2;
          b = atoi(rgbdata+ptr1);
          SetRGB(r,g,b);
        }
        GetRGB(r,g,b);
        sprintf(contents, "{\"RGB\":\"%d,%d,%d\"}",r,g,b);
    }
    else
    { // see if it matches a config command...
      Serial.print("Command: '");
      Serial.print(originaltopic);
      Serial.print("'...");
      if( options.IsOption(originaltopic))
      {
        Serial.println("Recognised");
        if( _length != 0 )
        {
          options.SetOption(originaltopic, payload);
          Serial.print("Saved option ");
          Serial.print(originaltopic);
          Serial.print(" with value ");
          Serial.println(payload);
        }
        sprintf(contents, "{\"%s\":\"%s\"}", originaltopic, options.GetOption(originaltopic,""));
      }
      else
      {
        Serial.print("NOT recognised (length ");
        Serial.print(_length);
        Serial.println(")");
        sprintf(contents, "{\"Command\":\"Unknown\"}");
      }
    }
  }
  Serial.print("Payload handled, result: ");
  Serial.println(contents);
  // now send the result mqtt message to confirm what happened
  char message[128];
  sprintf(message, "%s/%s/RESULT", options.MqttTopic(), options.Prefix2());
  //Serial.print("Going to publish ");
  //Serial.println(message);
  //Serial.println(contents);
  PSclient.publish(message, contents, false);
  //Serial.println("Done");
}

/*
 * Send off the mqtt discovery message for HA
 */
void setupAutoDisco( bool _clear )
{
  Serial.print("Sending auto discovery data...");
  for (int i=1; i<=NUM_OUTS; i++)
  {
    String UID = String(options.UID()) + "_LI_" + i;
    String disco = "homeassistant/light/" + UID + "/config";
    Serial.print(i);
    Serial.print("...");
    if( options.OutType(i) != OUTTYPE_NONE  && !_clear )
    {
      String data = "{\"name\":\"";
      data += options.FriendlyNameParsed(i);
      data += "\"";
      data += ",\"cmd_t\":\"~/cmnd/POWER" + String(i) + "\"";
      data += ",\"stat_t\":\"~/";
      data += options.Prefix3();
      data += "/STATE\"";
      data += ",\"val_tpl\":\"{{value_json.POWER" + String(i) + "}}\"";
      if( options.OutType(1) == OUTTYPE_RGB )
      { // it's an RGB output
        data += ",\"rgb_cmd_t\":\"~/cmnd/RGB\"";
        data += ",\"rgb_stat_t\":\"~/";
        data += options.Prefix3();
        data += "/STATE\"";
        data += ",\"rgb_val_tpl\":\"{{value_json.RGB}}\"";
        data += ",\"bri_cmd_t\":\"~/cmnd/BRIGHT\"";
        data += ",\"bri_stat_t\":\"~/";
        data += options.Prefix3();
        data += "/STATE\"";
        data += ",\"bri_val_tpl\":\"{{value_json.BRIGHT}}\"";
      }
      data += ",\"pl_off\":\"OFF\",\"pl_on\":\"ON\",\"avty_t\":\"~/";
      data += options.Prefix3();
      data += "/LWT\",\"pl_avail\":\"Online\",\"pl_not_avail\":\"Offline\"";
      data += ",\"uniq_id\":\"" + UID + "\"";
      data += ",\"device\":{\"identifiers\":[\"";
      data += options.UID();
      data += "\"]";
      data += ",\"name\":\"";
      data += options.Unit();
      data += "\"";
      data += ",\"model\":\"JAD-8266 Common\"";
      data += ",\"sw_version\":\"";
      data += P_VERSION;
      data += "\"";
      data += ",\"manufacturer\":\"JADixon\"";

      data += "}";
      data += ",\"~\":\"";
      data += options.MqttTopic();
      data += "\"";
      data += "}";
      if( !PSclient.publish(disco.c_str(), data.c_str(), true) )
      {
        Serial.println(data.c_str());
        Serial.println("FAIL.");
      }
    }
    else
    {
      // send empty payload to remove any previously retained message
      if(!PSclient.publish(disco.c_str(), "", true))
      {
        Serial.println("FAIL.");
      }
    }
  }
  // register as a sensor:
  String UID = String(options.UID()) + "_SE_1";
  String disco = "homeassistant/sensor/" + UID + "/config";
  if( !_clear && *options.AnalogTrigger() != 0 )
  {
    String data = "{\"name\":\"";
    data += options.Unit();
    data += "AnalogSense";
    data += "\"";
    data += ",\"stat_t\":\"~/";
    data += options.Prefix3();
    data += "/STATE\"";
    data += ",\"val_tpl\":\"{{value_json.AnalogStat}}\"";
    data += ",\"unit_of_meas\":\" \",\"avty_t\":\"~/";
    data += options.Prefix3();
    data += "/LWT\",\"pl_avail\":\"Online\",\"pl_not_avail\":\"Offline\"";
    data += ",\"uniq_id\":\"" + UID + "\"";
    data += ",\"device\":{\"identifiers\":[\"";
    data += options.UID();
    data += "\"]";
    data += ",\"name\":\"";
    data += options.Unit();
    data += "\"";
    data += ",\"model\":\"JAD-8266 Common\"";
    data += ",\"sw_version\":\"";
    data += P_VERSION;
    data += "\"";
    data += ",\"manufacturer\":\"JADixon\"";

    data += "}";
    data += ",\"~\":\"";
    data += options.MqttTopic();
    data += "\"";
    data += "}";
    if( !PSclient.publish(disco.c_str(), data.c_str(), true) )
    {
      Serial.println(data.c_str());
      Serial.println("FAIL.");
    }
  }
  else
  {
    // send empty payload to remove any previously retained message
    PSclient.publish(disco.c_str(), "", true);
  }
  Serial.println("Done AD.");
}
