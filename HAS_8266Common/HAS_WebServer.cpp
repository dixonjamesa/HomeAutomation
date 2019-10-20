// Web server functionality

#include "HAS_WebServer.h"
#include "HAS_option.h"
#include "HAS_Main.h"
#include "HAS_Comms.h"
#include "HAS_Animation.h"
#include <ESP8266WebServer.h>

void response( const char *_title, String &_content );
ESP8266WebServer TheServer(80);

const String HtmlHtml = "<html><head>"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1,user-scalable=no\" />";
const String HtmlStyles = "<style>div,fieldset,input,select{padding:5px;font-size:1em;}"
  "input{width:100%;box-sizing:border-box;-webkit-box-sizing:border-box;-moz-box-sizing:border-box;}"
  "select{width:100%;}"
  "textarea{resize:none;width:98%;height:318px;padding:5px;overflow:auto;}"
  "body{text-align:center;font-family:verdana;}"
  "td{padding:0px;}"
  "button{border:0;border-radius:0.3rem;background-color:#1fa3ec;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;-webkit-transition-duration:0.4s;transition-duration:0.4s;cursor:pointer;}"
  "button:hover{background-color:#0e70a4;}"
  ".bred{background-color:#d43535;}"
  ".bred:hover{background-color:#931f1f;}"
  ".bgrn{background-color:#47c266;}"
  ".bgrn:hover{background-color:#5aaf6f;}"
  ".bnot{background-color:#888888;}"
  "a{text-decoration:none;}"
  ".p{float:left;text-align:left;}"
  ".q{float:right;text-align:right;}</style>";
const String HtmlHtmlClose = "</html>";
const String HtmlTitle = "<h1>Miniserve Acess Point</h1><br/>\n";
const String HtmlScan = "<a href=\"./wifi?scan=1\"><button>Scan</button></a><br/>";
const String HtmlMenu = "<div style=\"display:inline-block;min-width:340px;\">";
const String HtmlMenuEnd = "</div>";
const String HtmlFooter = "<div style=\"text-align:right;font-size:11px;\"><hr><span style=\"color:#aaa;\">v0.1.7 (c) 2019, James Dixon</span></div>";
/*
 * Put the module name as a title
 */
String HtmlModule()
{
  String result;

  result = "<div style=\"text-align:center;\"><h2>";
  result += options.Unit();
  result += "</h2></div>";
  return result;
}

/*
 * Named box of parameters
 */
String HtmlParamBoxStart( const char *_title, const char *_action )
{
  String result = "<fieldset><legend style=\"text-align:left;\"><b>";
  result += _title;
  result += "</b></legend><form method=\"get\" name=\"pform\" action=\"";
  result += _action;
  result += "\">";
  return result;
}

/*
 * Add a parameter
 *
 */
String HtmlParamParam( const char *_title, const char *_subtitle, const char *_value, const char *_id, const char *_name, const char *_pass)
{
  String result = "<div style=\"margin-top:10px; text-align:left;\"><b>";
  result += _title;
  result += "</b>";
  if( _subtitle != NULL && _subtitle[0] != 0 )
  {
    result += " (";
    result += _subtitle;
    result += ")";
  }
  result += "</div><input id=\"";
  result += _id;
  result += "\" name=\"";
  result += _name;
  result += "\" value=\"";
  result += _value;
  result += "\" ";
  if( _pass != NULL )
  {
    result += "type=\"password\" placeholder=\"";
    result += _pass;
    result += "\" ";
  }
  result += "><br>";
  return result;
}

const String ParamSmTableStart = "<div style=\"margin-top:0;padding:2px;text-align:left;\"><table>";
const String ParamSmTableEnd = "</table></div>";
/*
 * Smaller footprint param editor (all horz)
 */
String HtmlParamParamSm( const char *_title, const char *_value, const char *_id, const char *_name)
{
  String result = "<tr><td><b>";
  result += _title;
  result += ":&nbsp;</b>";
  result += "</td><td><input id=\"";
  result += _id;
  result += "\" name=\"";
  result += _name;
  result += "\" value=\"";
  result += _value;
  result += "\" ";
  result += "></td></tr>";
  return result;
}

/* Return a string with each output type as an option (and select _sel)
 * Used to pass option list to HtmlParamOutTypeList
  */
String HTMLOutTypeList(int _sel, bool _addRGB)
{
  String ret = "<option value=\"0\"";
  if( _sel == 0 ) ret += " selected ";
  ret += ">N/a</option>";
  ret += "<option value=\"1\"";
  if( _sel == 1 ) ret += " selected ";
  ret += ">On/Off</option>";
  if( _addRGB)
  {
    ret += "<option value=\"2\"";
    if( _sel == 2 ) ret += " selected ";
    ret += ">RGB</option>";
  }
  return ret;
}

/* Return a string with each switch type as an option (and select _sel)
 * Used to pass option list to HtmlParamOutTypeList
  */
String HTMLSwTypeList(int _sel )
{
  String ret = "<option value=\"0\"";
  if( _sel == 0 ) ret += " selected";
  ret += ">N/a</option>";
  ret += "<option value=\"1\"";
  if( _sel == 1 ) ret += " selected";
  ret += ">Press</option>";
  ret += "<option value=\"2\"";
  if( _sel == 2 ) ret += " selected";
  ret += ">Release</option>";
  ret += "<option value=\"3\"";
  if( _sel == 3 ) ret += " selected";
  ret += ">Pushbutton</option>";
  ret += "<option value=\"4\"";
  if( _sel == 4 ) ret += " selected";
  ret += ">Dual</option>";
  return ret;
}

/*
 * param as a dropdown list...
 */
String HtmlParamOutTypeList( const char *_title, const char *_id, const char *_name, String _list)
{
  String result = String("<tr><td><b>") + _title + ":&nbsp;</b>";
  //result += "</td><td><select form=\"pform\" id=\"";
  result += "</td><td><select id=\"";
  result += _id;
  result += "\" name=\"";
  result += _name;
  result += "\">";
  result += _list;
  result += "</select></td></tr>";
  return result;
}

String HtmlSaveButton = "<br><button class=\"button bgrn\" name=\"save\" type=\"submit\">Save</button>";
String HtmlParamBoxEnd = "</form></fieldset>";

/*
 * Add a button to the html
 */
const String AddButton(const char *_name, const char *_path, const char *_tags = NULL, const char *_class= NULL, const char *_inputs = NULL)
{
  String ret = "<form id=\"tform\" action=\"";
  ret += _path;
  ret += "\" method=\"get\" ";
  ret += _tags;
  ret += ">";
  if( _inputs != NULL )
  {
    ret += _inputs;
  }
  ret += "<button class='";
  ret += _class;
  ret += "'>";
  ret += _name;
  ret += "</button><br></form>";
  return ret;
}

const String AddSlider(const char *_name, const char *_path, int _max, int _val)
{
  char tb[10], tb2[10];
  sprintf(tb, "%d", _max);
  sprintf(tb2, "%d", _val);
  String ret = "<form id=\"tform\" action=\"";
  ret += _path;
  ret += "\" method=\"get\" >";
  ret += "<div class=\"slidecontainer\">";
  ret += "<input type=\"range\" min=\"1\" max=\"";
  ret += tb;
  ret += "\" value=\"";
  ret += tb2;
  ret += "\" class=\"slider\" name=\"myRange\" onchange=\"document.getElementById('tform').submit();\">";
  ret += "</div>";

  ret += "<button class='";
  ret += "bgrn";
  ret += "'>";
  ret += _name;
  ret += "</button><br>";

  ret += "</form>";

  return ret;
}

const String AddTableRow(const char *_head, const char *_data)
{
  String ret = "<tr>";
  ret += "<th>";
  ret += _head;
  ret += "<th><td>";
  ret += _data;
  ret += "</td><tr>";
  return ret;
}

void handleReboot()
{
  Serial.println("Serving /reboot");

  setupAutoDisco(true);
  Serial.println("Auto discovery reset");
  String content = HtmlModule();
  content += HtmlMenu;

  content += "<div>Module will now reboot...</div>";
  content += "<br>";
  content += AddButton("Main Menu", "/");
  content += HtmlMenuEnd;
  response("Main Menu", content);
  Serial.println("Waiting 5s...");
  delay(5000);
  Serial.println("Resetting...");
  ESP.restart();
}
/*
 * Read back page args into options
 * and reset if requested
 */
void handlePageArgs( bool _restart )
{
  bool changed = false; // monitor if any option has changed

  if(TheServer.args() > 0 )
  {
    // args have come back, so let's apply them...
    for(int i=0 ; i < TheServer.args() ; i++)
    {
      if( TheServer.argName(i) != "save" &&
          TheServer.argName(i) != "scan" ) // reserved names - don't do anything
      {
        options.SetOption(TheServer.argName(i).c_str(), TheServer.arg(i).c_str());

        changed = true; // something has been changed - used later to decide to reboot or not
        Serial.print("Changed option ");
        Serial.print(TheServer.argName(i));
        Serial.print(" to ");
        Serial.println(TheServer.arg(i));
        }
    }
    if( changed && _restart )
    {
      // restart the unit:
      handleReboot();
    }
  }
}

/*
 * Output switch and state
 * State will be shown as ON or OFF if _outEnabled is true
 * _name is the name shown on the switch button
 * _message is the http GET payload parameter name
 */
String HtmlSwitch( const char *_name, const char *_message, bool _outEnabled, int _id, int _type, bool _rgb=false )
{
  // show the current state of the output pin:
  String content = "<div style=\"text-align:center;font-size:1.5rem;border:solid 1px black;border-radius:5px;margin:2px;\">";
  if( _outEnabled )
  {
    content += GetOutput(_id)?"ON":"OFF";
  }
  else
  { // output not connected
    content += "N/a";
  }
  content += "</div>";
  // add a button to simulate pressing the switch:
  String inputs = (String("<input type='hidden' name='")+_message+"'>");
  if( _rgb )
  {
    char tb[10], tb2[10];
    sprintf(tb, "%d", options.RGBCount());
    sprintf(tb2, "%d", GetStripLength());
    inputs += "<div class=\"slidecontainer\">";
    inputs += "<input type=\"range\" min=\"1\" max=\"";
    inputs += tb;
    inputs += "\" value=\"";
    inputs += tb2;
    inputs += "\" class=\"slider\" name=\"";
    inputs += "StripLength";
    inputs += "\" onchange=\"document.getElementById('tform').submit();\">";
    inputs += "</div>";
  }
  content += AddButton(_name, "", "style='margin:2px;'", _type==0?"button bnot":"", inputs.c_str());

  return content;
}

/*
 * Output ability to fake rotary via web
 */
String HtmlRotary( const char *_name, bool _disabled)
{
  return ""; // currently disabled until better interface can be thought of ;)
  String dat1 = String("<input type='hidden' name='") + _name + "' value='-1'>";
  String dat2 = String("<input type='hidden' name='") + _name + "' value='1'>";
  String content = "<div style='padding:0;'><table width='100%' style='margin:0px;padding:0;border:0;'><tr><td>";
  content += AddButton("<", "", "style='margin:2px;'", _disabled?"button bnot":"", dat1.c_str());
  content += "</td><td>";
  content += AddButton(">", "", "style='margin:2px;'", _disabled?"button bnot":"", dat2.c_str());
  content += "</td></tr></table></div>";
  return content;
}

/*
 * Handle switch message via web interface
 */
void WebSwitch(int id)
{
  switch(switches[id].type)
  {
    case SWTYPE_NONE:
    case SWTYPE_PRESS:
    case SWTYPE_RELEASE:
      switches[id].Toggle();
      break;
    case SWTYPE_PUSHBUTTON:
      switches[id].On();
      switches[id].Off();
      break;
    case SWTYPE_DUAL:
    case SWTYPE_DELAY:
      switches[id].On();
      break;
  }
}
void handleRoot()
{
  Serial.println("Serving /");
  String content = HtmlModule();
  content += HtmlMenu;

  if(TheServer.args() > 0 )
  {
    char topic[128];
    // args have come back, so let's apply them...
    Serial.print("Passed arg count: ");
    Serial.println(TheServer.args());
    for(int i=0 ; i < TheServer.args() ; i++)
    {
      Serial.print("Passed arg: ");
      Serial.print(TheServer.argName(i));
      Serial.print(": ");
      Serial.println(TheServer.arg(i));
      if( TheServer.argName(i) == "StripLength") SetStripLength(atoi(TheServer.arg(i).c_str()));
      if( TheServer.argName(i) == "Switch1")      WebSwitch(0);
      else if( TheServer.argName(i) == "Switch2") WebSwitch(1);
      else if( TheServer.argName(i) == "Switch3") WebSwitch(2);
      else if( TheServer.argName(i) == "Switch4") WebSwitch(3);
      else if( TheServer.argName(i) == "Switch5") WebSwitch(4);
      else if( TheServer.argName(i) == "Switch6") WebSwitch(5);
      else if( TheServer.argName(i) == "Switch7") WebSwitch(6);
      else if( TheServer.argName(i) == "Switch8") WebSwitch(7);
      else if( TheServer.argName(i) == "Rotary1")
      {
        sprintf(topic, "%s/%s/ROTARY1", options.MqttTopic(), options.Prefix1());
        PSclient.publish(topic, TheServer.arg(i).c_str(), false);
      }
      else if( TheServer.argName(i) == "Rotary2")
      {
        sprintf(topic, "%s/%s/ROTARY2", options.MqttTopic(), options.Prefix1());
        PSclient.publish(topic, TheServer.arg(i).c_str(), false);
      }
      else if( TheServer.argName(i) == "Rotary3")
      {
        sprintf(topic, "%s/%s/ROTARY3", options.MqttTopic(), options.Prefix1());
        PSclient.publish(topic, TheServer.arg(i).c_str(), false);
      }
      else if( TheServer.argName(i) == "Rotary4")
      {
        sprintf(topic, "%s/%s/ROTARY4", options.MqttTopic(), options.Prefix1());
        PSclient.publish(topic, TheServer.arg(i).c_str(), false);
      }
    }
  }

  int cellcount = 0;
  // table to contain all switches and crrent outputs
  content += "<table width=\"100%\"><tr>";
  for( int i=0;i<NUM_OUTS ; i++ )
  {
    if( options.SwType(i+1) != SWTYPE_NONE || options.OutType(i+1) != OUTTYPE_NONE)
    {
      bool rgb = false;
      char tbuf[16];
      sprintf(tbuf,"Switch%d",i+1);
      content += "<td valign='top'>";
      //content += HtmlSwitch(tbuf+6, tbuf, options.OutPin(i+1) != -1, i+1, options.SwType(i+1));
      if( i == 0 && options.OutType(1) == OUTTYPE_RGB ) rgb = true;
      content += HtmlSwitch(options.FriendlyNameParsed(i+1), tbuf, options.OutPin(i+1) != -1, i+1, options.SwType(i+1), rgb);
      //content += HtmlRotary( "Rotary1", options.Rot1PinU() != -1);
      content += "</td>";
      if( ++cellcount > 1 )
      { // new row once 2 items outputted
        cellcount = 0;
        content += "</tr><tr>";
      }
    }
  }
  content += "</tr></table>";

  // now the buttons:
  content += "<br>";
  //content += AddButton("Status", "status");
  content += AddButton("Configuration", "config");
  content += AddButton("Information", "info");
  content += "<br>";
  content += AddButton("Restart", "/reboot", "onsubmit=\"return confirm('Confirm Restart');\"", "button bred");
  content += HtmlMenuEnd;
  response("Main Menu", content);
}

void handleInfo()
{
  Serial.println("Serving /info");
  String content = HtmlModule();
  content += HtmlMenu;
  content += "<div><table style=\"width:100%;\"><tbody>";
  content += AddTableRow("Web Name", options.WebNameParsed());
  content += AddTableRow("SSID (RSSI)", (WiFi.SSID() + " ("+WiFi.RSSI()+"%)").c_str());
  content += AddTableRow("IP Address", WiFi.localIP().toString().c_str());
  content += AddTableRow("Gateway", WiFi.gatewayIP().toString().c_str());
  content += AddTableRow("MQTT Host", options.MqttHost());
  content += AddTableRow("MQTT connection", PSclient.connected()?"Connected":"Disconnected");
  content += "</tbody></table>";
  content += AddButton("Main Menu", "/");
  content += HtmlMenuEnd;
  response("Information", content);
}

void handleSwitches()
{
  Serial.println("Serving /config/sw");
  String content = HtmlModule();

  handlePageArgs( true );

  content += HtmlMenu;

  content += HtmlParamBoxStart("Switches", "./sw");

  content += ParamSmTableStart;
  for( int i=0 ; i < NUM_SWITCHES ; i++ )
  {
    char n1buf[16], n2buf[16], n3buf[16], n4buf[16];
    sprintf(n1buf,"S%d Type", i+1);
    sprintf(n2buf,"S%d Topic", i+1);
    sprintf(n3buf,"Sw%dType", i+1);
    sprintf(n4buf,"Sw%dTopic", i+1);
    content += HtmlParamOutTypeList(n1buf, "", n3buf, HTMLSwTypeList(options.SwType(i+1)));
    content += HtmlParamParamSm(n2buf, options.SwTopic(i+1), "", n4buf);
  }
  content += ParamSmTableEnd;
  content += HtmlSaveButton;
  content += HtmlParamBoxEnd;
  content += "<br>";
  content += AddButton("Configuration", "/config");
  content += HtmlMenuEnd;
  Serial.println(content);
  response("Switches", content);
}

void handleRotaries()
{
  Serial.println("Serving /config/rot");
  String content = HtmlModule();

  handlePageArgs( true );

  content += HtmlMenu;

  content += HtmlParamBoxStart("Rotary Params", "./rot");

  content += ParamSmTableStart;
  for( int i=0 ; i < NUM_ROTS ; i++ )
  {
    char n1buf[16], n2buf[16], n3buf[16], n4buf[16];
    //sprintf(n1buf,"Sw%d Type", i+1);
    sprintf(n2buf,"Rot%d Topic", i+1);
    //sprintf(n3buf,"Sw%dType", i+1);
    sprintf(n4buf,"Rot%dTopic", i+1);
    //content += HtmlParamOutTypeList(n1buf, "", n3buf, HTMLSwTypeList(options.SwType(i+1)));
    content += HtmlParamParamSm(n2buf, options.RotTopic(i+1), "", n4buf);
  }
  content += ParamSmTableEnd;
  content += HtmlSaveButton;
  content += HtmlParamBoxEnd;
  content += "<br>";
  content += AddButton("Configuration", "/config");
  content += HtmlMenuEnd;

  response("Switches", content);
}

void handleLEDs()
{
  char tbuf[16];
  Serial.println("Serving /config/module");
  String content = HtmlModule();

  handlePageArgs( true );

  content += HtmlMenu;

  content += HtmlParamBoxStart("LED Params", "./led");

  content += ParamSmTableStart;

  for( int i=0 ; i < NUM_OUTS ; i++ )
  {
    char n1buf[16], n2buf[16], n3buf[16], n4buf[16];
    sprintf(n1buf,"Out%d Type", i+1);
    sprintf(n2buf,"LED%d Topic", i+1);
    sprintf(n3buf,"Out%dType", i+1);
    sprintf(n4buf,"LED%dTopic", i+1);
    content += HtmlParamOutTypeList(n1buf, "", n3buf, HTMLOutTypeList(options.OutType(i+1), i==0 /* only first one can be RGB */));
    content += HtmlParamParamSm(n2buf, options.LEDTopic(i+1), "", n4buf);
  }
  sprintf(tbuf, "%d", options.RGBCount()); content += HtmlParamParamSm("RGB LED Count", tbuf, "", "RGBCount");

  content += ParamSmTableEnd;
  content += HtmlSaveButton;
  content += HtmlParamBoxEnd;
  content += "<br>";
  content += AddButton("Configuration", "/config");
  content += HtmlMenuEnd;

  response("LEDs", content);
}

void handleModule()
{
  char tbuf[4];
  Serial.println("Serving /config/module");
  String content = HtmlModule();

  handlePageArgs( true );

  content += HtmlMenu;

  content += HtmlParamBoxStart("Module Params", "./module");
  content += HtmlParamParam("Name", options.Unit(), options.Unit(), "unit", "Unit", NULL);
  //content += HtmlParamParam("SoftAP", String(options.APServer()).c_str(), String(options.APServer()).c_str(), "port", "APServer", NULL);
  content += HtmlParamParam("WebName", options.WebNameParsed(), options.WebName(), "wname", "WebName", NULL);
  content += HtmlParamParam("Password", NULL, options.AP_pass(), "pwd1", "WebPassword", "password");
  for(int i=0 ; i<NUM_OUTS ; i++)
  {
    char nbuf[20],n2buf[16];
    sprintf(nbuf,"Friendly Name %d",i+1);
    sprintf(n2buf,"FriendlyName%d",i+1);
    content += HtmlParamParam(nbuf, options.FriendlyNameParsed(i+1), options.FriendlyName(i+1), "", n2buf, NULL);
  }

  content += HtmlParamParam("Prefix1", options.Prefix1(), options.Prefix1(), "pre1", "Prefix1", NULL);
  content += HtmlParamParam("Prefix2", options.Prefix2(), options.Prefix2(), "pre2", "Prefix2", NULL);
  content += HtmlParamParam("Prefix3", options.Prefix3(), options.Prefix3(), "pre3", "Prefix3", NULL);
  content += HtmlSaveButton;
  content += HtmlParamBoxEnd;
  content += "<br>";
  content += AddButton("Configuration", "/config");
  content += HtmlMenuEnd;

  response("Module", content);
}

void handleHW()
{
  char tbuf[4];
  Serial.println("Serving /config/hw");
  String content = HtmlModule();

  handlePageArgs( true );

  content += HtmlMenu;

  content += HtmlParamBoxStart("Hardware", "./hw");
  content += ParamSmTableStart;
  for(int i=0 ; i < NUM_SWITCHES ; i++)
  {
    char n1buf[16], n2buf[16];
    sprintf(n1buf,"Sw%d Pin 1", i+1);
    sprintf(n2buf,"Sw%dPin1", i+1);
    sprintf(tbuf, "%d", options.SwPin1(i+1)); content += HtmlParamParamSm(n1buf, tbuf, "", n2buf);
    sprintf(n1buf,"Sw%d Pin 2", i+1);
    sprintf(n2buf,"Sw%dPin2", i+1);
    sprintf(tbuf, "%d", options.SwPin2(i+1)); content += HtmlParamParamSm(n1buf, tbuf, "", n2buf);
  }

  for(int i=0 ; i < NUM_ROTS ; i++)
  {
    char n1buf[16], n2buf[16];
    sprintf(n1buf,"Rot%d PinU", i+1);
    sprintf(n2buf,"Rot%dPinU", i+1);
    sprintf(tbuf, "%d", options.RotPinU(i+1)); content += HtmlParamParamSm(n1buf, tbuf, "", n2buf);
    sprintf(n1buf,"Rot%d PinD", i+1);
    sprintf(n2buf,"Rot%dPinD", i+1);
    sprintf(tbuf, "%d", options.RotPinD(i+1)); content += HtmlParamParamSm(n1buf, tbuf, "", n2buf);
  }

  sprintf(tbuf, "%d", options.StatusLED()); content += HtmlParamParamSm("Status LED", tbuf, "", "StatusLED");
  sprintf(tbuf, "%d", options.InvertLED()); content += HtmlParamParamSm("Invert LED", tbuf, "", "InvertLED");

  content += ParamSmTableEnd;
  content += HtmlSaveButton;
  content += HtmlParamBoxEnd;
  content += "<br>";
  content += AddButton("Configuration", "/config");
  content += HtmlMenuEnd;

  response("Hardware", content);
}

void handleHW2()
{
  char tbuf[4];
  Serial.println("Serving /config/hw2");
  String content = HtmlModule();

  handlePageArgs( true );

  content += HtmlMenu;

  content += HtmlParamBoxStart("Hardware", "./hw");
  content += ParamSmTableStart;
  for(int i=0 ; i < NUM_OUTS ; i++)
  {
    char n1buf[16], n2buf[16];
    sprintf(n1buf,"Out%d Pin", i+1);
    sprintf(n2buf,"Out%dPin", i+1);
    sprintf(tbuf, "%d", options.OutPin(i+1)); content += HtmlParamParamSm(n1buf, tbuf, "", n2buf);
    sprintf(n1buf,"Out%d LED", i+1);
    sprintf(n2buf,"Out%dLED", i+1);
    sprintf(tbuf, "%d", options.OutLED(i+1)); content += HtmlParamParamSm(n1buf, tbuf, "", n2buf);
  }

  content += ParamSmTableEnd;
  content += HtmlSaveButton;
  content += HtmlParamBoxEnd;
  content += "<br>";
  content += AddButton("Configuration", "/config");
  content += HtmlMenuEnd;

  response("Hardware2", content);
}

void handleConfig()
{
  Serial.println("Serving /config");
  String content = HtmlModule();
  content += HtmlMenu;
  content += AddButton("Module", "config/module");
  content += AddButton("Switches", "config/sw");
  content += AddButton("Rotaries", "config/rot");
  content += AddButton("LEDs", "config/led");
  content += AddButton("WiFi", "config/wifi");
  content += AddButton("MQTT", "config/mqtt");
  content += AddButton("Hardware", "config/hw");
  content += AddButton("Hardware2", "config/hw2");
  content += "<br>";
  content += AddButton("Main Menu", "/");
  content += HtmlMenuEnd;
  response("Config", content);
}

void handleMQTT()
{
  char tb[16];
  Serial.println("Serving /config/mqtt");
  String content = HtmlModule();

  handlePageArgs( true );

  content += HtmlMenu;

  content += HtmlParamBoxStart("Mqtt Params", "./mqtt");
  content += HtmlParamParam("Host", options.MqttHost(), options.MqttHost(), "host", "MqttHost", NULL);
  sprintf(tb, "%d", options.MqttPort());
  content += HtmlParamParam("Port", tb, tb, "port", "MqttPort", NULL);
  content += HtmlParamParam("User", options.MqttUser(), options.MqttUser(), "user", "MqttUser", NULL);
  content += HtmlParamParam("Password", NULL, "", "pwd1", "MqttPass", "password");
  content += HtmlParamParam("Client", options.MqttClientParsed(), options.MqttClient(), "client", "MqttClient", NULL);
  content += HtmlParamParam("Topic", options.MqttTopic(), options.MqttTopic(), "topic", "MqttTopic", NULL);
  content += HtmlParamParam("Group Topic", options.MqttGroupTopic(), options.MqttGroupTopic(), "gtopic", "MqttGroupTopic", NULL);
  content += HtmlSaveButton;
  content += HtmlParamBoxEnd;
  content += "<br>";
  content += AddButton("Configuration", "/config");
  content += HtmlMenuEnd;

  response("Mqtt", content);
}

/*
 * Wifi config page
 */
void handleWifi()
{
  Serial.println("Serving /config/wifi");

  int numNetworks = 0;

  // if the "scan" argument is passed, scan and show available networks
  for(int i=0 ; i < TheServer.args() ; i++)
  {
    if( TheServer.argName(i) == "scan" )
    {
      // scan for all networks, including hidden if 2nd param true
      WiFi.scanNetworks(false, false);
      numNetworks = WiFi.scanComplete();
    }
  }
  // pass page args back to options
  handlePageArgs( true );

  String content = "<script>function ei(i){return document.getElementById(i);}function c(l){ei('ss1').value=l.innerText||l.textContent;ei('pwd1').focus();}</script>";
  content += HtmlModule();
  content += HtmlScan;
  content += "<p>"+WIFI_status+"</p>";
  content += HtmlMenu;

  if( numNetworks > 0 )
  {
    for(int i=0;i<numNetworks ; i++)
    {
      content += "<div><a style=\"float:left;text-align:left;\" href=\"#fs\" onclick=\"c(this)\">";
      content += WiFi.SSID(i);
      content += "</a>";
      content += "(";
      content += WiFi.channel(i);
      content += ")<span class=\"q\">";
      content += WiFi.BSSIDstr(i);
      content += "</span></div>";
    }
  }

  content += "<br>";
  content += HtmlParamBoxStart("Wifi Params", "./wifi");
  content += HtmlParamParam("SSID", WiFi.SSID().c_str(), WiFi.SSID().c_str(), "ss1", "Ssid", NULL);
  content += HtmlParamParam("Password", NULL, "", "pwd1", "Password", "password");
  content += HtmlSaveButton;
  content += HtmlParamBoxEnd;
  content += "<br>";
  content += AddButton("Configuration", "/config");
  content += HtmlMenuEnd;

  response("WiFi", content);
}


///
// HTML general response
///
void response(const char *_title, String &_content)
{
#if true
  String htmlRes = HtmlHtml + "<title>" + options.Unit() + " - ";
  htmlRes += _title;
  htmlRes += "</title></head>" + HtmlStyles + "<body>";
  htmlRes += _content;
  htmlRes += HtmlFooter;
  htmlRes += "</body>" + HtmlHtmlClose;
  Serial.print("Serving ");
  Serial.print(htmlRes.length());
  Serial.print(" bytes data, content is ");
  Serial.print(_content.length());
#else
  static char htmlRes[6000];
  sprintf(htmlRes,"FUCOF");
  /*
  sprintf(htmlRes, "%s%s%s%s", HtmlHtml.c_str(), "<title>", options.Unit(), " - ");
  strcat(htmlRes, _title);
  strcat(htmlRes, "</title></head>");
  strcat(htmlRes, HtmlStyles.c_str());
  strcat(htmlRes, "<body>");
  strcat(htmlRes, _content.c_str());
  strcat(htmlRes, HtmlFooter.c_str());
  strcat(htmlRes, "</body>");
  strcat(htmlRes, HtmlHtmlClose.c_str());
  Serial.print("Serving ");
  Serial.print(strlen(htmlRes));
  Serial.print(" bytes data, content is ");
  Serial.print(_content.length());*/
#endif
  TheServer.send(200, "text/html", htmlRes);
}

void InitWebServer()
{
  TheServer.on("/", handleRoot);
  TheServer.on("/reboot", handleReboot);
  TheServer.on("/info", handleInfo);
  TheServer.on("/config", handleConfig);
  TheServer.on("/config/module", handleModule);
  TheServer.on("/config/sw", handleSwitches);
  TheServer.on("/config/rot", handleRotaries);
  TheServer.on("/config/led", handleLEDs);
  TheServer.on("/config/wifi", handleWifi);
  TheServer.on("/config/mqtt", handleMQTT);
  TheServer.on("/config/hw", handleHW);
  TheServer.on("/config/hw2", handleHW2);
  TheServer.begin();
}

void UpdateWebServer()
{
  TheServer.handleClient();
}
