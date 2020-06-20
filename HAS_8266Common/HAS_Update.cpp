/*
 * HAS_Update.cpp
 *
 * (C) 2019 James Dixon
 *
 */

#include "HAS_Update.h"
#include "HAS_option.h"
#include "config.h"
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPClient.h>

String UpdateURL;
bool UpdateNow = false;
bool Updating = false;

void TestServerUpdate()
{
  if( !Updating )
  { // if we're not updating, then we must have restarted, hopefully the new version!
    // first check the version is what we are actually running...
    if( strcmp(options.NewVersion(), options.Version()) != 0  )
    {
      options.Version( options.NewVersion() );
      // let's just check...
      String newver = options.Version();
      int upos = newver.lastIndexOf("_");
      newver = newver.substring(upos+1);
      if( newver != P_VERSION )
      {
        Serial.print("**WARNING: Image version ");
        Serial.print(options.Version());
        Serial.print(" (");
        Serial.print(options.NewVersion());
        Serial.print(") is not the same as running version ");
        Serial.println(P_VERSION);
      }
      else
      {
        Serial.print("***UPDATED: Now running version ");
        Serial.println(options.Version());
      }
    }
  }
  // only do update if we are on latest version (ie not some experimental beta)
  String buildtype = options.Version();
  if( buildtype.indexOf("latest") == -1 && buildtype.indexOf("Latest") == -1 && buildtype.indexOf("Local") == -1)
  {
    Serial.print("Update check - using non-'latest' build (");
    Serial.print(buildtype);
    Serial.println(") so aborted.");
    return;
  }
  UpdateURL = options.UpdateServer();
  String VersionURL = UpdateURL.substring(0,UpdateURL.lastIndexOf("/")) + "/version.php";
  Serial.print("Checking for new software update on ");
  Serial.print(VersionURL);
  Serial.print("...");

  HTTPClient client;
  client.begin(VersionURL);
  int httpCode = client.GET();
  if (httpCode == 200)
  {
    String payload = client.getString();
    Serial.print(payload);
    char tb[32];

    int fbuild = atoi(payload.substring(payload.lastIndexOf(".")+1).c_str());
    payload = payload.substring(0,payload.lastIndexOf("."));
    int fminor = atoi(payload.substring(payload.lastIndexOf(".")+1).c_str());
    payload = payload.substring(0,payload.lastIndexOf("."));
    int fmajor = atoi(payload.substring(payload.lastIndexOf(".")+1).c_str());
    sprintf(tb, "  Local:%d.%d.%d <> Remote:%d.%d.%d", VER_MAJOR, VER_MINOR, VER_BUILD, fmajor, fminor, fbuild);
    Serial.print(tb);
    if( fmajor > VER_MAJOR ||
      (fmajor == VER_MAJOR && fminor > VER_MINOR) ||
      (fmajor == VER_MAJOR && fminor == VER_MINOR && fbuild > VER_BUILD))
    {
      UpdateNow = true;
      Serial.println("...YES");
    }
    else
    {
      UpdateNow = false;
      Serial.println("...NO");
    }
  }
  else
  {
    Serial.print("Returned ");
    Serial.println(httpCode);
  }
  client.end();
}

void TestUpdate()
{
  if( UpdateNow )
  {
    char topic[128];
    sprintf(topic, "%s/%s/updating", options.MqttTopic(), options.Prefix4());
    UpdateNow = false;
    Serial.print("Updating using: ");
    Serial.println(UpdateURL);
    PSclient.publish(topic, UpdateURL.c_str(), false);
    //PSclient.loop();

    HTTPClient client;
    client.begin(UpdateURL);
    int httpCode = client.GET();
    if (httpCode == 200)
    {
        String payload = client.getString();
        UpdateURL = UpdateURL.substring(0,UpdateURL.lastIndexOf("/")) + payload;
        Serial.print("Updating to image: ");
        Serial.println(UpdateURL);
        options.NewVersion(UpdateURL.c_str()); // store the version we're updating to
        options.Save();
        Updating = true; // tell the (currently running) code that we are updating.
        ESPhttpUpdate.rebootOnUpdate(false);
        int ret = ESPhttpUpdate.update(UpdateURL);
        Serial.println(ret);
        switch(ret)
        {
          case HTTP_UPDATE_FAILED:
            Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s",  ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
            break;
          case HTTP_UPDATE_NO_UPDATES:
            Serial.println("HTTP_UPDATE_NO_UPDATES");
            break;
          case HTTP_UPDATE_OK:
            Serial.println("HTTP_UPDATE_OK");
            ESP.restart();
            break;
        }
    }
    else
    {
      Serial.print("HTTP error: ");
      Serial.print(client.getString());
      Serial.print(":");
      Serial.println(httpCode);
    }
  }
}
