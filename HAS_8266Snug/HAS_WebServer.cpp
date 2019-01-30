// Web server functionality

#include "HAS_WebServer.h"
#include "HAS_option.h"
#include "HAS_Main.h"
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
const String HtmlLedStateLow = "<big>LED is now <b>ON</b></big><br/>\n";
const String HtmlLedStateHigh = "<big>LED is now <b>OFF</b></big><br/>\n";
const String HtmlScan = "<a href=\"./wifi?scan=1\"><button>Scan</button></a><br/>";
const String HtmlMenu = "<div style=\"display:inline-block;min-width:340px;\">";
const String HtmlMenuEnd = "</div>";
const String HtmlFooter = "<div style=\"text-align:right;font-size:11px;\"><hr><span style=\"color:#aaa;\">v0.1.3 (c) 2019, James Dixon</span></div>";
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
  result += "</b></legend><form method=\"get\" action=\"";
  result += _action;
  result += "\">";
  return result;
}
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

/*
 * Smaller footprint param editor (all horz)
 */
String HtmlParamParamSm( const char *_title, const char *_value, const char *_id, const char *_name)
{
  String result = "<div style=\"margin-top:0;padding:2px;text-align:left;\"><table><tr><td><b>";
  result += _title;
  result += ":&nbsp;</b>";
  result += "</td><td><input id=\"";
  result += _id;
  result += "\" name=\"";
  result += _name;
  result += "\" value=\"";
  result += _value;
  result += "\" ";
  result += "></td></tr></table></div>";
  return result;
}

String HtmlSaveButton = "<br><button class=\"button bgrn\" name=\"save\" type=\"submit\">Save</button>";
String HtmlParamBoxEnd = "</form></fieldset>";

/*
 * Add a button to the html
 */
const String AddButton(const char *_name, const char *_path, const char *_tags = "", const char *_class="", const char *_inputs = NULL)
{
  String ret = "<form action=\"";
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
  String content = HtmlModule();
  content += HtmlMenu;

  content += "<div>Module will now reboot...</div>";
  content += "<br>";
  content += AddButton("Main Menu", "/");
  content += HtmlMenuEnd;
  response("Main Menu", content);
  delay(1000);
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
 */
String HtmlSwitch( const char *_name, const char *_message, bool _outEnabled, int _type )
{
  String content = "<div style=\"text-align:center;font-size:1.5rem;border:solid 1px black;border-radius:5px;margin:2px;\">";
  if( _outEnabled )
  {
    content += GetOutput(1)?"ON":"OFF";
  }
  else
  {
    content += "N/a";
  }
  content += "</div>";
  content += AddButton(_name, "", "style='margin:2px;'", _type==0?"button bnot":"", (String("<input type='hidden' name='")+_message+"'>").c_str()); 

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
    case 0:
      break;
    case 1:
    case 2:
      switches[id].Toggle();
      break;
    case 3:
      switches[id].On();
      switches[id].Off();
      break;            
    case 4:
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
    // args have come back, so let's apply them...
    for(int i=0 ; i < TheServer.args() ; i++)
    {
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
        PSclient.publish((options.MqttTopic()+"/"+options.Prefix1()+"/"+"ROTARY1").c_str(), TheServer.arg(i).c_str(), false);
      }
      else if( TheServer.argName(i) == "Rotary2")
      {
        PSclient.publish((options.MqttTopic()+"/"+options.Prefix1()+"/"+"ROTARY2").c_str(), TheServer.arg(i).c_str(), false);
      }
      else if( TheServer.argName(i) == "Rotary3")
      {
        PSclient.publish((options.MqttTopic()+"/"+options.Prefix1()+"/"+"ROTARY3").c_str(), TheServer.arg(i).c_str(), false);
      }
      else if( TheServer.argName(i) == "Rotary4")
      {
        PSclient.publish((options.MqttTopic()+"/"+options.Prefix1()+"/"+"ROTARY4").c_str(), TheServer.arg(i).c_str(), false);
      }
    }
  }

  content += "<table width=\"100%\"><tr>";
  for( int i=0;i<NUM_OUTS ; i++ )
  {
    if( options.SwType(i+1) != 0 || options.OutPin(i+1) != -1)
    {
      char tbuf[16];
      sprintf(tbuf,"Switch%d",i+1);
      content += "<td valign='top'>";
      content += HtmlSwitch(tbuf+6, tbuf, options.OutPin(i+1) != -1, options.SwType(i+1));
      //content += HtmlRotary( "Rotary1", options.Rot1PinU() != -1);
      content += "</td>";
    }
  }
  content += "</tr></table>";
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
  content += AddTableRow("MQTT Host", options.MqttHost().c_str());
  content += AddTableRow("MQTT connection", PSclient.connected()?"Connected":"Disconnected");
  content += "</tbody></table>";
  content += AddButton("Main Menu", "/");
  content += HtmlMenuEnd;
  response("Information", content);
}

#define SWITCHTYPES "0 Dis, 1 Press, 2 Release, 3 PB, 4 Dual"
void handleModule()
{
  char tbuf[4];
  Serial.println("Serving /config/module");
  String content = HtmlModule();

  handlePageArgs( true );
  
  content += HtmlMenu;

  content += HtmlParamBoxStart("Module Params", "./mqtt");
  content += HtmlParamParam("Name", options.Unit().c_str(), options.Unit().c_str(), "unit", "Unit", NULL);
  //content += HtmlParamParam("SoftAP", String(options.APServer()).c_str(), String(options.APServer()).c_str(), "port", "APServer", NULL);
  content += HtmlParamParam("WebName", options.WebNameParsed(), options.WebName().c_str(), "wname", "WebName", NULL);
  content += HtmlParamParam("Password", NULL, options.AP_pass().c_str(), "pwd1", "WebPassword", "password");
  for(int i=0 ; i<NUM_OUTS ; i++)
  {
    char nbuf[20],n2buf[16];
    sprintf(nbuf,"Friendly Name %d",i+1);
    sprintf(n2buf,"FriendlyName%d",i+1);
    content += HtmlParamParam(nbuf, options.FriendlyNameParsed(i+1), options.FriendlyName(i+1).c_str(), "", n2buf, NULL);
  }

  content += "<div>";
  content += SWITCHTYPES;
  content += "</div>";
  for( int i=0 ; i < NUM_SWITCHES ; i++ )
  {
    char n1buf[16], n2buf[16], n3buf[16], n4buf[16];
    sprintf(n1buf,"Switch%d", i+1);
    sprintf(n2buf,"Switch%d Topic", i+1);
    sprintf(n3buf,"Sw%dType", i+1);
    sprintf(n4buf,"Sw%dTopic", i+1);
    sprintf(tbuf,"%d", options.SwType(i+1));
    content += HtmlParamParamSm(n1buf, tbuf, "", n3buf);
    content += HtmlParamParam(n2buf, NULL, options.SwTopic(i+1).c_str(), "", n4buf, NULL);
  }
  content += HtmlParamParam("Prefix1", options.Prefix1().c_str(), options.Prefix1().c_str(), "pre1", "Prefix1", NULL);
  content += HtmlParamParam("Prefix2", options.Prefix2().c_str(), options.Prefix2().c_str(), "pre2", "Prefix2", NULL);
  content += HtmlParamParam("Prefix3", options.Prefix3().c_str(), options.Prefix3().c_str(), "pre3", "Prefix3", NULL);
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

  content += HtmlParamBoxStart("Hardware Params", "./hw");
  for(int i=0 ; i < NUM_SWITCHES ; i++)
  {
    char n1buf[16], n2buf[16];
    sprintf(n1buf,"Switch %d Pin 1", i+1);
    sprintf(n2buf,"Sw%dPin1", i+1);
    sprintf(tbuf, "%d", options.SwPin1(i+1)); content += HtmlParamParamSm(n1buf, tbuf, "", n2buf);
    sprintf(n1buf,"Switch %d Pin 2", i+1);
    sprintf(n2buf,"Sw%dPin2", i+1);
    sprintf(tbuf, "%d", options.SwPin2(i+1)); content += HtmlParamParamSm(n1buf, tbuf, "", n2buf);
  }

  for(int i=0 ; i < NUM_OUTS ; i++)
  {
    char n1buf[16], n2buf[16];
    sprintf(n1buf,"Output %d Pin", i+1);
    sprintf(n2buf,"Out%dPin", i+1);
    sprintf(tbuf, "%d", options.OutPin(i+1)); content += HtmlParamParamSm(n1buf, tbuf, "", n2buf);
    sprintf(n1buf,"Output %d LED", i+1);
    sprintf(n2buf,"Out%dLED", i+1);
    sprintf(tbuf, "%d", options.OutLED(i+1)); content += HtmlParamParamSm(n1buf, tbuf, "", n2buf);
  }
  
  for(int i=0 ; i < NUM_ROTS ; i++)
  {
    char n1buf[16], n2buf[16];
    sprintf(n1buf,"Rotary %d PinU", i+1);
    sprintf(n2buf,"Rot%dPinU", i+1);
    sprintf(tbuf, "%d", options.RotPinU(i+1)); content += HtmlParamParamSm(n1buf, tbuf, "", n2buf);
    sprintf(n1buf,"Rotary %d PinD", i+1);
    sprintf(n2buf,"Rot%dPinD", i+1);
    sprintf(tbuf, "%d", options.RotPinD(i+1)); content += HtmlParamParamSm(n1buf, tbuf, "", n2buf);
  }

  sprintf(tbuf, "%d", options.StatusLED()); content += HtmlParamParamSm("Status LED", tbuf, "", "StatusLED");
  sprintf(tbuf, "%d", options.InvertLED()); content += HtmlParamParamSm("Invert LED", tbuf, "", "InvertLED");

  content += HtmlSaveButton;
  content += HtmlParamBoxEnd;
  content += "<br>";
  content += AddButton("Configuration", "/config");
  content += HtmlMenuEnd;

  response("Hardware", content);  
}

void handleConfig()
{
  Serial.println("Serving /config");
  String content = HtmlModule();
  content += HtmlMenu;
  content += AddButton("Module", "config/module");
  content += AddButton("WiFi", "config/wifi");
  content += AddButton("MQTT", "config/mqtt");
  content += AddButton("Hardware", "config/hw");
  content += "<br>";
  content += AddButton("Main Menu", "/");
  content += HtmlMenuEnd;
  response("Config", content);  
}

void handleMQTT()
{
  Serial.println("Serving /config/mqtt");
  String content = HtmlModule();

  handlePageArgs( true );
  
  content += HtmlMenu;
 
  content += HtmlParamBoxStart("Mqtt Params", "./mqtt");
  content += HtmlParamParam("Host", options.MqttHost().c_str(), options.MqttHost().c_str(), "host", "MqttHost", NULL);
  content += HtmlParamParam("Port", String(options.MqttPort()).c_str(), String(options.MqttPort()).c_str(), "port", "MqttPort", NULL);
  content += HtmlParamParam("User", options.MqttUser().c_str(), options.MqttUser().c_str(), "user", "MqttUser", NULL);
  content += HtmlParamParam("Password", NULL, "", "pwd1", "MqttPass", "password");
  content += HtmlParamParam("Client", options.MqttClientParsed(), options.MqttClient().c_str(), "client", "MqttClient", NULL);
  content += HtmlParamParam("Topic", options.MqttTopic().c_str(), options.MqttTopic().c_str(), "topic", "MqttTopic", NULL);
  content += HtmlParamParam("Group Topic", options.MqttGroupTopic().c_str(), options.MqttGroupTopic().c_str(), "gtopic", "MqttGroupTopic", NULL);
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
  String htmlRes = HtmlHtml + "<title>" + options.Unit() + " - ";
  htmlRes += _title;
  htmlRes += "</title></head>" + HtmlStyles + "<body>";
  htmlRes += _content;
  htmlRes += HtmlFooter;
  htmlRes += "</body>" + HtmlHtmlClose;
  
  TheServer.send(200, "text/html", htmlRes);
}

void InitWebServer()
{
  TheServer.on("/", handleRoot);
  TheServer.on("/reboot", handleReboot);
//  TheServer.on("/status", handleStatus);
  TheServer.on("/info", handleInfo);
  TheServer.on("/config", handleConfig);
  TheServer.on("/config/module", handleModule);
  TheServer.on("/config/wifi", handleWifi);
  TheServer.on("/config/mqtt", handleMQTT);
  TheServer.on("/config/hw", handleHW);
  TheServer.begin();
}

void UpdateWebServer()
{
  TheServer.handleClient();
}
