// Web server functionality

#include "HAS_WebServer.h"
#include "HAS_option.h"
#include "HAS_Main.h"
#include "HAS_Comms.h"
#include "HAS_Animation.h"
#include "HAS_Update.h"
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

char rBuffer[4000];
void response();
ESP8266WebServer TheServer(80);

const char *HtmlHtml = "<html><head>"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1,user-scalable=no\" />";
const char *HtmlStyles = "<style>div,fieldset,input,select{padding:5px;font-size:1em;}"
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
const char *HtmlHtmlClose = "</html>";
const char *HtmlTitle = "<h1>Miniserve Acess Point</h1><br/>\n";
const char * HtmlScan = "<a href=\"./wifi?scan=1\"><button>Scan</button></a><br/>";
const char *HtmlMenu = "<div style=\"display:inline-block;min-width:340px;\">";
const char *HtmlMenuEnd = "</div>";
const char *HtmlFooter = "<div style=\"text-align:right;font-size:11px;\"><hr><span style=\"color:#aaa;\">v" P_VERSION " (c) 2019, James Dixon</span></div>";
/*
 * Put the module name as a title
 */
void HtmlModule(char *_to, const char *_title)
{
  sprintf(_to, "%s<title>%s - %s</title></head>%s<body><div style=\"text-align:center;\"><h2>%s</h2></div>", HtmlHtml, options.Unit(), _title, HtmlStyles, options.Unit());
}

/*
 * Named box of parameters
 */
void HtmlParamBoxStart( char *_to, const char *_title, const char *_action )
{
  strcat(_to, "<fieldset><legend style=\"text-align:left;\"><b>");
  strcat(_to, _title);
  strcat(_to, "</b></legend><form method=\"get\" name=\"pform\" action=\"");
  strcat(_to, _action);
  strcat(_to, "\">");
}

/*
 * Add a parameter
 *
 */
void HtmlParamParam( char *_to, const char *_title, const char *_subtitle, const char *_value, const char *_id, const char *_name, const char *_pass)
{
  strcat(_to, "<div style=\"margin-top:10px; text-align:left;\"><b>");
  strcat(_to, _title);
  strcat(_to, "</b>");
  if( _subtitle != NULL && _subtitle[0] != 0 )
  {
    strcat(_to, " (");
    strcat(_to, _subtitle);
    strcat(_to, ")");
  }
  strcat(_to, "</div><input id=\"");
  strcat(_to, _id);
  strcat(_to, "\" name=\"");
  strcat(_to, _name);
  strcat(_to, "\" value=\"");
  strcat(_to, _value);
  strcat(_to, "\" ");
  if( _pass != NULL )
  {
    strcat(_to, "type=\"password\" placeholder=\"");
    strcat(_to, _pass);
    strcat(_to, "\" ");
  }
  strcat(_to, "><br>");
}

const char *ParamSmTableStart = "<div style=\"margin-top:0;padding:2px;text-align:left;\"><table>";
const char *ParamSmTableEnd = "</table></div>";
/*
 * Smaller footprint param editor (all horz)
 */
void HtmlParamParamSm( char*_to, const char *_title, const char *_value, const char *_id, const char *_name)
{
  strcat(_to, "<tr><td><b>");
  strcat(_to, _title);
  strcat(_to, ":&nbsp;</b>");
  strcat(_to, "</td><td><input id=\"");
  strcat(_to, _id);
  strcat(_to, "\" name=\"");
  strcat(_to, _name);
  strcat(_to, "\" value=\"");
  strcat(_to, _value);
  strcat(_to, "\" ");
  strcat(_to, "></td></tr>");
}

char typeList[512];
/* Return a string with each output type as an option (and select _sel)
 * Used to pass option list to HtmlParamOutTypeList
  */
const char *HTMLOutTypeList( int _sel, bool _addRGB)
{
  char *_to = typeList;
  char tb[8];
  sprintf(_to,"<option value=\"%d\"", OUTTYPE_NONE);
  if( _sel == OUTTYPE_NONE ) strcat(_to, " selected ");
  strcat(_to, ">N/a</option>");

  sprintf(tb,"<option value=\"%d\"", OUTTYPE_ONOFF);
  strcat(_to, tb);
  if( _sel == OUTTYPE_ONOFF ) strcat(_to, " selected ");
  strcat(_to, ">On/Off</option>");
  if( _addRGB)
  {
    sprintf(tb,"<option value=\"%d\"", OUTTYPE_RGB);
    strcat(_to, tb);
    if( _sel == OUTTYPE_RGB ) strcat(_to, " selected ");
    strcat(_to, ">RGB</option>");
  }
  sprintf(tb,"<option value=\"%d\"", OUTTYPE_OFFON);
  strcat(_to, tb);
  if( _sel == OUTTYPE_OFFON ) strcat(_to, " selected ");
  strcat(_to, ">Off/On</option>");
  sprintf(tb,"<option value=\"%d\"", OUTTYPE_ANALOG);
  strcat(_to, tb);
  if( _sel == OUTTYPE_ANALOG ) strcat(_to, " selected ");
  strcat(_to, ">Analog</option>");
  return typeList;
}

/* Return a string with each switch type as an option (and select _sel)
 * Used to pass option list to HtmlParamOutTypeList
  */
const char *HTMLSwTypeList( int _sel )
{
  char *_to = typeList;
  sprintf(_to, "<option value=\"0\"");
  if( _sel == 0 ) strcat(_to, " selected");
  strcat(_to, ">N/a</option>");
  strcat(_to, "<option value=\"1\"");
  if( _sel == 1 ) strcat(_to, " selected");
  strcat(_to, ">Press</option>");
  strcat(_to, "<option value=\"2\"");
  if( _sel == 2 ) strcat(_to, " selected");
  strcat(_to, ">Release</option>");
  strcat(_to, "<option value=\"3\"");
  if( _sel == 3 ) strcat(_to, " selected");
  strcat(_to, ">Pushbutton</option>");
  strcat(_to, "<option value=\"4\"");
  if( _sel == 4 ) strcat(_to, " selected");
  strcat(_to, ">Dual</option>");
  strcat(_to, "<option value=\"5\"");
  if( _sel == 5 ) strcat(_to, " selected");
  strcat(_to, ">Delay</option>");
  strcat(_to, "<option value=\"6\"");
  if( _sel == 6 ) strcat(_to, " selected");
  strcat(_to, ">On/Off</option>");
  return typeList;
}

/*
 * param as a dropdown list...
 */
void HtmlParamOutTypeList( char *_to, const char *_title, const char *_id, const char *_name, const char * _list)
{
  strcat(_to, "<tr><td><b>");
  strcat(_to, _title);
  strcat(_to,  ":&nbsp;</b>");
  //strcat(_to, "</td><td><select form=\"pform\" id=\"";
  strcat(_to, "</td><td><select id=\"");
  strcat(_to, _id);
  strcat(_to, "\" name=\"");
  strcat(_to, _name);
  strcat(_to, "\">");
  strcat(_to, _list);
  strcat(_to, "</select></td></tr>");
}

char *HtmlSaveButton = "<br><button class=\"button bgrn\" name=\"save\" type=\"submit\">Save</button>";
char *HtmlParamBoxEnd = "</form></fieldset>";

/*
 * Add a button to the html
 */
void AddButton(char *_to, const char *_name, const char *_path, const char *_tags = NULL, const char *_class= NULL, const char *_inputs = NULL)
{
  strcat(_to, "<form id=\"tform\" action=\"");
  strcat(_to, _path);
  strcat(_to, "\" method=\"get\" ");
  if( _tags != NULL ) strcat(_to, _tags);
  strcat(_to, ">");
  if( _inputs != NULL ) strcat(_to, _inputs);
  strcat(_to, "<button class='");
  if( _class != NULL ) strcat(_to, _class);
  strcat(_to, "'>");
  strcat(_to, _name);
  strcat(_to, "</button><br></form>");
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

void AddTableRow(char *_to, const char *_head, const char *_data)
{
  strcat(_to, "<tr><th>");
  strcat(_to, _head);
  strcat(_to, "<th><td>");
  strcat(_to, _data);
  strcat(_to, "</td><tr>");
}

void handleReboot()
{
  Serial.println("Serving /reboot");

  setupAutoDisco(true);
  Serial.println("Auto discovery reset");
  HtmlModule(rBuffer, "Reboot");
  strcat(rBuffer, HtmlMenu);

  strcat(rBuffer, "<div>Module will now reboot...</div>");
  strcat(rBuffer, "<br>");
  AddButton(rBuffer, "Main Menu", "/");
  strcat(rBuffer, HtmlMenuEnd);
  response();
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
void HtmlSwitch( char *_to, const char *_name, const char *_message, bool _outEnabled, int _id, int _type, bool _rgb=false )
{
  // show the current state of the output pin:
  strcat(_to, "<div style=\"text-align:center;font-size:1.5rem;border:solid 1px black;border-radius:5px;margin:2px;\">");
  if( _outEnabled )
  {
    strcat(_to, GetOutput(_id)?"ON":"OFF");
  }
  else
  { // output not connected
    strcat(_to, "N/a");
  }
  strcat(_to, "</div>");
  // add a button to simulate pressing the switch:
  char inputs[1024];
  sprintf(inputs, "<input type='hidden' name='%s'>", _message);
  if( _rgb )
  {
    char tb[10], tb2[10];
    sprintf(tb, "%d", options.RGBCount());
    sprintf(tb2, "%d", GetStripLength());
    strcat(inputs, "<div class=\"slidecontainer\">");
    strcat(inputs, "<input type=\"range\" min=\"1\" max=\"");
    strcat(inputs, tb);
    strcat(inputs, "\" value=\"");
    strcat(inputs, tb2);
    strcat(inputs, "\" class=\"slider\" name=\"");
    strcat(inputs, "StripLength");
    strcat(inputs, "\" onchange=\"document.getElementById('tform').submit();\">");
    strcat(inputs, "</div>");
  }
  AddButton(_to, _name, "", "style='margin:2px;'", _type==0?"button bnot":"", inputs);
}

/*
 * Output ability to fake rotary via web
 */
void HtmlRotary( char *_to, const char *_name, bool _disabled)
{
  return ; // currently disabled until better interface can be thought of ;)
  String dat1 = String("<input type='hidden' name='") + _name + "' value='-1'>";
  String dat2 = String("<input type='hidden' name='") + _name + "' value='1'>";
  strcat(_to, "<div style='padding:0;'><table width='100%%' style='margin:0px;padding:0;border:0;'><tr><td>");
  AddButton(_to, "<", "", "style='margin:2px;'", _disabled?"button bnot":"", dat1.c_str());
  strcat(_to, "</td><td>");
  AddButton(_to, ">", "", "style='margin:2px;'", _disabled?"button bnot":"", dat2.c_str());
  strcat(_to, "</td></tr></table></div>");
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
  HtmlModule(rBuffer, "Main Menu");
  strcat(rBuffer, HtmlMenu);

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
  strcat(rBuffer, "<table width=\"100%%\"><tr>");
  for( int i=0;i<NUM_OUTS ; i++ )
  {
    if( options.SwType(i+1) != SWTYPE_NONE || options.OutType(i+1) != OUTTYPE_NONE)
    {
      bool rgb = false;
      char tbuf[16];
      sprintf(tbuf,"Switch%d",i+1);
      strcat(rBuffer, "<td valign='top'>");
      //HtmlSwitch(rBuffer, tbuf+6, tbuf, options.OutPin(i+1) != -1, i+1, options.SwType(i+1));
      if( i == 0 && options.OutType(1) == OUTTYPE_RGB ) rgb = true;
      HtmlSwitch(rBuffer, options.FriendlyNameParsed(i+1), tbuf, options.OutPin(i+1) != -1, i+1, options.SwType(i+1), rgb);
      //HtmlRotary( rBuffer, "Rotary1", options.Rot1PinU() != -1);
      strcat(rBuffer, "</td>");
      if( ++cellcount > 1 )
      { // new row once 2 items outputted
        cellcount = 0;
        strcat(rBuffer, "</tr><tr>");
      }
    }
  }
  strcat(rBuffer, "</tr></table>");
  // now the buttons:
  strcat(rBuffer, "<br>");
  //AddButton(rBuffer, "Status", "status");
  AddButton(rBuffer, "Configuration", "config");
  AddButton(rBuffer, "Information", "info");
  AddButton(rBuffer, "Update", "update");
  strcat(rBuffer, "<br>");
  AddButton(rBuffer, "Restart", "/reboot", "onsubmit=\"return confirm('Confirm Restart');\"", "button bred");
  Serial.println("Contents done");
  strcat(rBuffer, HtmlMenuEnd);
  response();
}

void handleUpdate()
{
  Serial.println("Serving /update");
  HtmlModule(rBuffer, "Update");
  strcat(rBuffer, HtmlMenu);
  if(TheServer.args() > 0 )
  {
    // args have come back, so let's apply them...
    for(int i=0 ; i < TheServer.args() ; i++)
    {
      if( TheServer.argName(i) == "URL" )
      {
        options.UpdateServer( TheServer.arg(i).c_str() );
        UpdateURL = TheServer.arg(i);
      }
      else
      if(  TheServer.argName(i) == "update" )
      {
        UpdateNow = true;
        strcat(rBuffer, "<p>Downloading from server ...</p><br>");
        AddButton(rBuffer, "Main Menu", "/");
        strcat(rBuffer, HtmlMenuEnd);
        response();
        return;
      }
    }
  }

  HtmlParamBoxStart(rBuffer, "Update Params", "./update");
  HtmlParamParam(rBuffer, "URL", NULL, options.UpdateServer(), "URL", "URL", NULL);
  strcat(rBuffer, HtmlSaveButton);
  strcat(rBuffer, "<br><button class=\"button bgrn\" name=\"update\" type=\"submit\">Update</button>");
  strcat(rBuffer, "<br>");
  AddButton(rBuffer, "Main Menu", "/");
  strcat(rBuffer, HtmlMenuEnd);
  response();
}

void handleInfo()
{
  Serial.println("Serving /info");
  HtmlModule(rBuffer, "Information");
  strcat(rBuffer, HtmlMenu);
  strcat(rBuffer, "<div><table style=\"width:100%;\"><tbody>");
  AddTableRow(rBuffer, "Web Name", options.WebNameParsed());
  AddTableRow(rBuffer, "SSID (RSSI)", (WiFi.SSID() + " ("+WiFi.RSSI()+"%)").c_str());
  AddTableRow(rBuffer, "IP Address", WiFi.localIP().toString().c_str());
  AddTableRow(rBuffer, "Gateway", WiFi.gatewayIP().toString().c_str());
  AddTableRow(rBuffer, "MQTT Host", options.MqttHost());
  AddTableRow(rBuffer, "MQTT connection", PSclient.connected()?"Connected":"Disconnected");
  char tb[8];
  sprintf(tb,"%d", system_get_free_heap_size());
  AddTableRow(rBuffer, "Free Mem", tb);
  sprintf(tb,"%d", ESP.getChipId());
  AddTableRow(rBuffer, "ESP Chip ID", tb);
  sprintf(tb,"%d", ESP.getFlashChipId());
  AddTableRow(rBuffer, "Flash Chip ID", tb);
  sprintf(tb,"%d", ESP.getFlashChipRealSize() / 1024);
  AddTableRow(rBuffer, "Flash Size", tb);
  sprintf(tb,"%d", ESP.getFlashChipSize() / 1024);
  AddTableRow(rBuffer, "Flash Program Size", tb);
  sprintf(tb,"%d", ESP.getSketchSize() / 1024);
  AddTableRow(rBuffer, "Program Size", tb);
  sprintf(tb,"%d", ESP.getFreeSketchSpace() / 1024);
  AddTableRow(rBuffer, "Free Program Space", tb);

  strcat(rBuffer, "</tbody></table>");
  AddButton(rBuffer, "Main Menu", "/");
  strcat(rBuffer, HtmlMenuEnd);
  response();
}

void handleSwitches1()
{
  Serial.println("Serving /config/sw1");
  HtmlModule(rBuffer,"Switches1");

  handlePageArgs( true );

  strcat(rBuffer, HtmlMenu);

  HtmlParamBoxStart(rBuffer, "Switches", "./sw1");

  strcat(rBuffer, ParamSmTableStart);
  for( int i=0 ; i < 4 ; i++ )
  {
    char n1buf[16], n2buf[16], n3buf[16], n4buf[16];
    sprintf(n1buf,"S%d Type", i+1);
    sprintf(n2buf,"S%d Topic", i+1);
    sprintf(n3buf,"Sw%dType", i+1);
    sprintf(n4buf,"Sw%dTopic", i+1);
    HtmlParamOutTypeList(rBuffer, n1buf, "", n3buf, HTMLSwTypeList(options.SwType(i+1)));
    HtmlParamParamSm(rBuffer, n2buf, options.SwTopic(i+1), "", n4buf);
  }
  strcat(rBuffer, ParamSmTableEnd);
  strcat(rBuffer, HtmlSaveButton);
  strcat(rBuffer, HtmlParamBoxEnd);
  strcat(rBuffer, "<br>");
  AddButton(rBuffer, "Configuration", "/config");
  strcat(rBuffer, HtmlMenuEnd);
  response();
}

void handleSwitches2()
{
  Serial.println("Serving /config/sw2");
  HtmlModule(rBuffer,"Switches2");

  handlePageArgs( true );

  strcat(rBuffer, HtmlMenu);

  HtmlParamBoxStart(rBuffer, "Switches", "./sw2");

  strcat(rBuffer, ParamSmTableStart);
  for( int i=4 ; i < NUM_SWITCHES ; i++ )
  {
    char n1buf[16], n2buf[16], n3buf[16], n4buf[16];
    sprintf(n1buf,"S%d Type", i+1);
    sprintf(n2buf,"S%d Topic", i+1);
    sprintf(n3buf,"Sw%dType", i+1);
    sprintf(n4buf,"Sw%dTopic", i+1);
    HtmlParamOutTypeList(rBuffer, n1buf, "", n3buf, HTMLSwTypeList(options.SwType(i+1)));
    HtmlParamParamSm(rBuffer, n2buf, options.SwTopic(i+1), "", n4buf);
  }
  strcat(rBuffer, ParamSmTableEnd);
  strcat(rBuffer, HtmlSaveButton);
  strcat(rBuffer, HtmlParamBoxEnd);
  strcat(rBuffer, "<br>");
  AddButton(rBuffer, "Configuration", "/config");
  strcat(rBuffer, HtmlMenuEnd);
  response();
}

void handleRotaries()
{
  Serial.println("Serving /config/rot");
  HtmlModule(rBuffer,"Rotaries");

  handlePageArgs( true );

  strcat(rBuffer, HtmlMenu);

  HtmlParamBoxStart(rBuffer, "Rotary Params", "./rot");

  strcat(rBuffer, ParamSmTableStart);
  for( int i=0 ; i < NUM_ROTS ; i++ )
  {
    char n1buf[16], n2buf[16], n3buf[16], n4buf[16];
    //sprintf(n1buf,"Sw%d Type", i+1);
    sprintf(n2buf,"Rot%d Topic", i+1);
    //sprintf(n3buf,"Sw%dType", i+1);
    sprintf(n4buf,"Rot%dTopic", i+1);
    //HtmlParamOutTypeList(rBuffer, n1buf, "", n3buf, HTMLSwTypeList(options.SwType(i+1)));
    HtmlParamParamSm(rBuffer, n2buf, options.RotTopic(i+1), "", n4buf);
  }
  strcat(rBuffer, ParamSmTableEnd);
  strcat(rBuffer, HtmlSaveButton);
  strcat(rBuffer, HtmlParamBoxEnd);
  strcat(rBuffer, "<br>");
  AddButton(rBuffer, "Configuration", "/config");
  strcat(rBuffer, HtmlMenuEnd);

  response();
}

void handleAnalog()
{
  char tbuf[16];
  Serial.println("Serving /config/analog");
  HtmlModule(rBuffer,"Analog");

  handlePageArgs( true );

  strcat(rBuffer, HtmlMenu);

  HtmlParamBoxStart(rBuffer, "Analog Params", "./analog");

  strcat(rBuffer, ParamSmTableStart);

  HtmlParamParamSm(rBuffer, "Topic", options.AnalogTopic(), "", "AnalogTopic");
  HtmlParamParamSm(rBuffer, "Trigger", options.AnalogTrigger(), "", "AnalogTrigger");

  sprintf(tbuf,"%d", options.ThresholdLow());
  HtmlParamParamSm(rBuffer, "Threshold Low",  tbuf, "", "ThresholdLow");
  sprintf(tbuf,"%d", options.ThresholdMid());
  HtmlParamParamSm(rBuffer, "Threshold Mid",  tbuf, "", "ThresholdMid");
  sprintf(tbuf,"%d", options.ThresholdHigh());
  HtmlParamParamSm(rBuffer, "Threshold High",  tbuf, "", "ThresholdHigh");

  strcat(rBuffer, ParamSmTableEnd);
  strcat(rBuffer, HtmlSaveButton);
  strcat(rBuffer, HtmlParamBoxEnd);
  strcat(rBuffer, "<br>");
  AddButton(rBuffer, "Configuration", "/config");
  strcat(rBuffer, HtmlMenuEnd);

  response();
}

void handleLEDs1()
{
  char tbuf[16];
  Serial.println("Serving /config/led1");
  HtmlModule(rBuffer, "LEDs1");

  handlePageArgs( true );

  strcat(rBuffer, HtmlMenu);

  HtmlParamBoxStart(rBuffer, "LED Params", "./led1");

  strcat(rBuffer, ParamSmTableStart);

  for( int i=0 ; i < 4 ; i++ )
  {
    char n1buf[16], n2buf[16], n3buf[16], n4buf[16];
    sprintf(n1buf,"Out%d Type", i+1);
    sprintf(n2buf,"LED%d Topic", i+1);
    sprintf(n3buf,"Out%dType", i+1);
    sprintf(n4buf,"LED%dTopic", i+1);
    HtmlParamOutTypeList(rBuffer, n1buf, "", n3buf, HTMLOutTypeList(options.OutType(i+1), i==0 /* only first one can be RGB */));
    HtmlParamParamSm(rBuffer, n2buf, options.LEDTopic(i+1), "", n4buf);
  }
  sprintf(tbuf, "%d", options.RGBCount());
  HtmlParamParamSm(rBuffer, "RGB LED Count", tbuf, "", "RGBCount");

  strcat(rBuffer, ParamSmTableEnd);
  strcat(rBuffer, HtmlSaveButton);
  strcat(rBuffer, HtmlParamBoxEnd);
  strcat(rBuffer, "<br>");
  AddButton(rBuffer, "Configuration", "/config");
  strcat(rBuffer, HtmlMenuEnd);

  response();
}

void handleLEDs2()
{
  char tbuf[16];
  Serial.println("Serving /config/led2");
  HtmlModule(rBuffer,"LEDs2");

  handlePageArgs( true );

  strcat(rBuffer, HtmlMenu);

  HtmlParamBoxStart(rBuffer, "LED Params", "./led2");

  strcat(rBuffer, ParamSmTableStart);

  for( int i=4 ; i < NUM_OUTS ; i++ )
  {
    char n1buf[16], n2buf[16], n3buf[16], n4buf[16];
    sprintf(n1buf,"Out%d Type", i+1);
    sprintf(n2buf,"LED%d Topic", i+1);
    sprintf(n3buf,"Out%dType", i+1);
    sprintf(n4buf,"LED%dTopic", i+1);
    HtmlParamOutTypeList(rBuffer, n1buf, "", n3buf, HTMLOutTypeList(options.OutType(i+1), i==0 /* only first one can be RGB */));
    HtmlParamParamSm(rBuffer, n2buf, options.LEDTopic(i+1), "", n4buf);
  }
  sprintf(tbuf, "%d", options.RGBCount());
  HtmlParamParamSm(rBuffer, "RGB LED Count", tbuf, "", "RGBCount");

  strcat(rBuffer, ParamSmTableEnd);
  strcat(rBuffer, HtmlSaveButton);
  strcat(rBuffer, HtmlParamBoxEnd);
  strcat(rBuffer, "<br>");
  AddButton(rBuffer, "Configuration", "/config");
  strcat(rBuffer, HtmlMenuEnd);

  response();
}

void handleModule()
{
  char tbuf[4];
  Serial.println("Serving /config/module");
  HtmlModule(rBuffer,"Module");

  handlePageArgs( true );

  strcat(rBuffer, HtmlMenu);

  HtmlParamBoxStart(rBuffer, "Module Params", "./module");
  HtmlParamParam(rBuffer, "Name", options.Unit(), options.Unit(), "unit", "Unit", NULL);
  //HtmlParamParam(rBuffer, "SoftAP", String(options.APServer()).c_str(), String(options.APServer()).c_str(), "port", "APServer", NULL);
  HtmlParamParam(rBuffer, "WebName", options.WebNameParsed(), options.WebName(), "wname", "WebName", NULL);
  HtmlParamParam(rBuffer, "Password", NULL, options.AP_pass(), "pwd1", "WebPassword", "password");
  for(int i=0 ; i<NUM_OUTS ; i++)
  {
    char nbuf[20],n2buf[16];
    sprintf(nbuf,"Friendly Name %d",i+1);
    sprintf(n2buf,"FriendlyName%d",i+1);
    HtmlParamParam(rBuffer, nbuf, options.FriendlyNameParsed(i+1), options.FriendlyName(i+1), "", n2buf, NULL);
  }

  HtmlParamParam(rBuffer, "Prefix1", options.Prefix1(), options.Prefix1(), "pre1", "Prefix1", NULL);
  HtmlParamParam(rBuffer, "Prefix2", options.Prefix2(), options.Prefix2(), "pre2", "Prefix2", NULL);
  HtmlParamParam(rBuffer, "Prefix3", options.Prefix3(), options.Prefix3(), "pre3", "Prefix3", NULL);
  strcat(rBuffer, HtmlSaveButton);
  strcat(rBuffer, HtmlParamBoxEnd);
  strcat(rBuffer, "<br>");
  AddButton(rBuffer, "Configuration", "/config");
  strcat(rBuffer, HtmlMenuEnd);

  response();
}

void handleHW()
{
  char tbuf[4];
  Serial.println("Serving /config/hw");
  HtmlModule(rBuffer,"Hardware");

  handlePageArgs( true );

  strcat(rBuffer, HtmlMenu);

  HtmlParamBoxStart(rBuffer, "Hardware", "./hw");
  strcat(rBuffer, ParamSmTableStart);
  for(int i=0 ; i < NUM_SWITCHES ; i++)
  {
    char n1buf[16], n2buf[16];
    sprintf(n1buf,"Sw%d Pin 1", i+1);
    sprintf(n2buf,"Sw%dPin1", i+1);
    sprintf(tbuf, "%d", options.SwPin1(i+1)); HtmlParamParamSm(rBuffer, n1buf, tbuf, "", n2buf);
    sprintf(n1buf,"Sw%d Pin 2", i+1);
    sprintf(n2buf,"Sw%dPin2", i+1);
    sprintf(tbuf, "%d", options.SwPin2(i+1)); HtmlParamParamSm(rBuffer, n1buf, tbuf, "", n2buf);
  }

  sprintf(tbuf, "%d", options.StatusLED()); HtmlParamParamSm(rBuffer, "Status LED", tbuf, "", "StatusLED");
  sprintf(tbuf, "%d", options.InvertLED()); HtmlParamParamSm(rBuffer, "Invert LED", tbuf, "", "InvertLED");

  strcat(rBuffer, ParamSmTableEnd);
  strcat(rBuffer, HtmlSaveButton);
  strcat(rBuffer, HtmlParamBoxEnd);
  strcat(rBuffer, "<br>");
  AddButton(rBuffer, "Configuration", "/config");
  strcat(rBuffer, HtmlMenuEnd);

  response();
}

void handleHW2()
{
  char tbuf[4];
  Serial.println("Serving /config/hw2");
  HtmlModule(rBuffer,"Hardware2");

  handlePageArgs( true );

  strcat(rBuffer, HtmlMenu);

  HtmlParamBoxStart(rBuffer, "Hardware", "./hw");
  strcat(rBuffer, ParamSmTableStart);
  for(int i=0 ; i < NUM_OUTS ; i++)
  {
    char n1buf[16], n2buf[16];
    sprintf(n1buf,"Out%d Pin", i+1);
    sprintf(n2buf,"Out%dPin", i+1);
    sprintf(tbuf, "%d", options.OutPin(i+1)); HtmlParamParamSm(rBuffer, n1buf, tbuf, "", n2buf);
    sprintf(n1buf,"Out%d LED", i+1);
    sprintf(n2buf,"Out%dLED", i+1);
    sprintf(tbuf, "%d", options.OutLED(i+1)); HtmlParamParamSm(rBuffer, n1buf, tbuf, "", n2buf);
  }

  for(int i=0 ; i < NUM_ROTS ; i++)
  {
    char n1buf[16], n2buf[16];
    sprintf(n1buf,"Rot%d PinU", i+1);
    sprintf(n2buf,"Rot%dPinU", i+1);
    sprintf(tbuf, "%d", options.RotPinU(i+1)); HtmlParamParamSm(rBuffer, n1buf, tbuf, "", n2buf);
    sprintf(n1buf,"Rot%d PinD", i+1);
    sprintf(n2buf,"Rot%dPinD", i+1);
    sprintf(tbuf, "%d", options.RotPinD(i+1)); HtmlParamParamSm(rBuffer, n1buf, tbuf, "", n2buf);
  }

  strcat(rBuffer, ParamSmTableEnd);
  strcat(rBuffer, HtmlSaveButton);
  strcat(rBuffer, HtmlParamBoxEnd);
  strcat(rBuffer, "<br>");
  AddButton(rBuffer, "Configuration", "/config");
  strcat(rBuffer, HtmlMenuEnd);

  response();
}

void handleConfig()
{
  Serial.println("Serving /config");
  HtmlModule(rBuffer,"Config");
  strcat(rBuffer, HtmlMenu);
  AddButton(rBuffer, "Module", "config/module");
  AddButton(rBuffer, "Switches1", "config/sw1");
  AddButton(rBuffer, "Switches2", "config/sw2");
  AddButton(rBuffer, "Rotaries", "config/rot");
  AddButton(rBuffer, "LEDs1", "config/led1");
  AddButton(rBuffer, "LEDs2", "config/led2");
  AddButton(rBuffer, "WiFi", "config/wifi");
  AddButton(rBuffer, "MQTT", "config/mqtt");
  AddButton(rBuffer, "Analog", "config/analog");
  AddButton(rBuffer, "Hardware", "config/hw");
  AddButton(rBuffer, "Hardware2", "config/hw2");
  strcat(rBuffer, "<br>");
  AddButton(rBuffer, "Main Menu", "/");
  strcat(rBuffer, HtmlMenuEnd);
  response();
}

void handleMQTT()
{
  char tb[16];
  Serial.println("Serving /config/mqtt");
  HtmlModule(rBuffer,"Mqtt");

  handlePageArgs( true );

  strcat(rBuffer, HtmlMenu);

  HtmlParamBoxStart(rBuffer, "Mqtt Params", "./mqtt");
  HtmlParamParam(rBuffer, "Host", options.MqttHost(), options.MqttHost(), "host", "MqttHost", NULL);
  sprintf(tb, "%d", options.MqttPort());
  HtmlParamParam(rBuffer, "Port", tb, tb, "port", "MqttPort", NULL);
  HtmlParamParam(rBuffer, "User", options.MqttUser(), options.MqttUser(), "user", "MqttUser", NULL);
  HtmlParamParam(rBuffer, "Password", NULL, "", "pwd1", "MqttPass", "password");
  HtmlParamParam(rBuffer, "Client", options.MqttClientParsed(), options.MqttClient(), "client", "MqttClient", NULL);
  HtmlParamParam(rBuffer, "Topic", options.MqttTopic(), options.MqttTopic(), "topic", "MqttTopic", NULL);
  HtmlParamParam(rBuffer, "Group Topic", options.MqttGroupTopic(), options.MqttGroupTopic(), "gtopic", "MqttGroupTopic", NULL);
  strcat(rBuffer, HtmlSaveButton);
  strcat(rBuffer, HtmlParamBoxEnd);
  strcat(rBuffer, "<br>");
  AddButton(rBuffer, "Configuration", "/config");
  strcat(rBuffer, HtmlMenuEnd);

  response();
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

  HtmlModule(rBuffer,"WiFi");

  strcat(rBuffer, "<script>function ei(i){return document.getElementById(i);}function c(l){ei('ss1').value=l.innerText||l.textContent;ei('pwd1').focus();}</script>");
  strcat(rBuffer, HtmlScan);
  strcat(rBuffer, "<p>");
  strcat(rBuffer, WIFI_status);
  strcat(rBuffer, "</p>");
  strcat(rBuffer, HtmlMenu);

  if( numNetworks > 0 )
  {
    for(int i=0;i<numNetworks ; i++)
    {
      char tb[8];
      strcat(rBuffer, "<div><a style=\"float:left;text-align:left;\" href=\"#fs\" onclick=\"c(this)\">");
      strcat(rBuffer, WiFi.SSID(i).c_str());
      strcat(rBuffer, "</a>");
      strcat(rBuffer, "(");
      sprintf(tb, "%d", WiFi.channel(i));
      strcat(rBuffer, tb);
      strcat(rBuffer, ")<span class=\"q\">");
      strcat(rBuffer, WiFi.BSSIDstr(i).c_str());
      strcat(rBuffer, "</span></div>");
    }
  }

  strcat(rBuffer, "<br>");
  HtmlParamBoxStart(rBuffer, "Wifi Params", "./wifi");
  HtmlParamParam(rBuffer, "SSID", WiFi.SSID().c_str(), WiFi.SSID().c_str(), "ss1", "Ssid", NULL);
  HtmlParamParam(rBuffer, "Password", NULL, "", "pwd1", "Password", "password");
  strcat(rBuffer, HtmlSaveButton);
  strcat(rBuffer, HtmlParamBoxEnd);
  strcat(rBuffer, "<br>");
  AddButton(rBuffer, "Configuration", "/config");
  strcat(rBuffer, HtmlMenuEnd);

  response();
}


///
// HTML general response
///
void response()
{
  strcat(rBuffer, HtmlFooter);
  strcat(rBuffer, "</body>");
  strcat(rBuffer,  HtmlHtmlClose);
  Serial.print("Serving ");
  Serial.print(strlen(rBuffer));
  Serial.print(" bytes data...");

  TheServer.setContentLength(strlen(rBuffer));
  char a = rBuffer[2000];
  rBuffer[2000]=0;
  TheServer.send(200, "text/html", rBuffer);
  rBuffer[2000] = a;
  if(strlen(rBuffer) > 2000 )
    TheServer.sendContent(rBuffer+2000);
  Serial.println(" Done");
}

void InitWebServer()
{
  TheServer.on("/", handleRoot);
  TheServer.on("/reboot", handleReboot);
  TheServer.on("/update", handleUpdate);
  TheServer.on("/info", handleInfo);
  TheServer.on("/config", handleConfig);
  TheServer.on("/config/module", handleModule);
  TheServer.on("/config/sw1", handleSwitches1);
  TheServer.on("/config/sw2", handleSwitches2);
  TheServer.on("/config/rot", handleRotaries);
  TheServer.on("/config/led1", handleLEDs1);
  TheServer.on("/config/led2", handleLEDs2);
  TheServer.on("/config/wifi", handleWifi);
  TheServer.on("/config/mqtt", handleMQTT);
  TheServer.on("/config/analog", handleAnalog);
  TheServer.on("/config/hw", handleHW);
  TheServer.on("/config/hw2", handleHW2);
  TheServer.begin();
}

void UpdateWebServer()
{
  TheServer.handleClient();
}
